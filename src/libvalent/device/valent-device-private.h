// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#include "valent-device.h"

G_BEGIN_DECLS

ValentDevice * valent_device_new_full      (JsonNode      *identity,
                                            ValentData    *data);
void           valent_device_handle_packet (ValentDevice  *device,
                                            JsonNode      *packet);
void           valent_device_set_channel   (ValentDevice  *device,
                                            ValentChannel *channel);
void           valent_device_set_paired    (ValentDevice  *device,
                                            gboolean       paired);

G_END_DECLS
