# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>


# Headers
libvalent_test_public_headers = [
  'libvalent-test.h',
  'valent-test-fixture.h',
  'valent-test-utils.h',
]

libvalent_test_private_headers = [
  'packetless-plugin.h',
  'valent-mock-application-plugin.h',
  'valent-mock-channel.h',
  'valent-mock-channel-service.h',
  'valent-mock-clipboard-adapter.h',
  'valent-mock-contacts-adapter.h',
  'valent-mock-device-gadget.h',
  'valent-mock-device-plugin.h',
  'valent-mock-device-preferences-page.h',
  'valent-mock-input-adapter.h',
  'valent-mock-media-adapter.h',
  'valent-mock-media-player.h',
  'valent-mock-mixer-adapter.h',
  'valent-mock-notifications-adapter.h',
  'valent-mock-preferences-page.h',
  'valent-mock-session-adapter.h',
]


# Sources
libvalent_test_public_sources = [
  'mock-plugin.c',
  'packetless-plugin.c',
  'valent-mock-application-plugin.c',
  'valent-mock-channel.c',
  'valent-mock-channel-service.c',
  'valent-mock-clipboard-adapter.c',
  'valent-mock-contacts-adapter.c',
  'valent-mock-device-gadget.c',
  'valent-mock-device-plugin.c',
  'valent-mock-device-preferences-page.c',
  'valent-mock-input-adapter.c',
  'valent-mock-media-adapter.c',
  'valent-mock-media-player.c',
  'valent-mock-mixer-adapter.c',
  'valent-mock-notifications-adapter.c',
  'valent-mock-preferences-page.c',
  'valent-mock-session-adapter.c',
  'valent-test-fixture.c',
  'valent-test-utils.c',
]


# Generated Resource Files
libvalent_test_resources = gnome.compile_resources('libvalent-test-resources',
                                                   'libvalent-test.gresource.xml',
  c_name: 'valent_test',
)
libvalent_test_generated_sources = [libvalent_test_resources]
libvalent_test_generated_headers = [libvalent_test_resources[1]]


# Dependencies
libvalent_test_deps = [
  libvalent_deps,
]

if get_option('fuzz_tests') and libwalbottle_dep.found()
  libvalent_c_args += ['-DHAVE_WALBOTTLE']
  libvalent_test_deps += [libwalbottle_dep]
endif


# Library Definitions
libvalent_test = static_library('valent-test-@0@'.format(libvalent_api_version),
                                libvalent_test_public_sources,
                                libvalent_test_generated_sources,
                                libvalent_test_generated_headers,
        c_args: libvalent_c_args + release_args + ['-DVALENT_TEST_COMPILATION'],
  dependencies: libvalent_test_deps,
)

libvalent_test_dep = declare_dependency(
              sources: [libvalent_test_private_headers, libvalent_test_generated_headers],
         dependencies: libvalent_test_deps,
            link_with: libvalent_test,
  include_directories: include_directories('.'),
)

