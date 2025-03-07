// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-runcommand-plugin"

#include "config.h"

#include <glib/gi18n.h>
#include <json-glib/json-glib.h>
#include <libportal/portal.h>
#include <libvalent-core.h>
#include <libvalent-device.h>

#include "valent-runcommand-plugin.h"
#include "valent-runcommand-utils.h"


struct _ValentRuncommandPlugin
{
  ValentDevicePlugin   parent_instance;

  GSubprocessLauncher *launcher;
  GHashTable          *subprocesses;

  unsigned long        commands_changed_id;
};

G_DEFINE_FINAL_TYPE (ValentRuncommandPlugin, valent_runcommand_plugin, VALENT_TYPE_DEVICE_PLUGIN)

static void valent_runcommand_plugin_execute_local_command     (ValentRuncommandPlugin *self,
                                                                const char             *key);
static void valent_runcommand_plugin_execute_remote_command    (ValentRuncommandPlugin *self,
                                                                const char             *key);
static void valent_runcommand_plugin_handle_command_list       (ValentRuncommandPlugin *self,
                                                                JsonObject             *command_list);
static void valent_runcommand_plugin_handle_runcommand         (ValentRuncommandPlugin *self,
                                                                JsonNode               *packet);
static void valent_runcommand_plugin_handle_runcommand_request (ValentRuncommandPlugin *self,
                                                                JsonNode               *packet);
static void valent_runcommand_plugin_send_command_list         (ValentRuncommandPlugin *self);


/*
 * Launcher Helpers
 */
static void
launcher_init (ValentRuncommandPlugin *self)
{
  ValentDevice *device;
  GSubprocessFlags flags;

  if G_UNLIKELY (self->launcher != NULL)
    return;

#ifdef VALENT_ENABLE_DEBUG
  flags = G_SUBPROCESS_FLAGS_NONE;
#else
  flags = (G_SUBPROCESS_FLAGS_STDERR_SILENCE |
           G_SUBPROCESS_FLAGS_STDOUT_SILENCE);
#endif /* VALENT_ENABLE_DEBUG */

  self->launcher = g_subprocess_launcher_new (flags);

  device = valent_device_plugin_get_device (VALENT_DEVICE_PLUGIN (self));
  g_subprocess_launcher_setenv (self->launcher,
                                "VALENT_DEVICE_ID",
                                valent_device_get_id (device),
                                TRUE);
  g_subprocess_launcher_setenv (self->launcher,
                                "VALENT_DEVICE_NAME",
                                valent_device_get_name (device),
                                TRUE);
}

static void
launcher_clear (ValentRuncommandPlugin *self)
{
  GHashTableIter iter;
  gpointer subprocess;

  g_hash_table_iter_init (&iter, self->subprocesses);

  while (g_hash_table_iter_next (&iter, NULL, &subprocess))
    {
      g_subprocess_force_exit (G_SUBPROCESS (subprocess));
      g_hash_table_iter_remove (&iter);
    }

  g_clear_object (&self->launcher);
}

