// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include "config.h"

#include <libpeas/peas.h>
#include <libvalent-device.h>

#include "valent-bluez-channel-service.h"


G_MODULE_EXPORT void
valent_bluez_plugin_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              VALENT_TYPE_CHANNEL_SERVICE,
                                              VALENT_TYPE_BLUEZ_CHANNEL_SERVICE);
}
