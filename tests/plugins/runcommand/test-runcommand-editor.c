// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-ui.h>

#include "valent-runcommand-editor.h"

static void
on_command_changed (ValentRuncommandEditor  *editor,
                    GParamSpec              *pspec,
                    GVariant               **out)
{
  if (out)
    *out = valent_runcommand_editor_get_command (editor);
}

static void
test_runcommand_dialog (void)
{
  g_autoptr (ValentRuncommandEditor) editor = NULL;
  g_autoptr (GVariant) command = NULL;
  g_autoptr (GVariant) test_command = NULL;
  g_autofree char *uuid = NULL;
  GVariant *out = NULL;

  test_command = g_variant_new_parsed ("{'name': <'%s'>, 'command': <'%s'>}",
                                       "Test Command", "echo \"foobar\"");
  g_variant_ref_sink (test_command);

  editor = g_object_new (VALENT_TYPE_RUNCOMMAND_EDITOR,
                         "uuid",    "test",
                         "command", test_command,
                         NULL);
  g_signal_connect (editor,
                    "notify::command",
                    G_CALLBACK (on_command_changed),
                    &out);
  gtk_window_present (GTK_WINDOW (editor));

  /* Properties */
  g_object_get (editor,
                "uuid",    &uuid,
                "command", &command,
                NULL);

  g_assert_true (command == test_command);
  g_assert_cmpstr (uuid, ==, "test");
  g_clear_pointer (&command, g_variant_unref);
  g_clear_pointer (&uuid, g_free);

  /* Cancel */
  gtk_widget_activate_action (GTK_WIDGET (editor), "editor.cancel", NULL);
  g_assert_true (out == test_command);
  g_assert_true (valent_runcommand_editor_get_command (editor) == test_command);
  g_assert_cmpstr (valent_runcommand_editor_get_uuid (editor), ==, "test");

  /* Save */
  gtk_widget_activate_action (GTK_WIDGET (editor), "editor.save", NULL);
  g_assert_true (out != test_command);
  g_assert_true (valent_runcommand_editor_get_command (editor) != test_command);
  g_assert_cmpstr (valent_runcommand_editor_get_uuid (editor), ==, "test");

  /* Remove */
  gtk_widget_activate_action (GTK_WIDGET (editor), "editor.remove", NULL);
  g_assert_true (out == NULL);
  g_assert_null (valent_runcommand_editor_get_command (editor));
  g_assert_cmpstr (valent_runcommand_editor_get_uuid (editor), ==, "test");
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/plugins/runcommand/dialog",
                   test_runcommand_dialog);

  return g_test_run ();
}

