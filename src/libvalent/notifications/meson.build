# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

libvalent_notifications_header_dir = join_paths(libvalent_header_dir, 'notifications')
libvalent_notifications_header_subdir = join_paths(libvalent_header_subdir, 'notifications')
libvalent_include_directories += include_directories('.')
libvalent_notifications_generated_headers = []
libvalent_notifications_generated_sources = []


# Headers
libvalent_notifications_public_headers = [
  'libvalent-notifications.h',
  'valent-notification.h',
  'valent-notifications.h',
  'valent-notifications-adapter.h',
]

libvalent_notifications_private_headers = [
]

install_headers(libvalent_notifications_public_headers,
  subdir: libvalent_notifications_header_subdir,
)


# Sources
libvalent_notifications_public_sources = [
  'valent-notification.c',
  'valent-notifications.c',
  'valent-notifications-adapter.c',
]


# Dependencies
libvalent_notifications_deps = [
  libvalent_core_dep,
]


# Library Definitions
libvalent_notifications = static_library('valent-notifications-@0@'.format(libvalent_api_version),
                                         libvalent_notifications_public_sources,
                                         libvalent_notifications_generated_sources,
                                         libvalent_notifications_generated_headers,
        c_args: libvalent_c_args + release_args + ['-DVALENT_NOTIFICATIONS_COMPILATION'],
  dependencies: libvalent_notifications_deps,
)

libvalent_notifications_dep = declare_dependency(
              sources: [libvalent_notifications_generated_headers, libvalent_notifications_private_headers],
         dependencies: libvalent_notifications_deps,
            link_with: libvalent_notifications,
  include_directories: include_directories('.'),
)


libvalent_static += [libvalent_notifications]
libvalent_deps += [libvalent_notifications_dep]
libvalent_public_sources += files(libvalent_notifications_public_sources)
libvalent_public_headers += files(libvalent_notifications_public_headers)
libvalent_private_headers += files(libvalent_notifications_private_headers)
libvalent_generated_headers += libvalent_notifications_generated_headers
libvalent_generated_sources += libvalent_notifications_generated_sources
libvalent_pkgconfig_subdirs += libvalent_notifications_header_subdir
libvalent_gir_extra_args += ['--c-include=libvalent-notifications.h', '-DVALENT_NOTIFICATIONS_COMPILATION']

