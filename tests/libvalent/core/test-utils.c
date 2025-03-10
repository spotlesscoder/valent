// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <libvalent-core.h>
#include <libvalent-test.h>


static void
test_utils_version (void)
{
  unsigned int major, minor, micro;

  major = valent_get_major_version ();
  minor = valent_get_minor_version ();
  micro = valent_get_micro_version ();

  g_assert_cmpuint (major, ==, VALENT_MAJOR_VERSION);
  g_assert_cmpuint (minor, ==, VALENT_MINOR_VERSION);
  g_assert_cmpuint (micro, ==, VALENT_MICRO_VERSION);

  g_assert_true (valent_check_version (major, minor));
  g_assert_false (valent_check_version (major + 1, minor));
  g_assert_false (valent_check_version (major, minor + 1));
}

int
main (int   argc,
      char *argv[])
{
  valent_test_init (&argc, &argv, NULL);

  g_test_add_func ("/libvalent/core/utils/version",
                   test_utils_version);

  g_test_run ();
}
