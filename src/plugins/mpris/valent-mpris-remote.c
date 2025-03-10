// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-mpris-remote"

#include "config.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libvalent-core.h>
#include <libvalent-device.h>
#include <libvalent-media.h>

#include "valent-mpris-remote.h"
#include "valent-mpris-utils.h"


struct _ValentMprisRemote
{
  AdwWindow          parent_instance;

  ValentDevice      *device;
  GListModel        *players;
  ValentMediaPlayer *player;
  unsigned int       timer_id;

  /* template */
  GtkDropDown       *media_player;
  GtkImage          *media_art;
  GtkLabel          *media_title;
  GtkLabel          *media_artist;
  GtkLabel          *media_album;
  GtkScale          *media_position;
  GtkAdjustment     *media_position_adjustment;
  GtkLabel          *media_position_current;
  GtkLabel          *media_position_length;
  GtkButton         *play_pause_button;
  GtkImage          *repeat_image;
  GtkVolumeButton   *volume_button;
};

G_DEFINE_FINAL_TYPE (ValentMprisRemote, valent_mpris_remote, ADW_TYPE_WINDOW)

enum {
  PROP_0,
  PROP_DEVICE,
  PROP_PLAYERS,
  PROP_SHUFFLE,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };


static char *
get_player_name (ValentMediaPlayer *player)
{
  g_assert (VALENT_IS_MEDIA_PLAYER (player));

  return g_strdup (valent_media_player_get_name (player));
}

static gboolean
valent_mpris_remote_timer_tick (gpointer data)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (data);
  g_autofree char *length = NULL;
  g_autofree char *current = NULL;
  double value = 0.0;
  double upper = 0.0;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  value = gtk_adjustment_get_value (self->media_position_adjustment);
  upper = gtk_adjustment_get_upper (self->media_position_adjustment);

  current = valent_mpris_time_to_string (value * 1000L, TOTEM_TIME_FLAG_NONE);
  gtk_label_set_label (self->media_position_current, current);

  length = valent_mpris_time_to_string (upper * 1000L, TOTEM_TIME_FLAG_NONE);
  gtk_label_set_label (self->media_position_length, length);

  gtk_adjustment_set_value (self->media_position_adjustment, value + 1.0);

  return G_SOURCE_CONTINUE;
}

/*
 * Interface
 */
