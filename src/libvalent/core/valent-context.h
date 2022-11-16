// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#pragma once

#if !defined (VALENT_CORE_INSIDE) && !defined (VALENT_CORE_COMPILATION)
# error "Only <libvalent-core.h> can be included directly."
#endif

#include <libpeas/peas.h>

#include "valent-object.h"

G_BEGIN_DECLS

#define VALENT_TYPE_CONTEXT (valent_context_get_type())

VALENT_AVAILABLE_IN_1_0
G_DECLARE_DERIVABLE_TYPE (ValentContext, valent_context, VALENT, CONTEXT, ValentObject)

struct _ValentContextClass
{
  ValentObjectClass   parent_class;

  /*< private >*/
  gpointer            padding[8];
};

VALENT_AVAILABLE_IN_1_0
ValentContext * valent_context_new                    (ValentContext  *parent,
                                                       const char     *domain,
                                                       const char     *id);
VALENT_AVAILABLE_IN_1_0
ValentContext * valent_context_new_for_plugin         (ValentContext  *parent,
                                                       PeasPluginInfo *plugin_info);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_cache_path         (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_config_path        (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_data_path          (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_domain             (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_id                 (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
ValentContext * valent_context_get_parent             (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
const char    * valent_context_get_path               (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
void            valent_context_clear_cache            (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
void            valent_context_clear                  (ValentContext  *context);
VALENT_AVAILABLE_IN_1_0
GFile         * valent_context_create_cache_file      (ValentContext  *context,
                                                       const char     *filename);
VALENT_AVAILABLE_IN_1_0
GFile         * valent_context_create_config_file     (ValentContext  *context,
                                                       const char     *filename);
VALENT_AVAILABLE_IN_1_0
GFile         * valent_context_create_data_file       (ValentContext  *context,
                                                       const char     *filename);
VALENT_AVAILABLE_IN_1_0
GSettings     * valent_context_create_plugin_settings (ValentContext  *context,
                                                       PeasPluginInfo *plugin_info,
                                                       const char     *plugin_key);
VALENT_AVAILABLE_IN_1_0
GSettings     * valent_context_create_settings        (ValentContext  *context,
                                                       const char     *schema_id);

/* Static Utilities */
VALENT_AVAILABLE_IN_1_0
const char    * valent_get_user_directory             (GUserDirectory  directory);
VALENT_AVAILABLE_IN_1_0
GFile         * valent_context_get_file               (const char     *dirname,
                                                       const char     *basename,
                                                       gboolean        unique);

G_END_DECLS
