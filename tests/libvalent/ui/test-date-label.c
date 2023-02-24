// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#include <gdk/gdk.h>
#include <libvalent-core.h>
#include <libvalent-test.h>
#include <libvalent-ui.h>

#include "libvalent-ui-private.h"


static void
test_sms_date_label (void)
{
  GtkWidget *window;
  GtkWidget *label;
  gint64 date = 123456789;
  gint64 date_out;
  unsigned int format = 1;
  unsigned int format_out;

  /* Construction */
  label = valent_date_label_new (date);
  g_object_set (G_OBJECT (label), "format", format, NULL);

  /* Properties */
  g_object_get (label,
                "date", &date_out,
                "format", &format_out,
                NULL);

  g_assert_cmpint (date, ==, date_out);
  g_assert_cmpuint (format, ==, format_out);

  /* Display */
  window = gtk_window_new ();
  gtk_window_set_child (GTK_WINDOW (window), label);

  gtk_window_present (GTK_WINDOW (window));
  gtk_window_destroy (GTK_WINDOW (window));
}

int
main (int   argc,
      char *argv[])
{
  valent_test_ui_init (&argc, &argv, NULL);

  g_test_add_func ("/libvalent/ui/date-label",
                   test_sms_date_label);

  return g_test_run ();
}

