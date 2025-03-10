// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#if !defined (VALENT_MEDIA_INSIDE) && !defined (VALENT_MEDIA_COMPILATION)
# error "Only <libvalent-media.h> can be included directly."
#endif

#include <gio/gio.h>
#include <libvalent-core.h>

#include "valent-media-player.h"

G_BEGIN_DECLS

#define VALENT_TYPE_MEDIA (valent_media_get_type ())

VALENT_AVAILABLE_IN_1_0
G_DECLARE_FINAL_TYPE (ValentMedia, valent_media, VALENT, MEDIA, ValentComponent)

VALENT_AVAILABLE_IN_1_0
ValentMedia * valent_media_get_default     (void);
VALENT_AVAILABLE_IN_1_0
void          valent_media_export_player   (ValentMedia       *media,
                                            ValentMediaPlayer *player);
VALENT_AVAILABLE_IN_1_0
void          valent_media_unexport_player (ValentMedia       *media,
                                            ValentMediaPlayer *player);
VALENT_AVAILABLE_IN_1_0
void          valent_media_pause           (ValentMedia       *media);
VALENT_AVAILABLE_IN_1_0
void          valent_media_unpause         (ValentMedia       *media);

G_END_DECLS