static void
launcher_watch (GSubprocess  *subprocess,
                GAsyncResult *result,
                GHashTable   *subprocesses)
{
  g_autoptr (GError) error = NULL;

  if (!g_subprocess_wait_finish (subprocess, result, &error) &&
      !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_warning ("Process failed: %s", error->message);

  g_hash_table_remove (subprocesses, subprocess);
  g_hash_table_unref (subprocesses);
}

static gboolean
launcher_execute (ValentRuncommandPlugin  *self,
                  const char              *command,
                  GError                 **error)
{
  g_autoptr (GSubprocess) subprocess = NULL;
  g_autoptr (GString) command_line = NULL;
  g_autofree char *quoted = NULL;
  g_auto (GStrv) argv = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_assert (command != NULL && *command != '\0');
  g_assert (error == NULL || *error == NULL);

  launcher_init (self);

  command_line = g_string_new ("");

  /* When running in a Flatpak, run the command on the host if requested */
  if (xdp_portal_running_under_flatpak () &&
      valent_runcommand_can_spawn_host ())
    {
      GSettings *settings;

      settings = valent_device_plugin_get_settings (VALENT_DEVICE_PLUGIN (self));

      if (!g_settings_get_boolean (settings, "isolate-subprocesses"))
        g_string_append (command_line, "flatpak-spawn --host ");
    }

  /* Quote the command for the subshell */
  quoted = g_shell_quote (command);
  g_string_append_printf (command_line, "sh -c %s", quoted);

  if (!g_shell_parse_argv (command_line->str, NULL, &argv, error))
    return FALSE;

  subprocess = g_subprocess_launcher_spawnv (self->launcher,
                                             (const char * const *)argv,
                                             error);

  if (subprocess == NULL)
    return FALSE;

  /* The task holds the final reference to the GSubprocess object */
  g_subprocess_wait_async (subprocess,
                           NULL,
                           (GAsyncReadyCallback)launcher_watch,
                           g_hash_table_ref (self->subprocesses));
  g_hash_table_add (self->subprocesses, subprocess);

  return TRUE;
}

/*
 * Local Commands
 */
static void
on_commands_changed (GSettings              *settings,
                     const char             *key,
                     ValentRuncommandPlugin *self)
{
  ValentDevice *device;
  ValentDeviceState state;

  g_assert (G_IS_SETTINGS (settings));
  g_assert (key != NULL);
  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));

  device = valent_device_plugin_get_device (VALENT_DEVICE_PLUGIN (self));
  state = valent_device_get_state (device);

  if ((state & VALENT_DEVICE_STATE_CONNECTED) != 0 &&
      (state & VALENT_DEVICE_STATE_PAIRED) != 0)
    valent_runcommand_plugin_send_command_list (self);
}

static void
valent_runcommand_plugin_execute_local_command (ValentRuncommandPlugin *self,
                                                const char             *key)
{
  GSettings *settings;
  g_autoptr (GVariant) commands = NULL;
  g_autoptr (GVariant) command = NULL;
  const char *command_str;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_return_if_fail (key != NULL);

  /* Lookup the command by UUID */
  settings = valent_device_plugin_get_settings (VALENT_DEVICE_PLUGIN (self));
  commands = g_settings_get_value (settings, "commands");

  if (!g_variant_lookup (commands, key, "@a{sv}", &command))
    return valent_runcommand_plugin_send_command_list (self);

  /* Lookup the command line */
  if (g_variant_lookup (command, "command", "&s", &command_str))
    {
      g_autoptr (GError) error = NULL;

      if (!launcher_execute (self, command_str, &error))
        {
          g_warning ("%s(): spawning \"%s\": %s",
                     G_STRFUNC,
                     command_str,
                     error->message);
        }
    }
}

static void
valent_runcommand_plugin_send_command_list (ValentRuncommandPlugin *self)
{
  GSettings *settings;
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) packet = NULL;
  g_autoptr (GVariant) commands = NULL;
  g_autoptr (JsonBuilder) commands_builder = NULL;
  g_autoptr (JsonNode) commands_node = NULL;
  g_autofree char *commands_json = NULL;
  GVariantIter iter;
  char *key;
  GVariant *value;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));

  settings = valent_device_plugin_get_settings (VALENT_DEVICE_PLUGIN (self));
  commands = g_settings_get_value (settings, "commands");

  /* The `commandList` dictionary is sent as a string of serialized JSON */
  commands_builder = json_builder_new ();
  json_builder_begin_object (commands_builder);

  g_variant_iter_init (&iter, commands);

  while (g_variant_iter_next (&iter, "{sv}", &key, &value))
    {
      const char *name = NULL;
      const char *command = NULL;

      if (g_variant_lookup (value, "name", "&s", &name) &&
          g_variant_lookup (value, "command", "&s", &command))
        {
          json_builder_set_member_name (commands_builder, key);
          json_builder_begin_object (commands_builder);
          json_builder_set_member_name (commands_builder, "name");
          json_builder_add_string_value (commands_builder, name);
          json_builder_set_member_name (commands_builder, "command");
          json_builder_add_string_value (commands_builder, command);
          json_builder_end_object (commands_builder);
        }

      g_clear_pointer (&key, g_free);
      g_clear_pointer (&value, g_variant_unref);
    }

  json_builder_end_object (commands_builder);
  commands_node = json_builder_get_root (commands_builder);
  commands_json = json_to_string (commands_node, FALSE);

  valent_packet_init (&builder, "kdeconnect.runcommand");
  json_builder_set_member_name (builder, "commandList");
  json_builder_add_string_value (builder, commands_json);
  packet = valent_packet_end (&builder);

  valent_device_plugin_queue_packet (VALENT_DEVICE_PLUGIN (self), packet);
}