static void
valent_mpris_remote_clear (ValentMprisRemote *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  gtk_image_set_from_icon_name (self->media_art, "valent-mpris-albumart");

  gtk_label_set_label (self->media_artist, "");
  gtk_label_set_label (self->media_title, "");
  gtk_label_set_label (self->media_album, "");

  gtk_adjustment_set_value (self->media_position_adjustment, 0.0);
  gtk_adjustment_set_upper (self->media_position_adjustment, 0.0);
  gtk_label_set_label (self->media_position_current, "");
  gtk_label_set_label (self->media_position_length, "");

  gtk_widget_action_set_enabled (widget, "remote.next", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.pause", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.play", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.play-pause", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.previous", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.seek", FALSE);
  gtk_widget_action_set_enabled (widget, "remote.stop", FALSE);
}

static void
valent_mpris_remote_update_flags (ValentMprisRemote *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  ValentMediaActions flags = VALENT_MEDIA_ACTION_NONE;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return valent_mpris_remote_clear (self);

  flags = valent_media_player_get_flags (self->player);

  gtk_widget_action_set_enabled (widget, "remote.next",
                                 (flags & VALENT_MEDIA_ACTION_NEXT) != 0);
  gtk_widget_action_set_enabled (widget, "remote.pause",
                                 (flags & VALENT_MEDIA_ACTION_PAUSE) != 0);
  gtk_widget_action_set_enabled (widget, "remote.play",
                                 (flags & VALENT_MEDIA_ACTION_PLAY) != 0);
  gtk_widget_action_set_enabled (widget, "remote.play-pause",
                                 (flags & (VALENT_MEDIA_ACTION_PLAY |
                                           VALENT_MEDIA_ACTION_PAUSE)) != 0);
  gtk_widget_action_set_enabled (widget, "remote.previous",
                                 (flags & VALENT_MEDIA_ACTION_PREVIOUS) != 0);
  gtk_widget_action_set_enabled (widget, "remote.seek",
                                 (flags & VALENT_MEDIA_ACTION_SEEK) != 0);
  gtk_widget_action_set_enabled (widget, "remote.stop",
                                 (flags & VALENT_MEDIA_ACTION_STOP) != 0);
}

static void
valent_mpris_remote_update_position (ValentMprisRemote *self)
{
  double position = 0.0;
  g_autofree char *position_str = NULL;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return valent_mpris_remote_clear (self);

  position = valent_media_player_get_position (self->player);
  gtk_adjustment_set_value (self->media_position_adjustment, position);

  position_str = valent_mpris_time_to_string (position * 1000L, TOTEM_TIME_FLAG_NONE);
  gtk_label_set_label (self->media_position_current, position_str);
}

static void
valent_mpris_remote_update_metadata (ValentMprisRemote *self)
{
  g_autoptr (GVariant) metadata = NULL;
  g_autofree const char **artists = NULL;
  g_autofree char *length_str = NULL;
  const char *title;
  const char *album;
  const char *art_url;
  gint64 length_us = 0;
  double length = -1.0;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return valent_mpris_remote_clear (self);

  metadata = valent_media_player_get_metadata (self->player);

  if (g_variant_lookup (metadata, "xesam:artist", "^a&s", &artists) &&
      artists[0] != NULL && *artists[0] != '\0')
    {
      g_autofree char *artist = NULL;

      artist = g_strjoinv (", ", (char **)artists);
      gtk_label_set_label (self->media_artist, artist);
    }
  else
    {
      gtk_label_set_label (self->media_artist, "");
    }

  if (g_variant_lookup (metadata, "xesam:album", "&s", &album))
    gtk_label_set_label (self->media_album, album);
  else
    gtk_label_set_label (self->media_album, "");

  if (g_variant_lookup (metadata, "xesam:title", "&s", &title))
    gtk_label_set_label (self->media_title, title);
  else
    gtk_label_set_label (self->media_title, "");

  if (g_variant_lookup (metadata, "mpris:artUrl", "&s", &art_url))
    {
      g_autoptr (GIcon) icon = NULL;

      icon = g_icon_new_for_string (art_url, NULL);
      gtk_image_set_from_gicon (self->media_art, icon);
    }
  else
    {
      gtk_image_set_from_icon_name (self->media_art, "valent-mpris-albumart");
    }

  /* Convert microseconds to seconds */
  if (g_variant_lookup (metadata, "mpris:length", "x", &length_us))
    length = length_us / G_TIME_SPAN_SECOND;

  gtk_adjustment_set_upper (self->media_position_adjustment, length);
  length_str = valent_mpris_time_to_string (length * 1000L, TOTEM_TIME_FLAG_NONE);
  gtk_label_set_label (self->media_position_length, length_str);

  valent_mpris_remote_update_position (self);
}

static void
valent_mpris_remote_update_repeat (ValentMprisRemote *self)
{
  ValentMediaRepeat repeat = VALENT_MEDIA_REPEAT_NONE;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player != NULL)
     repeat = valent_media_player_get_repeat (self->player);

  switch (repeat)
    {
    case VALENT_MEDIA_REPEAT_NONE:
      gtk_image_set_from_icon_name (self->repeat_image,
                                    "media-playlist-consecutive-symbolic");
      break;

    case VALENT_MEDIA_REPEAT_ALL:
      gtk_image_set_from_icon_name (self->repeat_image,
                                    "media-playlist-repeat-symbolic");
      break;

    case VALENT_MEDIA_REPEAT_ONE:
      gtk_image_set_from_icon_name (self->repeat_image,
                                    "media-playlist-repeat-song-symbolic");
      break;
    }
}

static void
valent_mpris_remote_update_state (ValentMprisRemote *self)
{
  GtkWidget *child;
  ValentMediaState state;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return valent_mpris_remote_clear (self);

  child = gtk_button_get_child (self->play_pause_button);
  state = valent_media_player_get_state (self->player);

  if (state == VALENT_MEDIA_STATE_PLAYING)
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (child),
                                    "media-playback-pause-symbolic");
      gtk_widget_set_tooltip_text (GTK_WIDGET (self->play_pause_button),
                                   _("Pause"));

      if (self->timer_id == 0)
        self->timer_id = g_timeout_add_seconds (1, valent_mpris_remote_timer_tick, self);
    }
  else
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (child),
                                    "media-playback-start-symbolic");
      gtk_widget_set_tooltip_text (GTK_WIDGET (self->play_pause_button),
                                   _("Play"));
      g_clear_handle_id (&self->timer_id, g_source_remove);
    }

  if (state == VALENT_MEDIA_STATE_STOPPED)
    {
      g_object_freeze_notify (G_OBJECT (self->media_position_adjustment));
      gtk_adjustment_set_value (self->media_position_adjustment, 0.0);
      gtk_adjustment_set_upper (self->media_position_adjustment, 0.0);
      g_object_thaw_notify (G_OBJECT (self->media_position_adjustment));
    }

  valent_mpris_remote_update_metadata (self);
}

