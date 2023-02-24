// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-runcommand-preferences"

#include "config.h"

#include <glib/gi18n.h>
#include <libportal/portal.h>
#include <libvalent-core.h>
#include <libvalent-ui.h>

#include "valent-runcommand-editor.h"
#include "valent-runcommand-preferences.h"
#include "valent-runcommand-utils.h"


struct _ValentRuncommandPreferences
{
  ValentDevicePreferencesPage  parent_instance;

  GtkWindow                   *editor;

  /* template */
  AdwPreferencesGroup         *command_group;
  GtkListBox                  *command_list;
  GtkWidget                   *command_placeholder;
};

G_DEFINE_FINAL_TYPE (ValentRuncommandPreferences, valent_runcommand_preferences, VALENT_TYPE_DEVICE_PREFERENCES_PAGE)


/*
 * Rows
 */
static void
valent_runcommand_preferences_populate (ValentRuncommandPreferences *self)
{
  ValentDevicePreferencesPage *page = VALENT_DEVICE_PREFERENCES_PAGE (self);
  GSettings *settings;
  GtkWidget *child;
  g_autoptr (GVariant) commands = NULL;
  GVariantIter iter;
  char *uuid;
  GVariant *item;

  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (self));

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self->command_list));
       child != NULL;)
    {
      GtkWidget *current_child = child;
      child = gtk_widget_get_next_sibling (current_child);

      if (self->command_placeholder != current_child)
        gtk_list_box_remove (self->command_list, current_child);
    }

  settings = valent_device_preferences_page_get_settings (page);
  commands = g_settings_get_value (settings, "commands");

  g_variant_iter_init (&iter, commands);

  while (g_variant_iter_next (&iter, "{sv}", &uuid, &item))
    {
      const char *name = NULL;
      const char *command = NULL;

      if (g_variant_lookup (item, "name", "&s", &name) &&
          g_variant_lookup (item, "command", "&s", &command))
        {
          GtkWidget *row;
          GtkWidget *arrow;

          row = g_object_new (ADW_TYPE_ACTION_ROW,
                              "name",          uuid,
                              "title",         name,
                              "subtitle",      command,
                              "activatable",   TRUE,
                              "action-target", g_variant_new_string (uuid),
                              "action-name",   "runcommand.edit",
                              NULL);

          arrow = gtk_image_new_from_icon_name ("go-next-symbolic");
          gtk_widget_add_css_class (arrow, "dim-label");
          adw_action_row_add_suffix (ADW_ACTION_ROW (row), arrow);

          gtk_list_box_append (self->command_list, row);
        }

      g_clear_pointer (&uuid, g_free);
      g_clear_pointer (&item, g_variant_unref);
    }
}

static int
sort_commands (GtkListBoxRow *row1,
               GtkListBoxRow *row2,
               gpointer       user_data)
{
  const char *title1;
  const char *title2;

  title1 = adw_preferences_row_get_title (ADW_PREFERENCES_ROW (row1));
  title2 = adw_preferences_row_get_title (ADW_PREFERENCES_ROW (row2));

  return g_utf8_collate (title1, title2);
}

/*
 * GAction
 */
static void
on_command_changed (ValentRuncommandEditor      *editor,
                    GParamSpec                  *pspec,
                    ValentRuncommandPreferences *self)
{
  const char *uuid;
  GVariant *command;

  g_assert (VALENT_IS_RUNCOMMAND_EDITOR (editor));
  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (self));

  uuid = valent_runcommand_editor_get_uuid (editor);
  command = valent_runcommand_editor_get_command (editor);

  if (command != NULL)
    gtk_widget_activate_action (GTK_WIDGET (self), "runcommand.save", "s", uuid);
  else
    gtk_widget_activate_action (GTK_WIDGET (self), "runcommand.remove", "s", uuid);

  gtk_window_destroy (GTK_WINDOW (editor));
}

static void
runcommand_add_action (GtkWidget  *widget,
                       const char *action_name,
                       GVariant   *parameter)
{
  g_autofree char *uuid = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (widget));

  uuid = g_uuid_string_random ();
  gtk_widget_activate_action (widget, "runcommand.edit", "s", uuid);
}

static void
runcommand_edit_action (GtkWidget  *widget,
                        const char *action_name,
                        GVariant   *parameter)
{
  ValentRuncommandPreferences *self = VALENT_RUNCOMMAND_PREFERENCES (widget);
  ValentDevicePreferencesPage *page = VALENT_DEVICE_PREFERENCES_PAGE (self);
  const char *uuid = NULL;
  GSettings *settings;
  GtkRoot *window;
  g_autoptr (GVariant) commands = NULL;
  g_autoptr (GVariant) command = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (self));

  uuid = g_variant_get_string (parameter, NULL);
  settings = valent_device_preferences_page_get_settings (page);
  commands = g_settings_get_value (settings, "commands");
  g_variant_lookup (commands, uuid, "@a{sv}", &command);

  window = gtk_widget_get_root (GTK_WIDGET (self));
  self->editor = g_object_new (VALENT_TYPE_RUNCOMMAND_EDITOR,
                               "uuid",          uuid,
                               "command",       command,
                               "modal",         (window != NULL),
                               "transient-for", window,
                               NULL);
  g_object_add_weak_pointer (G_OBJECT (self->editor),
                             (gpointer) &self->editor);
  g_signal_connect_object (self->editor,
                           "notify::command",
                           G_CALLBACK (on_command_changed),
                           self, 0);

  gtk_window_present_with_time (self->editor, GDK_CURRENT_TIME);
}

