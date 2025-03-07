// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-ui.h>


static void
test_preferences_page_basic (void)
{
  PeasEngine *engine;
  PeasPluginInfo *info;
  PeasExtension *prefs;
  PeasPluginInfo *plugin_info = NULL;

  engine = valent_get_plugin_engine ();
  info = peas_engine_get_plugin_info (engine, "mock");
  prefs = peas_engine_create_extension (engine,
                                        info,
                                        VALENT_TYPE_PREFERENCES_PAGE,
                                        NULL);
  g_object_ref_sink (prefs);

  g_object_get (prefs,
                "plugin-info", &plugin_info,
                NULL);
  g_assert_true (plugin_info == info);
  g_boxed_free (PEAS_TYPE_PLUGIN_INFO, plugin_info);

  g_object_unref (prefs);
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/libvalent/ui/preferences-page/basic",
                   test_preferences_page_basic);

  return g_test_run ();
}

