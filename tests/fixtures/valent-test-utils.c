// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <locale.h>
#include <stdio.h>
#include <sys/socket.h>

#include <gtk/gtk.h>
#include <adwaita.h>
#include <json-glib/json-glib.h>
#include <libvalent-core.h>
#include <libvalent-device.h>
#include <libvalent-contacts.h>
#include <libvalent-clipboard.h>
#include <libvalent-input.h>
#include <libvalent-media.h>
#include <libvalent-mixer.h>
#include <libvalent-notifications.h>
#include <libvalent-session.h>
#include <libvalent-ui.h>

#include "libvalent-ui-resources.h"
#include "valent-component-private.h"
#include "valent-contact-cache-private.h"
#include "valent-mock-channel.h"
#include "valent-test-utils.h"


/*
 * Transfer Helpers
 */
typedef struct
{
  GRecMutex  lock;
  JsonNode  *packet;
  GFile     *file;
} TransferOperation;

static void
transfer_op_free (gpointer data)
{
  TransferOperation *op = data;

  g_rec_mutex_lock (&op->lock);
  g_clear_object (&op->file);
  g_clear_pointer (&op->packet, json_node_unref);
  g_rec_mutex_unlock (&op->lock);
  g_rec_mutex_clear (&op->lock);
  g_clear_pointer (&op, g_free);
}

static void
upload_task (GTask        *task,
             gpointer      source_object,
             gpointer      task_data,
             GCancellable *cancellable)
{
  TransferOperation *op = task_data;
  g_autoptr (GIOStream) stream = NULL;
  g_autoptr (GFileInfo) file_info = NULL;
  g_autoptr (GFileInputStream) file_source = NULL;
  goffset size;
  GError *error = NULL;

  g_rec_mutex_lock (&op->lock);

  file_info = g_file_query_info (op->file, "standard::size", 0, NULL, &error);

  if (file_info == NULL)
    {
      g_rec_mutex_unlock (&op->lock);
      return g_task_return_error (task, error);
    }

  file_source = g_file_read (op->file, cancellable, &error);

  if (file_source == NULL)
    {
      g_rec_mutex_unlock (&op->lock);
      return g_task_return_error (task, error);
    }

  size = g_file_info_get_size (file_info);
  valent_packet_set_payload_size (op->packet, size);

  stream = valent_channel_upload (VALENT_CHANNEL (source_object),
                                  op->packet,
                                  cancellable,
                                  &error);

  if (stream == NULL)
    {
      g_rec_mutex_unlock (&op->lock);
      return g_task_return_error (task, error);
    }

  g_output_stream_splice (g_io_stream_get_output_stream (stream),
                          G_INPUT_STREAM (file_source),
                          (G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                           G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET),
                          cancellable,
                          &error);
  g_rec_mutex_unlock (&op->lock);

  if (error != NULL)
    return g_task_return_error (task, error);

  g_task_return_boolean (task, TRUE);
}

static void
download_task (GTask        *task,
               gpointer      source_object,
               gpointer      task_data,
               GCancellable *cancellable)
{
  JsonNode *packet = task_data;
  g_autoptr (GIOStream) stream = NULL;
  g_autoptr (GOutputStream) target = NULL;
  GError *error = NULL;

  stream = valent_channel_download (VALENT_CHANNEL (source_object),
                                    packet,
                                    cancellable,
                                    &error);

  if (stream == NULL)
    return g_task_return_error (task, error);

  target = g_memory_output_stream_new_resizable ();

  g_output_stream_splice (target,
                          g_io_stream_get_input_stream (stream),
                          (G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
                           G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET),
                          cancellable,
                          &error);

  if (error != NULL)
    return g_task_return_error (task, error);

  g_task_return_boolean (task, TRUE);
}


/**
 * valent_test_mute_domain:
 * @log_domain: the log domain of the message
 * @log_level:
 * @message:
 * @user_data: the log domain to match against
 *
 * A #GTestLogFatalFunc for preventing fatal errors if @log_domain matches
 * @user_data with g_strcmp0().
 */
gboolean
valent_test_mute_domain (const char     *log_domain,
                         GLogLevelFlags  log_level,
                         const char     *message,
                         gpointer        user_data)
{
  const char *match_domain = user_data;

  if (g_strcmp0 (log_domain, match_domain) == 0)
    return FALSE;

  return TRUE;
}

/**
 * valent_test_mute_match:
 * @log_domain: the log domain of the message
 * @log_level:
 * @message:
 * @user_data: the log domain to match against
 *
 * A #GTestLogFatalFunc for preventing fatal errors if @message matches
 * @user_data with g_match_regex_simple().
 */
