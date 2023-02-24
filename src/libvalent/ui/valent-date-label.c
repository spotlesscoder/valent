// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-date-label"

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "valent-date-label.h"


struct _ValentDateLabel
{
  GtkWidget     parent_instance;

  GtkWidget    *label;
  gint64        date;
  unsigned int  format;
};

G_DEFINE_FINAL_TYPE (ValentDateLabel, valent_date_label, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_DATE,
  PROP_FORMAT,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };


/*
 * Label Auto-Update
 */
static GPtrArray *label_cache = NULL;
static unsigned int label_source = 0;

static void
valent_date_label_update (ValentDateLabel *self)
{
  g_autofree char *text = NULL;

  g_assert (VALENT_IS_DATE_LABEL (self));

  if (self->format == 0)
    text = valent_date_label_string_short (self->date);
  else
    text = valent_date_label_string (self->date);

  gtk_label_set_label (GTK_LABEL (self->label), text);
}

static gboolean
valent_date_label_update_func (gpointer user_data)
{
  for (unsigned int i = 0; i < label_cache->len; i++)
    valent_date_label_update (g_ptr_array_index (label_cache, i));

  return G_SOURCE_CONTINUE;
}

/*
 * GObject
 */
static void
valent_date_label_finalize (GObject *object)
{
  ValentDateLabel *self = VALENT_DATE_LABEL (object);

  g_clear_pointer (&self->label, gtk_widget_unparent);

  g_assert (label_cache != NULL);
  g_ptr_array_remove (label_cache, self);

  if (label_cache->len == 0)
    {
      g_clear_handle_id (&label_source, g_source_remove);
      g_clear_pointer (&label_cache, g_ptr_array_unref);
    }

  G_OBJECT_CLASS (valent_date_label_parent_class)->finalize (object);
}

