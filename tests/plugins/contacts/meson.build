# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

# Dependencies
plugin_contacts_test_deps = [
  libvalent_test_dep,
  plugin_contacts_deps,
]

plugin_contacts_tests = [
  'test-contacts-plugin',
  'test-contacts-preferences',
]

foreach test : plugin_contacts_tests
  test_program = executable(test, '@0@.c'.format(test),
                 c_args: test_c_args,
           dependencies: plugin_contacts_test_deps,
    include_directories: plugin_contacts_include_directories,
              link_args: test_link_args,
             link_whole: [libvalent_test, plugin_contacts],
  )

  test(test, test_program,
            env: tests_env,
    is_parallel: false,
          suite: ['plugins', 'contacts'],
  )
endforeach