gboolean
valent_test_mute_match (const char     *log_domain,
                        GLogLevelFlags  log_level,
                        const char     *message,
                        gpointer        user_data)
{
  if (g_regex_match_simple (user_data, message, 0, 0) ||
      g_pattern_match_simple (user_data, message))
    return FALSE;

  return TRUE;
}

/**
 * valent_test_mute_warning:
 * @log_domain: the log domain of the message
 * @log_level:
 * @message:
 * @user_data: the log domain to match against
 *
 * A #GTestLogFatalFunc for preventing warnings from aborting a test.
 */
gboolean
valent_test_mute_warning (const char     *log_domain,
                          GLogLevelFlags  log_level,
                          const char     *message,
                          gpointer        user_data)
{
  if (log_level & G_LOG_LEVEL_WARNING)
    return FALSE;

  return TRUE;
}

/**
 * valent_test_mute_fuzzing:
 * @log_domain: the log domain of the message
 * @log_level: the log level of the message
 * @message: the message to process
 * @user_data: the log domain to match against
 *
 * A #GTestLogFatalFunc for quieting fatal logging during fuzzing tests. This
 * includes messages:
 *
 * - @log_domain is `Json`
 * - @log_level is %G_LOG_LEVEL_WARNING
 *
 * Returns: %TRUE if not matching
 */
gboolean
valent_test_mute_fuzzing (const char     *log_domain,
                          GLogLevelFlags  log_level,
                          const char     *message,
                          gpointer        user_data)
{
  if (log_level & G_LOG_LEVEL_WARNING)
    return FALSE;

  if (g_strcmp0 (log_domain, "Json") == 0)
    return FALSE;

  return TRUE;
}

static GQueue *events = NULL;

/**
 * valent_test_event_free:
 * @free_func: the function to be called to free each event item
 *
 * Clear the event queue.
 */
void
valent_test_event_free (GDestroyNotify free_func)
{
  if (events == NULL)
    return;

  g_queue_free_full (events, free_func);
  events = NULL;
}

/**
 * valent_test_event_pop:
 *
 * Remove and return the event at the head of the queue
 *
 * Returns: (transfer full): an event
 */
gpointer
valent_test_event_pop (void)
{
  if (events == NULL)
    return NULL;

  return g_queue_pop_head (events);
}

/**
 * valent_test_event_push:
 * @event: an event
 *
 * Add @event to the end of the event queue.
 */
void
valent_test_event_push (gpointer event)
{
  if (events == NULL)
    events = g_queue_new ();

  g_queue_push_tail (events, event);
}

/**
 * valent_test_load_json:
 * @path: (type filename): path to JSON file
 *
 * A simple convenience to load the JSON at @path.
 *
 * Returns: (transfer full): a #JsonNode
 */
JsonNode *
valent_test_load_json (const char *path)
{
  g_autoptr (JsonParser) parser = NULL;
  GError *error = NULL;

  parser = json_parser_new ();
  json_parser_load_from_file (parser, path, &error);
  g_assert_no_error (error);

  return json_parser_steal_root (parser);
}

/**
 * valent_test_await_adapter:
 * @component: (type Valent.Component): a #ValentComponent
 *
 * Wait for a [class@Valent.Component] adapter to load and return it.
 *
 * Returns: a component adapter
 */
gpointer
valent_test_await_adapter (gpointer component)
{
  gpointer ret = NULL;

  g_assert (VALENT_IS_COMPONENT (component));

  while ((ret = valent_component_get_preferred (component)) == NULL)
    g_main_context_iteration (NULL, FALSE);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  return ret;
}

static gboolean
valent_test_wait_cb (gpointer data)
{
  gboolean *done = data;

  if (done != NULL)
    *done = TRUE;

  return G_SOURCE_REMOVE;
}

/**
 * valent_test_wait:
 * @duration: the time to wait, in milliseconds
 *
 * Iterate the default main context for @duration.
 */
void
valent_test_wait (unsigned int duration)
{
  gboolean done = FALSE;

  g_timeout_add (duration, valent_test_wait_cb, &done);

  while (!done)
    g_main_context_iteration (NULL, FALSE);
}

/**
 * valent_test_channel_pair:
 * @identity: a #JsonNode
 * @peer_identity: (nullable): a #JsonNode
 *
 * Create a pair of connected channels with @identity representing the local
 * device and @peer_identity representing the endpoint device.
 *
 * Returns: (array length=2) (element-type Valent.Channel): a pair of #ValentChannel
 */
