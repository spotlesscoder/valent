// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include "config.h"

#include <libpeas/peas.h>
#include <libvalent-device.h>
#include <libvalent-notifications.h>

#include "valent-notification-plugin.h"
#include "valent-notification-preferences.h"


G_MODULE_EXPORT void
valent_notification_plugin_register_types (PeasObjectModule *module)
{
  peas_object_module_register_extension_type (module,
                                              VALENT_TYPE_DEVICE_PLUGIN,
                                              VALENT_TYPE_NOTIFICATION_PLUGIN);

  peas_object_module_register_extension_type (module,
                                              VALENT_TYPE_DEVICE_PREFERENCES_PAGE,
                                              VALENT_TYPE_NOTIFICATION_PREFERENCES);
}

