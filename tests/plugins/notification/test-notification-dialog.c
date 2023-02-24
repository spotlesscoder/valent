// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-notifications.h>
#include <libvalent-ui.h>

#include "valent-notification-dialog.h"

static void
on_destroy (GtkWindow *window,
            gboolean  *destroyed)
{
  if (destroyed)
    *destroyed = TRUE;
}

static void
test_notification_dialog (void)
{
  ValentNotificationDialog *dialog = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (ValentNotification) notification = NULL;
  g_autoptr (ValentNotification) notification_out = NULL;
  g_autofree char *reply_id = NULL;
  g_autofree char *reply_id_out = NULL;
  gboolean destroyed = FALSE;

  /* Mock Notification */
  icon = g_themed_icon_new ("phone-symbolic");
  notification = g_object_new (VALENT_TYPE_NOTIFICATION,
                               "icon",  icon,
                               "title", "Mock Title",
                               "body",  "Mock Body",
                               NULL);
  reply_id = g_uuid_string_random ();

  /* Open dialog */
  dialog = g_object_new (VALENT_TYPE_NOTIFICATION_DIALOG,
                         "notification",   notification,
                         "reply-id",       reply_id,
                         NULL);
  g_signal_connect (dialog,
                    "destroy",
                    G_CALLBACK (on_destroy),
                    &destroyed);
  gtk_window_present (GTK_WINDOW (dialog));

  /* Properties */
  g_object_get (dialog,
                "notification", &notification_out,
                "reply-id",     &reply_id_out,
                NULL);
  g_assert_true (notification == notification_out);
  g_assert_true (notification == valent_notification_dialog_get_notification (dialog));
  g_assert_cmpstr (reply_id, ==, reply_id_out);
  g_assert_cmpstr (reply_id, ==, valent_notification_dialog_get_reply_id (dialog));

  valent_notification_dialog_set_reply_id (dialog, NULL);
  g_assert_null (valent_notification_dialog_get_reply_id (dialog));

  /* Actions */
  gtk_widget_activate_action (GTK_WIDGET (dialog), "dialog.cancel", NULL);

  while (!destroyed)
    g_main_context_iteration (NULL, FALSE);

  g_assert_true (destroyed);
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/plugins/notification/dialog",
                   test_notification_dialog);

  return g_test_run ();
}