ValentChannel **
valent_test_channel_pair (JsonNode *identity,
                          JsonNode *peer_identity)
{
  ValentChannel **channels = NULL;
  int sv[2] = { 0, };
  g_autoptr (GSocket) channel_socket = NULL;
  g_autoptr (GSocket) endpoint_socket = NULL;
  g_autoptr (GSocketConnection) channel_connection = NULL;
  g_autoptr (GSocketConnection) endpoint_connection = NULL;
  GError *error = NULL;

  g_assert (VALENT_IS_PACKET (identity));
  g_assert (peer_identity == NULL || VALENT_IS_PACKET (peer_identity));

  channels = g_new0 (ValentChannel *, 2);
  g_assert_no_errno (socketpair (AF_UNIX, SOCK_STREAM, 0, sv));

  /* This is the "local" representation of the mock "remote" device */
  channel_socket = g_socket_new_from_fd (sv[0], &error);
  g_assert_no_error (error);
  channel_connection = g_object_new (G_TYPE_SOCKET_CONNECTION,
                                     "socket", channel_socket,
                                     NULL);
  channels[0] = g_object_new (VALENT_TYPE_MOCK_CHANNEL,
                              "base-stream",   channel_connection,
                              "identity",      identity,
                              "peer-identity", peer_identity,
                              NULL);

  /* This is the "remote" representation of our mock "local" service */
  endpoint_socket = g_socket_new_from_fd (sv[1], &error);
  g_assert_no_error (error);
  endpoint_connection = g_object_new (G_TYPE_SOCKET_CONNECTION,
                                      "socket", endpoint_socket,
                                      NULL);
  channels[1] = g_object_new (VALENT_TYPE_MOCK_CHANNEL,
                              "base-stream",   endpoint_connection,
                              "identity",      peer_identity,
                              "peer-identity", identity,
                              NULL);

  return channels;
}

/**
 * valent_test_download:
 * @channel: a #ValentChannel
 * @packet: a #JsonNode
 * @error: (nullable): a #GError
 *
 * Simulate downloading the payload described by @packet using @channel.
 *
 * Returns: %TRUE if successful
 */
gboolean
valent_test_download (ValentChannel  *channel,
                      JsonNode       *packet,
                      GError        **error)
{
  g_autoptr (GTask) task = NULL;

  g_assert (VALENT_IS_CHANNEL (channel));
  g_assert (VALENT_IS_PACKET (packet));
  g_assert (error == NULL || *error == NULL);

  task = g_task_new (channel, NULL, NULL, NULL);
  g_task_set_source_tag (task, valent_test_download);
  g_task_set_task_data (task,
                        json_node_ref (packet),
                        (GDestroyNotify)json_node_unref);
  g_task_run_in_thread (task, download_task);

  while (!g_task_get_completed (task))
    g_main_context_iteration (NULL, FALSE);

  return g_task_propagate_boolean (task, error);
}

/**
 * valent_test_upload:
 * @channel: a #ValentChannel
 * @packet: a #JsonNode
 * @file: a #GFile
 * @error: (nullable): a #GError
 *
 * Simulate uploading @file to the endpoint of @channel.
 */
gboolean
valent_test_upload (ValentChannel  *channel,
                    JsonNode       *packet,
                    GFile          *file,
                    GError        **error)
{
  g_autoptr (GTask) task = NULL;
  TransferOperation *op;

  g_assert (VALENT_IS_CHANNEL (channel));
  g_assert (VALENT_IS_PACKET (packet));
  g_assert (G_IS_FILE (file));
  g_assert (error == NULL || *error == NULL);

  op = g_new0 (TransferOperation, 1);
  g_rec_mutex_init (&op->lock);
  g_rec_mutex_lock (&op->lock);
  op->packet = json_node_ref (packet);
  op->file = g_object_ref (file);
  g_rec_mutex_unlock (&op->lock);

  task = g_task_new (channel, NULL, NULL, NULL);
  g_task_set_source_tag (task, valent_test_upload);
  g_task_set_task_data (task, op, transfer_op_free);
  g_task_run_in_thread (task, upload_task);

  while (!g_task_get_completed (task))
    g_main_context_iteration (NULL, FALSE);

  return g_task_propagate_boolean (task, error);
}