static void
valent_date_label_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ValentDateLabel *self = VALENT_DATE_LABEL (object);

  switch (prop_id)
    {
    case PROP_DATE:
      g_value_set_int64 (value, valent_date_label_get_date (self));
      break;

    case PROP_FORMAT:
      g_value_set_uint (value, valent_date_label_get_format (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
valent_date_label_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ValentDateLabel *self = VALENT_DATE_LABEL (object);

  switch (prop_id)
    {
    case PROP_DATE:
      valent_date_label_set_date (self, g_value_get_int64 (value));
      break;

    case PROP_FORMAT:
      valent_date_label_set_format (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
valent_date_label_class_init (ValentDateLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = valent_date_label_finalize;
  object_class->get_property = valent_date_label_get_property;
  object_class->set_property = valent_date_label_set_property;

  gtk_widget_class_set_css_name (widget_class, "date-label");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_LABEL);
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  /**
   * ValentDateLabel:date: (getter get_date) (setter set_date)
   *
   * The timestamp this label represents.
   *
   * Since: 1.0
   */
  properties [PROP_DATE] =
    g_param_spec_int64 ("date", NULL, NULL,
                        G_MININT64, G_MAXINT64,
                        0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  /**
   * ValentDateLabel:format: (getter get_format) (setter set_format)
   *
   * The format of the label.
   *
   * Since: 1.0
   */
  properties [PROP_FORMAT] =
    g_param_spec_uint ("format", NULL, NULL,
                       0, G_MAXUINT32,
                       0,
                       (G_PARAM_READWRITE |
                        G_PARAM_EXPLICIT_NOTIFY |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
valent_date_label_init (ValentDateLabel *self)
{
  self->label = gtk_label_new (NULL);
  gtk_widget_insert_after (self->label, GTK_WIDGET (self), NULL);

  gtk_accessible_update_relation (GTK_ACCESSIBLE (self),
                                  GTK_ACCESSIBLE_RELATION_LABELLED_BY,
                                  g_list_append (NULL, self->label),
                                  -1);


  /* Auto-Update */
  if (label_cache == NULL)
    {
      label_cache = g_ptr_array_new ();
      label_source = g_timeout_add_seconds_full (G_PRIORITY_LOW,
                                                 60,
                                                 valent_date_label_update_func,
                                                 NULL,
                                                 NULL);
    }

  g_ptr_array_add (label_cache, self);
}

/**
 * valent_date_label_new:
 * @date: a UNIX epoch timestamp (ms)
 *
 * Create a new #ValentDateLabel for @timestamp.
 *
 * Returns: (transfer full): a #GtkWidget
 *
 * Since: 1.0
 */
GtkWidget *
valent_date_label_new (gint64 date)
{
  return g_object_new (VALENT_TYPE_DATE_LABEL,
                       "date", date,
                       NULL);
}

/**
 * valent_date_label_get_date: (get-property date)
 * @label: a #ValentDateLabel
 *
 * Get the UNIX epoch timestamp (ms) for @label.
 *
 * Returns: the timestamp
 *
 * Since: 1.0
 */
gint64
valent_date_label_get_date (ValentDateLabel *label)
{
  g_return_val_if_fail (VALENT_IS_DATE_LABEL (label), 0);

  return label->date;
}

/**
 * valent_date_label_set_date: (set-property date)
 * @label: a #ValentDateLabel
 * @date: a UNIX epoch timestamp
 *
 * Set the timestamp for @label to @date.
 *
 * Since: 1.0
 */
void
valent_date_label_set_date (ValentDateLabel *label,
                            gint64           date)
{
  g_return_if_fail (VALENT_IS_DATE_LABEL (label));

  if (label->date == date)
    return;

  label->date = date;
  valent_date_label_update (label);
  g_object_notify_by_pspec (G_OBJECT (label), properties [PROP_DATE]);
}

/**
 * valent_date_label_get_format:
 * @label: a #ValentDateLabel
 *
 * Get the display format @label.
 *
 * Returns: the display format
 */
guint
valent_date_label_get_format (ValentDateLabel *label)
{
  g_return_val_if_fail (VALENT_IS_DATE_LABEL (label), 0);

  return label->format;
}

/**
 * valent_date_label_set_format:
 * @label: a #ValentDateLabel
 * @format: a format
 *
 * Set the format of @label to @format. Currently the options are `0` and `1`.
 */
void
valent_date_label_set_format (ValentDateLabel *label,
                            unsigned int     format)
{
  g_return_if_fail (VALENT_IS_DATE_LABEL (label));

  if (label->format == format)
    return;

  label->format = format;
  valent_date_label_update (label);
  g_object_notify_by_pspec (G_OBJECT (label), properties [PROP_FORMAT]);
}


/**
 * valent_date_label_string:
 * @timestamp: a UNIX epoch timestamp (ms)
 *
 * Create a user friendly date-time string for @timestamp, in a relative format.
 *
 * Examples:
 *     - "Just now"
 *     - "15 minutes"
 *     - "11:45 PM"
 *     - "Yesterday · 11:45 PM"
 *     - "Tuesday"
 *     - "February 29"
 *
 * Returns: (transfer full): a new string
 *
 * Since: 1.0
 */
char *
valent_date_label_string (gint64 timestamp)
{
  g_autoptr (GDateTime) dt = NULL;
  g_autoptr (GDateTime) now = NULL;
  GTimeSpan diff;

  dt = g_date_time_new_from_unix_local (timestamp / 1000);
  now = g_date_time_new_now_local ();
  diff = g_date_time_difference (now, dt);

  /* TRANSLATORS: Less than a minute ago */
  if (diff < G_TIME_SPAN_MINUTE)
      return g_strdup (_("Just now"));

  /* TRANSLATORS: Time duration in minutes (eg. 15 minutes) */
  if (diff < G_TIME_SPAN_HOUR)
    {
      unsigned int n_minutes;

      n_minutes = (diff / G_TIME_SPAN_MINUTE);
      return g_strdup_printf (ngettext("%d minute", "%d minutes", n_minutes),
                              n_minutes);
    }

  /* TRANSLATORS: Yesterday, but less than 24 hours (eg. Yesterday · 11:45 PM) */
  if (diff < G_TIME_SPAN_DAY)
    {
      g_autofree char *time_str = NULL;
      int today, day;

      today = g_date_time_get_day_of_month(now);
      day = g_date_time_get_day_of_month(dt);
      time_str = g_date_time_format(dt, "%l:%M %p");

      if (today == day)
        return g_steal_pointer (&time_str);
      else
        return g_strdup_printf (_("Yesterday · %s"), time_str);
    }

  /* Less than a week ago (eg. Tuesday) */
  if (diff < G_TIME_SPAN_DAY * 7)
    return g_date_time_format(dt, "%A");

  /* More than a week ago (eg. February 29) */
  return g_date_time_format(dt, "%B %e");
}

/**
 * valent_date_label_string_short:
 * @timestamp: a UNIX epoch timestamp (ms)
 *
 * Create a user friendly date-time string for @timestamp, in a relative format.
 * This is like valent_date_label_string() but abbreviated.
 *
 * Examples:
 *     - "Just now"
 *     - "15 mins"
 *     - "11:45 PM"
 *     - "Tue"
 *     - "Feb 29"
 *
 * Returns: (transfer full): a new string
 *
 * Since: 1.0
 */
char *
valent_date_label_string_short (gint64 timestamp)
{
  g_autoptr (GDateTime) dt = NULL;
  g_autoptr (GDateTime) now = NULL;
  GTimeSpan diff;

  dt = g_date_time_new_from_unix_local (timestamp / 1000);
  now = g_date_time_new_now_local ();
  diff = g_date_time_difference (now, dt);

  /* TRANSLATORS: Less than a minute ago */
  if (diff < G_TIME_SPAN_MINUTE)
      return g_strdup (_("Just now"));

  /* TRANSLATORS: Time duration in minutes, abbreviated (eg. 15 mins) */
  if (diff < G_TIME_SPAN_HOUR)
    {
      unsigned int n_minutes;

      n_minutes = (diff / G_TIME_SPAN_MINUTE);
      return g_strdup_printf (ngettext ("%d min", "%d mins", n_minutes),
                              n_minutes);
    }


  /* Less than a day ago (eg. 11:45 PM) */
  if (diff < G_TIME_SPAN_DAY)
    return g_date_time_format (dt, "%l:%M %p");

  /* Less than a week ago (eg. Tue) */
  if (diff < G_TIME_SPAN_DAY * 7)
    return g_date_time_format (dt, "%a");

  /* More than a week ago (eg. Feb 29) */
  return g_date_time_format (dt, "%b %e");
}

