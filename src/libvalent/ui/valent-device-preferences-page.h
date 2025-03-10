// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#if !defined (VALENT_UI_INSIDE) && !defined (VALENT_UI_COMPILATION)
# error "Only <libvalent-ui.h> can be included directly."
#endif

#include <adwaita.h>
#include <libvalent-core.h>

G_BEGIN_DECLS

#define VALENT_TYPE_DEVICE_PREFERENCES_PAGE (valent_device_preferences_page_get_type ())

VALENT_AVAILABLE_IN_1_0
G_DECLARE_DERIVABLE_TYPE (ValentDevicePreferencesPage, valent_device_preferences_page, VALENT, DEVICE_PREFERENCES_PAGE, AdwPreferencesPage)

struct _ValentDevicePreferencesPageClass
{
  AdwPreferencesPageClass   parent_class;

  /*< private >*/
  gpointer                  padding[8];
};

VALENT_AVAILABLE_IN_1_0
GSettings * valent_device_preferences_page_get_settings (ValentDevicePreferencesPage *page);

G_END_DECLS