static void
runcommand_save_action (GtkWidget  *widget,
                        const char *action_name,
                        GVariant   *parameter)
{
  ValentRuncommandPreferences *self = VALENT_RUNCOMMAND_PREFERENCES (widget);
  ValentDevicePreferencesPage *page = VALENT_DEVICE_PREFERENCES_PAGE (self);
  GSettings *settings;
  g_autoptr (GVariant) commands = NULL;
  GVariant *command;
  const char *uuid;
  GVariantDict dict;
  GVariant *value;

  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (self));

  settings = valent_device_preferences_page_get_settings (page);
  commands = g_settings_get_value (settings, "commands");

  uuid = valent_runcommand_editor_get_uuid (VALENT_RUNCOMMAND_EDITOR (self->editor));
  command = valent_runcommand_editor_get_command (VALENT_RUNCOMMAND_EDITOR (self->editor));

  g_variant_dict_init (&dict, commands);
  g_variant_dict_insert_value (&dict, uuid, command);
  value = g_variant_dict_end (&dict);

  g_settings_set_value (settings, "commands", value);
}

static void
runcommand_remove_action (GtkWidget  *widget,
                          const char *action_name,
                          GVariant   *parameter)
{
  ValentRuncommandPreferences *self = VALENT_RUNCOMMAND_PREFERENCES (widget);
  ValentDevicePreferencesPage *page = VALENT_DEVICE_PREFERENCES_PAGE (self);
  GSettings *settings;
  g_autoptr (GVariant) commands = NULL;
  GVariantDict dict;
  GVariant *value;
  const char *uuid = NULL;

  g_assert (VALENT_IS_RUNCOMMAND_PREFERENCES (self));

  uuid = g_variant_get_string (parameter, NULL);
  settings = valent_device_preferences_page_get_settings (page);
  commands = g_settings_get_value (settings, "commands");

  g_variant_dict_init (&dict, commands);
  g_variant_dict_remove (&dict, uuid);
  value = g_variant_dict_end (&dict);

  g_settings_set_value (settings, "commands", value);
}

/*
 * GObject
 */
static void
valent_runcommand_preferences_constructed (GObject *object)
{
  ValentRuncommandPreferences *self = VALENT_RUNCOMMAND_PREFERENCES (object);
  ValentDevicePreferencesPage *page = VALENT_DEVICE_PREFERENCES_PAGE (self);
  GSettings *settings;

  gtk_list_box_set_sort_func (self->command_list, sort_commands, self, NULL);

  /* Setup GSettings */
  settings = valent_device_preferences_page_get_settings (page);

  g_signal_connect_object (settings,
                           "changed::commands",
                           G_CALLBACK (valent_runcommand_preferences_populate),
                           self,
                           G_CONNECT_SWAPPED);
  valent_runcommand_preferences_populate (self);

  G_OBJECT_CLASS (valent_runcommand_preferences_parent_class)->constructed (object);
}

static void
valent_runcommand_preferences_finalize (GObject *object)
{
  ValentRuncommandPreferences *self = VALENT_RUNCOMMAND_PREFERENCES (object);

  g_clear_pointer (&self->editor, gtk_window_destroy);

  G_OBJECT_CLASS (valent_runcommand_preferences_parent_class)->finalize (object);
}

static void
valent_runcommand_preferences_class_init (ValentRuncommandPreferencesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = valent_runcommand_preferences_constructed;
  object_class->finalize = valent_runcommand_preferences_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/plugins/runcommand/valent-runcommand-preferences.ui");
  gtk_widget_class_bind_template_child (widget_class, ValentRuncommandPreferences, command_group);
  gtk_widget_class_bind_template_child (widget_class, ValentRuncommandPreferences, command_list);
  gtk_widget_class_bind_template_child (widget_class, ValentRuncommandPreferences, command_placeholder);

  gtk_widget_class_install_action (widget_class, "runcommand.add", NULL, runcommand_add_action);
  gtk_widget_class_install_action (widget_class, "runcommand.edit", "s", runcommand_edit_action);
  gtk_widget_class_install_action (widget_class, "runcommand.remove", "s", runcommand_remove_action);
  gtk_widget_class_install_action (widget_class, "runcommand.save", "s", runcommand_save_action);
}

static void
valent_runcommand_preferences_init (ValentRuncommandPreferences *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

