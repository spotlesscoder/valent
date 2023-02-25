// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#include "valent-component.h"

G_BEGIN_DECLS

PeasExtension * valent_component_get_preferred   (ValentComponent *self);
GSettings     * valent_component_create_settings (const char      *context,
                                                  const char      *module_name);

G_END_DECLS

