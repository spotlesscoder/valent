// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#include <libvalent-device.h>

G_BEGIN_DECLS

#define VALENT_TYPE_SHARE_PLUGIN (valent_share_plugin_get_type())

G_DECLARE_FINAL_TYPE (ValentSharePlugin, valent_share_plugin, VALENT, SHARE_PLUGIN, ValentDevicePlugin)

G_END_DECLS