static void
valent_runcommand_plugin_handle_runcommand_request (ValentRuncommandPlugin *self,
                                                    JsonNode               *packet)
{
  const char *key;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_assert (VALENT_IS_PACKET (packet));

  /* A request for the local command list */
  if (valent_packet_check_field (packet, "requestCommandList"))
    valent_runcommand_plugin_send_command_list (self);

  /* A request to execute a local command */
  if (valent_packet_get_string (packet, "key", &key))
    valent_runcommand_plugin_execute_local_command (self, key);
}

/*
 * Remote Commands
 */
static void
valent_runcommand_plugin_execute_remote_command (ValentRuncommandPlugin *self,
                                                 const char             *key)
{
  g_autoptr (JsonBuilder) builder = NULL;
  g_autoptr (JsonNode) packet = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_assert (key != NULL);

  valent_packet_init (&builder, "kdeconnect.runcommand.request");
  json_builder_set_member_name (builder, "key");
  json_builder_add_string_value (builder, key);
  packet = valent_packet_end (&builder);

  valent_device_plugin_queue_packet (VALENT_DEVICE_PLUGIN (self), packet);
}

static void
valent_runcommand_plugin_handle_command_list (ValentRuncommandPlugin *self,
                                              JsonObject             *command_list)
{
  JsonObjectIter iter;
  const char *key;
  JsonNode *command_node;
  g_autoptr (GMenuItem) cmd_item = NULL;
  g_autoptr (GIcon) cmd_icon = NULL;
  g_autoptr (GMenu) cmd_menu = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_assert (command_list != NULL);

  cmd_menu = g_menu_new ();
  cmd_item = g_menu_item_new_submenu (_("Run Command"), G_MENU_MODEL (cmd_menu));
  cmd_icon = g_themed_icon_new ("system-run-symbolic");
  g_menu_item_set_icon (cmd_item, cmd_icon);

  /* Iterate the commands */
  json_object_iter_init (&iter, command_list);

  while (json_object_iter_next (&iter, &key, &command_node))
    {
      JsonObject *cmd;
      const char *name;
      const char *command;
      g_autofree char *action = NULL;
      g_autoptr (GMenuItem) item = NULL;

      cmd = json_node_get_object (command_node);
      name = json_object_get_string_member (cmd, "name");
      command = json_object_get_string_member (cmd, "command");
      action = g_strdup_printf ("device.runcommand.execute::%s", key);

      item = g_menu_item_new (name, action);
      g_menu_item_set_attribute (item, "command", "s", command);
      g_menu_append_item (cmd_menu, item);
    }

  valent_device_plugin_replace_menu_item (VALENT_DEVICE_PLUGIN (self),
                                          cmd_item,
                                          "icon");
}

static void
valent_runcommand_plugin_handle_runcommand (ValentRuncommandPlugin *self,
                                            JsonNode               *packet)
{
  JsonObject *body;
  g_autoptr (JsonNode) command_node = NULL;
  const char *command_json;
  JsonObject *command_list;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));
  g_assert (VALENT_IS_PACKET (packet));

  body = valent_packet_get_body (packet);
  command_json = json_object_get_string_member_with_default (body, "commandList", "{}");
  command_node = json_from_string (command_json, NULL);

  if (command_node == NULL || !JSON_NODE_HOLDS_OBJECT (command_node))
    {
      g_warning ("%s(): malformed commandList field", G_STRFUNC);
      return;
    }

  command_list = json_node_get_object (command_node);
  valent_runcommand_plugin_handle_command_list (self, command_list);
}

/*
 * GActions
 */
static void
runcommand_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  ValentRuncommandPlugin *self = VALENT_RUNCOMMAND_PLUGIN (user_data);
  const char *key;

  g_return_if_fail (VALENT_IS_RUNCOMMAND_PLUGIN (self));

  key = g_variant_get_string (parameter, NULL);
  valent_runcommand_plugin_execute_remote_command (self, key);
}

