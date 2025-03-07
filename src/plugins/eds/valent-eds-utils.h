// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#include <libvalent-contacts.h>

G_BEGIN_DECLS

ESourceRegistry * valent_eds_get_registry    (GCancellable  *cancellable,
                                              GError       **error);
ESource         * valent_eds_register_source (ESource       *scratch,
                                              GCancellable  *cancellable,
                                              GError       **error);

G_END_DECLS