static void
valent_type_ensure (void)
{
  /* Core */
  g_type_ensure (VALENT_TYPE_DATA);
  g_type_ensure (VALENT_TYPE_OBJECT);
  g_type_ensure (VALENT_TYPE_COMPONENT);
  g_type_ensure (VALENT_TYPE_TRANSFER);

  g_type_ensure (VALENT_TYPE_CHANNEL);
  g_type_ensure (VALENT_TYPE_CHANNEL_SERVICE);
  g_type_ensure (VALENT_TYPE_DEVICE);
  g_type_ensure (VALENT_TYPE_DEVICE_MANAGER);
  g_type_ensure (VALENT_TYPE_DEVICE_PLUGIN);
  g_type_ensure (VALENT_TYPE_DEVICE_TRANSFER);

  /* Components */
  g_type_ensure (VALENT_TYPE_CLIPBOARD);
  g_type_ensure (VALENT_TYPE_CLIPBOARD_ADAPTER);
  g_type_ensure (VALENT_TYPE_CONTACTS);
  g_type_ensure (VALENT_TYPE_CONTACTS_ADAPTER);
  g_type_ensure (VALENT_TYPE_CONTACT_STORE);
  g_type_ensure (VALENT_TYPE_CONTACT_CACHE);
  g_type_ensure (VALENT_TYPE_INPUT);
  g_type_ensure (VALENT_TYPE_INPUT_ADAPTER);
  g_type_ensure (VALENT_TYPE_MEDIA);
  g_type_ensure (VALENT_TYPE_MEDIA_ADAPTER);
  g_type_ensure (VALENT_TYPE_MEDIA_PLAYER);
  g_type_ensure (VALENT_TYPE_MIXER);
  g_type_ensure (VALENT_TYPE_MIXER_ADAPTER);
  g_type_ensure (VALENT_TYPE_MIXER_STREAM);
  g_type_ensure (VALENT_TYPE_NOTIFICATIONS);
  g_type_ensure (VALENT_TYPE_NOTIFICATIONS_ADAPTER);
  g_type_ensure (VALENT_TYPE_NOTIFICATION);
  g_type_ensure (VALENT_TYPE_SESSION);
  g_type_ensure (VALENT_TYPE_SESSION_ADAPTER);

  /* UI */
  g_type_ensure (VALENT_TYPE_APPLICATION);
  g_type_ensure (VALENT_TYPE_APPLICATION_PLUGIN);

  g_type_ensure (VALENT_TYPE_DEVICE_GADGET);
  g_type_ensure (VALENT_TYPE_DEVICE_PREFERENCES_PAGE);
  g_type_ensure (VALENT_TYPE_PREFERENCES_PAGE);
}

/**
 * valent_test_init:
 * @argcp: Address of the `argc` parameter of the
 *        main() function. Changed if any arguments were handled.
 * @argvp: (inout) (array length=argcp): Address of the
 *        `argv` parameter of main().
 *        Any parameters understood by g_test_init() or gtk_init() are
 *        stripped before return.
 * @...: currently unused
 *
 * This function is used to initialize a GUI test program for Valent.
 *
 * In order, it will:
 * - Call g_content_type_set_mime_dirs() to ensure GdkPixbuf works
 * - Call g_test_init() with the %G_TEST_OPTION_ISOLATE_DIRS option
 * - Call g_type_ensure() for all public classes
 *
 * Like g_test_init(), any known arguments will be processed and stripped from
 * @argcp and @argvp.
 */
void
valent_test_init (int    *argcp,
                  char ***argvp,
                  ...)
{
  g_content_type_set_mime_dirs (NULL);
  g_test_init (argcp, argvp, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  valent_type_ensure ();
}

/**
 * valent_test_ui_init:
 * @argcp: Address of the `argc` parameter of the
 *        main() function. Changed if any arguments were handled.
 * @argvp: (inout) (array length=argcp): Address of the
 *        `argv` parameter of main().
 *        Any parameters understood by g_test_init() or gtk_init() are
 *        stripped before return.
 * @...: currently unused
 *
 * This function is used to initialize a GUI test program for Valent.
 *
 * In order, it will:
 * - Call g_content_type_set_mime_dirs() to ensure GdkPixbuf works
 * - Call g_test_init() with the %G_TEST_OPTION_ISOLATE_DIRS option
 * - Call g_type_ensure() for public classes
 * - Register GResources
 * - Set the locale to “en_US.UTF-8”
 * - Call gtk_init()
 * - Call adw_init()
 *
 * Like g_test_init(), any known arguments will be processed and stripped from
 * @argcp and @argvp.
 */
void
valent_test_ui_init (int    *argcp,
                     char ***argvp,
                     ...)
{
  g_content_type_set_mime_dirs (NULL);
  g_test_init (argcp, argvp, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  valent_type_ensure ();

  g_resources_register (valent_ui_get_resource ());

  gtk_disable_setlocale ();
  setlocale (LC_ALL, "en_US.UTF-8");
  gtk_init ();
  adw_init ();
}