static const GActionEntry actions[] = {
    {"execute", runcommand_action, "s", NULL, NULL}
};

/**
 * ValentDevicePlugin
 */
static void
valent_runcommand_plugin_enable (ValentDevicePlugin *plugin)
{
  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (plugin));

  g_action_map_add_action_entries (G_ACTION_MAP (plugin),
                                   actions,
                                   G_N_ELEMENTS (actions),
                                   plugin);
}

static void
valent_runcommand_plugin_disable (ValentDevicePlugin *plugin)
{
  ValentRuncommandPlugin *self = VALENT_RUNCOMMAND_PLUGIN (plugin);
  GSettings *settings;

  /* Stop watching for command changes */
  settings = valent_device_plugin_get_settings (plugin);
  g_clear_signal_handler (&self->commands_changed_id, settings);
}

static void
valent_runcommand_plugin_update_state (ValentDevicePlugin *plugin,
                                       ValentDeviceState   state)
{
  ValentRuncommandPlugin *self = VALENT_RUNCOMMAND_PLUGIN (plugin);
  GSettings *settings;
  gboolean available;

  g_assert (VALENT_IS_RUNCOMMAND_PLUGIN (self));

  available = (state & VALENT_DEVICE_STATE_CONNECTED) != 0 &&
              (state & VALENT_DEVICE_STATE_PAIRED) != 0;

  valent_device_plugin_toggle_actions (plugin, available);

  settings = valent_device_plugin_get_settings (plugin);

  if (available)
    {
      if (self->commands_changed_id == 0)
        {
          self->commands_changed_id =
            g_signal_connect (settings,
                              "changed::commands",
                              G_CALLBACK (on_commands_changed),
                              self);
        }

      valent_runcommand_plugin_send_command_list (self);
    }
  else
    {
      g_clear_signal_handler (&self->commands_changed_id, settings);
    }

  /* If the device is unpaired it is no longer trusted */
  if ((state & VALENT_DEVICE_STATE_PAIRED) == 0)
    launcher_clear (self);
}

static void
valent_runcommand_plugin_handle_packet (ValentDevicePlugin *plugin,
                                        const char         *type,
                                        JsonNode           *packet)
{
  ValentRuncommandPlugin *self = VALENT_RUNCOMMAND_PLUGIN (plugin);

  g_assert (VALENT_IS_DEVICE_PLUGIN (plugin));
  g_assert (type != NULL);
  g_assert (VALENT_IS_PACKET (packet));

  /* A request for the local command list or local execution */
  if (strcmp (type, "kdeconnect.runcommand.request") == 0)
    valent_runcommand_plugin_handle_runcommand_request (self, packet);

  /* A response to a request for the remote command list */
  else if (strcmp (type, "kdeconnect.runcommand") == 0)
    valent_runcommand_plugin_handle_runcommand (self, packet);

  else
    g_assert_not_reached ();
}

/*
 * GObject
 */
static void
valent_runcommand_plugin_finalize (GObject *object)
{
  ValentRuncommandPlugin *self = VALENT_RUNCOMMAND_PLUGIN (object);

  g_clear_object (&self->launcher);
  g_clear_pointer (&self->subprocesses, g_hash_table_unref);

  G_OBJECT_CLASS (valent_runcommand_plugin_parent_class)->finalize (object);
}

static void
valent_runcommand_plugin_class_init (ValentRuncommandPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ValentDevicePluginClass *plugin_class = VALENT_DEVICE_PLUGIN_CLASS (klass);

  object_class->finalize = valent_runcommand_plugin_finalize;

  plugin_class->enable = valent_runcommand_plugin_enable;
  plugin_class->disable = valent_runcommand_plugin_disable;
  plugin_class->handle_packet = valent_runcommand_plugin_handle_packet;
  plugin_class->update_state = valent_runcommand_plugin_update_state;
}

static void
valent_runcommand_plugin_init (ValentRuncommandPlugin *self)
{
  self->subprocesses = g_hash_table_new (NULL, NULL);
}