static void
valent_mpris_remote_update_shuffle (ValentMprisRemote *self)
{
  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHUFFLE]);
}

static void
valent_mpris_remote_update_volume (ValentMprisRemote *self)
{
  double volume = 0.0;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return;

  volume = valent_media_player_get_volume (self->player);
  gtk_scale_button_set_value (GTK_SCALE_BUTTON (self->volume_button), volume);
}

static void
on_selected_item (GObject           *object,
                  GParamSpec        *pspec,
                  ValentMprisRemote *self)
{
  ValentMediaPlayer *player;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player != NULL)
    {
      g_clear_handle_id (&self->timer_id, g_source_remove);
      g_signal_handlers_disconnect_by_data (self->player, self);
      g_clear_object (&self->player);
    }

  player = gtk_drop_down_get_selected_item (GTK_DROP_DOWN (object));

  if (g_set_object (&self->player, player))
    {
      g_signal_connect_object (self->player,
                               "notify::flags",
                               G_CALLBACK (valent_mpris_remote_update_flags),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::metadata",
                               G_CALLBACK (valent_mpris_remote_update_metadata),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::position",
                               G_CALLBACK (valent_mpris_remote_update_position),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::repeat",
                               G_CALLBACK (valent_mpris_remote_update_repeat),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::shuffle",
                               G_CALLBACK (valent_mpris_remote_update_shuffle),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::state",
                               G_CALLBACK (valent_mpris_remote_update_state),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (self->player,
                               "notify::volume",
                               G_CALLBACK (valent_mpris_remote_update_volume),
                               self, G_CONNECT_SWAPPED);
    }
  else
    {
      valent_mpris_remote_clear (self);
      return;
    }

  valent_mpris_remote_update_flags (self);
  valent_mpris_remote_update_metadata (self);
  valent_mpris_remote_update_position (self);
  valent_mpris_remote_update_repeat (self);
  valent_mpris_remote_update_shuffle (self);
  valent_mpris_remote_update_state (self);
  valent_mpris_remote_update_volume (self);
}

static gboolean
on_change_value (GtkRange          *range,
                 GtkScrollType      scroll,
                 double             value,
                 ValentMprisRemote *self)
{
  double lower, upper, page_size;

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return GDK_EVENT_STOP;

  lower = gtk_adjustment_get_lower (self->media_position_adjustment);
  upper = gtk_adjustment_get_upper (self->media_position_adjustment);
  page_size = gtk_adjustment_get_page_size (self->media_position_adjustment);
  value = CLAMP (value, lower, (upper - page_size));

  valent_media_player_set_position (self->player, value);
  gtk_adjustment_set_value (self->media_position_adjustment, value);

  return GDK_EVENT_STOP;
}

static void
on_volume_changed (GtkScaleButton    *button,
                   double             value,
                   ValentMprisRemote *self)
{
  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL)
    return;

  if (valent_media_player_get_volume (self->player) != value)
    valent_media_player_set_volume (self->player, value);
}

/*
 * GAction
 */
static void
remote_player_action (GtkWidget  *widget,
                      const char *action_name,
                      GVariant   *parameter)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (widget);

  g_assert (VALENT_IS_MPRIS_REMOTE (self));

  if (self->player == NULL || action_name == NULL)
    return;

  else if (g_str_equal (action_name, "remote.next"))
    valent_media_player_next (self->player);

  else if (g_str_equal (action_name, "remote.pause"))
    valent_media_player_pause (self->player);

  else if (g_str_equal (action_name, "remote.play"))
    valent_media_player_play (self->player);

  else if (g_str_equal (action_name, "remote.play-pause"))
    valent_media_player_play_pause (self->player);

  else if (g_str_equal (action_name, "remote.previous"))
    valent_media_player_previous (self->player);

  else if (g_str_equal (action_name, "remote.repeat"))
    {
      const char *icon_name = gtk_image_get_icon_name (self->repeat_image);

      if (g_str_equal (icon_name, "media-playlist-consecutive-symbolic"))
        valent_media_player_set_repeat (self->player, VALENT_MEDIA_REPEAT_ALL);

      else if (g_str_equal (icon_name, "media-playlist-repeat-symbolic"))
        valent_media_player_set_repeat (self->player, VALENT_MEDIA_REPEAT_ONE);

      else if (g_str_equal (icon_name, "media-playlist-repeat-song-symbolic"))
        valent_media_player_set_repeat (self->player, VALENT_MEDIA_REPEAT_NONE);
    }

  else if (g_str_equal (action_name, "remote.seek"))
    valent_media_player_seek (self->player, g_variant_get_double (parameter));

  else if (g_str_equal (action_name, "remote.stop"))
    valent_media_player_stop (self->player);
}

