// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-ui.h>


static int stage = 0;

static gboolean
basic_timeout_cb (gpointer data)
{
  GApplication *application = G_APPLICATION (data);

  g_application_quit (application);

  return G_SOURCE_REMOVE;
}

static void
test_application_basic (void)
{
  g_autoptr (ValentApplication) service = NULL;
  int ret = 0;

  service = _valent_application_new ();

  g_timeout_add_seconds (1, basic_timeout_cb, service);

  ret = g_application_run (G_APPLICATION (service), 0, NULL);

  g_assert_cmpint (ret, ==, 0);
}

static gboolean
actions_timeout_cb (gpointer data)
{
  GtkApplication *application = GTK_APPLICATION (data);
  GActionGroup *actions = G_ACTION_GROUP (data);
  GVariantBuilder builder;
  GVariant *target;

  switch (stage++)
    {
    case 0:
      target = g_variant_new_string ("main");
      g_action_group_activate_action (actions, "window", target);
      g_assert_nonnull (gtk_application_get_active_window (application));
      return G_SOURCE_CONTINUE;

    case 1:
      g_action_group_activate_action (actions, "refresh", NULL);
      return G_SOURCE_CONTINUE;

    case 2:
      g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
      target = g_variant_new ("(ssav)", "mock-device", "mock.echo", &builder);
      g_action_group_activate_action (actions, "device", target);
      return G_SOURCE_CONTINUE;

    case 3:
      g_action_group_activate_action (actions, "quit", NULL);
      return G_SOURCE_REMOVE;

    default:
      return G_SOURCE_REMOVE;
    }
}

static void
test_application_actions (void)
{
  g_autoptr (ValentApplication) service = NULL;
  int ret = 0;

  service = _valent_application_new ();

  stage = 0;
  g_timeout_add_seconds (1, actions_timeout_cb, service);

  ret = g_application_run (G_APPLICATION (service), 0, NULL);

  g_assert_cmpint (ret, ==, 0);
}

static gboolean
plugins_timeout_cb (gpointer data)
{
  GApplication *application = G_APPLICATION (data);
  g_autoptr (GSettings) settings = NULL;
  PeasEngine *engine;
  PeasPluginInfo *info;

  engine = valent_get_plugin_engine ();
  info = peas_engine_get_plugin_info (engine, "mock");
  settings = valent_component_create_settings ("application", "mock");

  switch (stage++)
    {
    case 0:
      peas_engine_unload_plugin (engine, info);
      return G_SOURCE_CONTINUE;

    case 1:
      peas_engine_load_plugin (engine, info);
      return G_SOURCE_CONTINUE;

    case 2:
      g_settings_set_boolean (settings, "enabled", FALSE);
      return G_SOURCE_CONTINUE;

    case 3:
      g_settings_set_boolean (settings, "enabled", TRUE);
      return G_SOURCE_CONTINUE;

    default:
      g_application_quit (application);
      return G_SOURCE_REMOVE;
    }
}

static void
test_application_plugins (void)
{
  g_autoptr (ValentApplication) service = NULL;
  int ret = 0;

  service = _valent_application_new ();

  stage = 0;
  g_timeout_add_seconds (1, plugins_timeout_cb, service);

  ret = g_application_run (G_APPLICATION (service), 0, NULL);

  g_assert_cmpint (ret, ==, 0);
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_set_application_name ("Valent");

  g_test_add_func ("/libvalent/ui/application/basic",
                   test_application_basic);

  g_test_add_func ("/libvalent/ui/application/actions",
                   test_application_actions);

  g_test_add_func ("/libvalent/ui/application/plugins",
                   test_application_plugins);

  return g_test_run ();
}
