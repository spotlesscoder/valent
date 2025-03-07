// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-test.h>
#include <libvalent-ui.h>

#include "valent-device-private.h"
#include "valent-window.h"


typedef struct
{
  ValentDeviceManager *manager;
} TestWindowFixture;

static void
test_window_set_up (TestWindowFixture *fixture,
                    gconstpointer      user_data)
{
  fixture->manager = valent_device_manager_new_sync (NULL, NULL);
  valent_device_manager_start (fixture->manager);
}

static void
test_window_tear_down (TestWindowFixture *fixture,
                       gconstpointer      user_data)
{
  valent_device_manager_stop (fixture->manager);
  v_assert_finalize_object (fixture->manager);
}

static void
test_window_basic (TestWindowFixture *fixture,
                   gconstpointer      user_data)
{
  g_autoptr (ValentDeviceManager) manager = NULL;
  GtkWindow *window;

  window = g_object_new (VALENT_TYPE_WINDOW,
                         "device-manager", fixture->manager,
                         NULL);
  g_assert_true (VALENT_IS_WINDOW (window));

  /* Show the window */
  gtk_window_present (window);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Properties */
  g_object_get (window,
                "device-manager", &manager,
                NULL);
  g_assert_true (fixture->manager == manager);
  g_clear_object (&manager);

  g_clear_pointer (&window, gtk_window_destroy);
}

VALENT_NO_ASAN static void
test_window_navigation (TestWindowFixture *fixture,
                        gconstpointer      user_data)
{
  GtkWindow *window;
  g_autoptr (ValentDevice) device = NULL;

  window = g_object_new (VALENT_TYPE_WINDOW,
                         "device-manager", fixture->manager,
                         NULL);
  g_assert_true (VALENT_IS_WINDOW (window));

  /* Show the window */
  gtk_window_present (window);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Refresh */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.refresh", NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Main -> Device -> Main */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.page", "s", "mock-device");
  gtk_widget_activate_action (GTK_WIDGET (window), "win.previous", NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Main -> Device -> Remove Device */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.page", "s", "mock-device");

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  device = g_list_model_get_item (G_LIST_MODEL (fixture->manager), 0);
  valent_device_set_channel (device, NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  g_clear_pointer (&window, gtk_window_destroy);
}

static void
test_window_dialogs (TestWindowFixture *fixture,
                     gconstpointer      user_data)
{
  GtkWindow *window;

  /* Preferences */
  window = g_object_new (VALENT_TYPE_WINDOW,
                         "device-manager", fixture->manager,
                         NULL);
  g_assert_true (VALENT_IS_WINDOW (window));

  gtk_window_present (window);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Changing the page closes the preferences */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.preferences", NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  gtk_widget_activate_action (GTK_WIDGET (window), "win.page", "s", "main");

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  /* Closing the window closed the preferences */
  gtk_widget_activate_action (GTK_WIDGET (window), "win.preferences", NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  gtk_widget_activate_action (GTK_WIDGET (window), "win.page", "s", "main");

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  g_clear_pointer (&window, gtk_window_destroy);

  /* About */
  window = g_object_new (VALENT_TYPE_WINDOW,
                         "device-manager", fixture->manager,
                         NULL);
  g_assert_true (VALENT_IS_WINDOW (window));

  gtk_window_present (window);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  gtk_widget_activate_action (GTK_WIDGET (window), "win.about", NULL);

  while (g_main_context_iteration (NULL, FALSE))
    continue;

  g_clear_pointer (&window, gtk_window_destroy);
}

int
main (int argc,
     char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add ("/libvalent/ui/window/basic",
              TestWindowFixture, NULL,
              test_window_set_up,
              test_window_basic,
              test_window_tear_down);

  g_test_add ("/libvalent/ui/window/navigation",
              TestWindowFixture, NULL,
              test_window_set_up,
              test_window_navigation,
              test_window_tear_down);

  g_test_add ("/libvalent/ui/window/dialogs",
              TestWindowFixture, NULL,
              test_window_set_up,
              test_window_dialogs,
              test_window_tear_down);

  return g_test_run ();
}