/*
 * GObject
 */
static void
valent_mpris_remote_constructed (GObject *object)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (object);
  g_autoptr (GtkExpression) expression = NULL;
  GtkWidget *child;

  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "device",
                                  G_ACTION_GROUP (self->device));

  expression = gtk_cclosure_expression_new (G_TYPE_STRING, NULL,
                                            0, NULL,
                                            G_CALLBACK (get_player_name),
                                            NULL, NULL);

  gtk_drop_down_set_expression (self->media_player, expression);
  gtk_drop_down_set_model (self->media_player, self->players);

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->volume_button));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      if (!GTK_IS_TOGGLE_BUTTON (child))
        continue;

      gtk_widget_set_css_classes (child, (const char *[]){
                                            "circular",
                                            "image-button",
                                            "toggle",
                                            NULL
                                         });
    }

  G_OBJECT_CLASS (valent_mpris_remote_parent_class)->constructed (object);
}

static void
valent_mpris_remote_dispose (GObject *object)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (object);

  g_clear_handle_id (&self->timer_id, g_source_remove);

  if (self->player != NULL)
    {
      g_signal_handlers_disconnect_by_data (self->player, self);
      g_clear_object (&self->player);
    }

  G_OBJECT_CLASS (valent_mpris_remote_parent_class)->dispose (object);
}

static void
valent_mpris_remote_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (object);

  switch (prop_id)
    {
    case PROP_DEVICE:
      g_value_set_object (value, self->device);
      break;

    case PROP_PLAYERS:
      g_value_set_object (value, self->players);
      break;

    case PROP_SHUFFLE:
      if (self->player != NULL)
        g_value_set_boolean (value, valent_media_player_get_shuffle (self->player));
      else
        g_value_set_boolean (value, FALSE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
valent_mpris_remote_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ValentMprisRemote *self = VALENT_MPRIS_REMOTE (object);

  switch (prop_id)
    {
    case PROP_DEVICE:
      self->device = g_value_get_object (value);
      break;

    case PROP_PLAYERS:
      self->players = g_value_dup_object (value);
      break;

    case PROP_SHUFFLE:
      if (self->player != NULL)
        valent_media_player_set_shuffle (self->player, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
valent_mpris_remote_class_init (ValentMprisRemoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = valent_mpris_remote_constructed;
  object_class->dispose = valent_mpris_remote_dispose;
  object_class->get_property = valent_mpris_remote_get_property;
  object_class->set_property = valent_mpris_remote_set_property;

  gtk_widget_class_set_template_from_resource (widget_class, "/plugins/mpris/valent-mpris-remote.ui");
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_player);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_art);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_title);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_artist);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_album);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_position);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_position_adjustment);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_position_current);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, media_position_length);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, play_pause_button);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, repeat_image);
  gtk_widget_class_bind_template_child (widget_class, ValentMprisRemote, volume_button);
  gtk_widget_class_bind_template_callback (widget_class, on_selected_item);
  gtk_widget_class_bind_template_callback (widget_class, on_change_value);
  gtk_widget_class_bind_template_callback (widget_class, on_volume_changed);

  gtk_widget_class_install_action (widget_class, "remote.next", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.pause", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.play", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.play-pause", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.previous", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.repeat", NULL, remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.seek", "d", remote_player_action);
  gtk_widget_class_install_action (widget_class, "remote.stop", NULL, remote_player_action);

  properties [PROP_DEVICE] =
    g_param_spec_object ("device", NULL, NULL,
                         VALENT_TYPE_DEVICE,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_PLAYERS] =
    g_param_spec_object ("players", NULL, NULL,
                         G_TYPE_LIST_MODEL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_SHUFFLE] =
    g_param_spec_boolean ("shuffle", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE |
                           G_PARAM_EXPLICIT_NOTIFY |
                           G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  gtk_widget_class_install_property_action (widget_class, "remote.shuffle", "shuffle");
}

static void
valent_mpris_remote_init (ValentMprisRemote *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

