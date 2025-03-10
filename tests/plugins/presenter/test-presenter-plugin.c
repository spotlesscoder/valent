// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <gio/gio.h>
#include <libvalent-core.h>
#include <libvalent-test.h>


static void
test_presenter_plugin_basic (ValentTestFixture *fixture,
                             gconstpointer      user_data)
{
  GActionGroup *actions = G_ACTION_GROUP (fixture->device);

  g_assert_true (g_action_group_has_action (actions, "presenter.remote"));
  g_assert_true (g_action_group_has_action (actions, "presenter.pointer"));
}

static void
test_presenter_plugin_handle_request (ValentTestFixture *fixture,
                                      gconstpointer      user_data)
{
  JsonNode *packet;

  valent_test_fixture_connect (fixture, TRUE);

  /* Expect the multiplier of the motion deltas to be reasonable */
  packet = valent_test_fixture_lookup_packet (fixture, "presenter-motion1");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_event_cmpstr ("POINTER MOTION -100.0 -100.0");

  packet = valent_test_fixture_lookup_packet (fixture, "presenter-motion2");
  valent_test_fixture_handle_packet (fixture, packet);
  valent_test_event_cmpstr ("POINTER MOTION 100.0 100.0");
}

static void
test_presenter_plugin_send_request (ValentTestFixture *fixture,
                                    gconstpointer      user_data)
{
  GActionGroup *actions = G_ACTION_GROUP (fixture->device);
  JsonNode *packet = NULL;

  valent_test_fixture_connect (fixture, TRUE);

  g_assert_false (g_action_group_get_action_enabled (actions, "presenter.remote"));
  g_assert_true (g_action_group_get_action_enabled (actions, "presenter.pointer"));

  g_action_group_activate_action (actions,
                                  "presenter.pointer",
                                  g_variant_new ("(ddu)", 0.1, -0.1, 0));

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.presenter");
  v_assert_packet_cmpfloat (packet, "dx", ==, 0.1);
  v_assert_packet_cmpfloat (packet, "dy", ==, -0.1);
  g_clear_pointer (&packet, json_node_unref);

  g_action_group_activate_action (actions,
                                  "presenter.pointer",
                                  g_variant_new ("(ddu)", 0.0, 0.0, 1));

  packet = valent_test_fixture_expect_packet (fixture);
  v_assert_packet_type (packet, "kdeconnect.presenter");
  v_assert_packet_true (packet, "stop");
  g_clear_pointer (&packet, json_node_unref);
}

static const char *schemas[] = {
  JSON_SCHEMA_DIR"/kdeconnect.presenter.json",
};

static void
test_presenter_plugin_fuzz (ValentTestFixture *fixture,
                            gconstpointer      user_data)

{
  valent_test_fixture_connect (fixture, TRUE);
  g_test_log_set_fatal_handler (valent_test_mute_fuzzing, NULL);

  for (unsigned int s = 0; s < G_N_ELEMENTS (schemas); s++)
    valent_test_fixture_schema_fuzz (fixture, schemas[s]);
}

int
main (int   argc,
      char *argv[])
{
  const char *path = TEST_DATA_DIR"/plugin-presenter.json";

  valent_test_init (&argc, &argv, NULL);

  g_test_add ("/plugins/presenter/basic",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_presenter_plugin_basic,
              valent_test_fixture_clear);

  g_test_add ("/plugins/presenter/handle-request",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_presenter_plugin_handle_request,
              valent_test_fixture_clear);

  g_test_add ("/plugins/presenter/send-request",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_presenter_plugin_send_request,
              valent_test_fixture_clear);

  g_test_add ("/plugins/presenter/fuzz",
              ValentTestFixture, path,
              valent_test_fixture_init,
              test_presenter_plugin_fuzz,
              valent_test_fixture_clear);

  return g_test_run ();
}
