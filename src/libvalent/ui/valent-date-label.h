// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#if !defined (VALENT_UI_INSIDE) && !defined (VALENT_UI_COMPILATION)
# error "Only <libvalent-ui.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VALENT_TYPE_DATE_LABEL (valent_date_label_get_type())

G_DECLARE_FINAL_TYPE (ValentDateLabel, valent_date_label, VALENT, DATE_LABEL, GtkWidget)

GtkWidget * valent_date_label_new          (gint64           timestamp);
gint64      valent_date_label_get_date     (ValentDateLabel *label);
void        valent_date_label_set_date     (ValentDateLabel *label,
                                            gint64           timestamp);
guint       valent_date_label_get_format   (ValentDateLabel *label);
void        valent_date_label_set_format   (ValentDateLabel *label,
                                            unsigned int     format);

char      * valent_date_label_string       (gint64           timestamp);
char      * valent_date_label_string_short (gint64           timestamp);

G_END_DECLS
