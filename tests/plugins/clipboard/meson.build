# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

# Dependencies
plugin_clipboard_test_deps = [
  libvalent_test_dep,
  plugin_clipboard_deps,
]

plugin_clipboard_tests = [
  'test-clipboard-plugin',
  'test-clipboard-preferences',
]

foreach test : plugin_clipboard_tests
  test_program = executable(test, '@0@.c'.format(test),
                 c_args: test_c_args,
           dependencies: plugin_clipboard_test_deps,
    include_directories: plugin_clipboard_include_directories,
              link_args: test_link_args,
             link_whole: [libvalent_test, plugin_clipboard],
  )

  test(test, test_program,
            env: tests_env,
    is_parallel: false,
          suite: ['plugins', 'clipboard'],
  )
endforeach

