// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-ui.h>


static void
test_device_preferences_page_basic (void)
{
  PeasEngine *engine;
  PeasPluginInfo *info;
  PeasExtension *prefs;
  g_autofree char *device_id = NULL;
  PeasPluginInfo *plugin_info = NULL;
  g_autoptr (GSettings) settings = NULL;

  engine = valent_get_plugin_engine ();
  info = peas_engine_get_plugin_info (engine, "mock");
  prefs = peas_engine_create_extension (engine,
                                        info,
                                        VALENT_TYPE_DEVICE_PREFERENCES_PAGE,
                                        "device-id", "test-device",
                                        NULL);
  g_object_ref_sink (prefs);

  g_object_get (prefs,
                "device-id",   &device_id,
                "plugin-info", &plugin_info,
                "settings",    &settings,
                NULL);
  g_assert_cmpstr (device_id, ==, "test-device");
  g_assert_true (plugin_info == info);
  g_boxed_free (PEAS_TYPE_PLUGIN_INFO, plugin_info);
  g_assert_false (G_IS_SETTINGS (settings));

  g_object_unref (prefs);
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/libvalent/ui/device-preferences-page/basic",
                   test_device_preferences_page_basic);

  return g_test_run ();
}

