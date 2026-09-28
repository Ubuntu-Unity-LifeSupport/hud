/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ted Gould <ted@canonical.com>
 */

#if !defined (_HUD_CLIENT_H_INSIDE) && !defined (HUD_CLIENT_COMPILATION)
#error "Only <hud-client.h> can be included directly."
#endif

#ifndef __HUD_CLIENT_QUERY_H__
#define __HUD_CLIENT_QUERY_H__

#pragma GCC visibility push(default)

#include <glib-object.h>
#include <dee.h>
#include <libhud-client/connection.h>
#include <libhud-client/param.h>
#include <libhud-client/toolbar-items.h>

G_BEGIN_DECLS

#define HUD_CLIENT_TYPE_QUERY            (hud_client_query_get_type ())
#define HUD_CLIENT_QUERY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HUD_CLIENT_TYPE_QUERY, HudClientQuery))
#define HUD_CLIENT_QUERY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HUD_CLIENT_TYPE_QUERY, HudClientQueryClass))
#define HUD_CLIENT_IS_QUERY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HUD_CLIENT_TYPE_QUERY))
#define HUD_CLIENT_IS_QUERY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HUD_CLIENT_TYPE_QUERY))
#define HUD_CLIENT_QUERY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HUD_CLIENT_TYPE_QUERY, HudClientQueryClass))

/**
 * HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED
 *
 * Signal to indicate when the models have changed
 */
#define HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED   "models-changed"
/**
 * HUD_CLIENT_QUERY_SIGNAL_TOOLBAR_UPDATED
 *
 * Signal to indicate when the toolbar has been updated
 */
#define HUD_CLIENT_QUERY_SIGNAL_TOOLBAR_UPDATED   "toolbar-updated"

typedef struct _HudClientQuery         HudClientQuery;
typedef struct _HudClientQueryClass    HudClientQueryClass;
typedef struct _HudClientQueryPrivate  HudClientQueryPrivate;

/**
 * HudClientQueryPrivate:
 *
 * Private data for #HudClientQuery.
 */

/**
 * HudClientQueryClass:
 * @parent_class: #GObjectClass
 *
 * Class information for #HudClientQuery
 */
struct _HudClientQueryClass {
	GObjectClass parent_class;
};

/**
 * HudClientQuery:
 *
 * Object to track a query and the models for that query.  Should
 * be unref'd when a client is done using the query so that applications
 * can be told that the HUD is no longer open.
 */
struct _HudClientQuery {
	GObject parent;
	HudClientQueryPrivate * priv;
};

GType              hud_client_query_get_type              (void);

HudClientQuery *   hud_client_query_new                   (const gchar *           query);
HudClientQuery *   hud_client_query_new_for_connection    (const gchar *           query,
                                                           HudClientConnection *   connection);

/* Query Tools */
void               hud_client_query_set_query             (HudClientQuery *        cquery,
                                                           const gchar *           query);
const gchar *      hud_client_query_get_query             (HudClientQuery *        cquery);

void               hud_client_query_voice_query           (HudClientQuery *        cquery);

/* Accessors */
DeeModel *         hud_client_query_get_results_model     (HudClientQuery *        cquery);
DeeModel *         hud_client_query_get_appstack_model    (HudClientQuery *        cquery);

gboolean           hud_client_query_toolbar_item_active   (HudClientQuery *        cquery,
                                                           HudClientQueryToolbarItems  item);
GArray *           hud_client_query_get_active_toolbar    (HudClientQuery *        cquery);

/* Execute and Control */
void               hud_client_query_set_appstack_app      (HudClientQuery *        cquery,
                                                           const gchar *           application_id);
void               hud_client_query_execute_command       (HudClientQuery *        cquery,
                                                           GVariant *              command_key,
                                                           guint                   timestamp);
HudClientParam *   hud_client_query_execute_param_command (HudClientQuery *        cquery,
                                                           GVariant *              command_key,
                                                           guint                   timestamp);
void               hud_client_query_execute_toolbar_item  (HudClientQuery *        cquery,
                                                           HudClientQueryToolbarItems  item,
                                                           guint                   timestamp);

/* Appstack Accessors */
const gchar *      hud_client_query_appstack_get_app_id   (HudClientQuery *        cquery,
                                                           DeeModelIter *          row);
const gchar *      hud_client_query_appstack_get_app_icon (HudClientQuery *        cquery,
                                                           DeeModelIter *          row);

/* Results Accessors */
GVariant *         hud_client_query_results_get_command_id (HudClientQuery *       cquery,
                                                            DeeModelIter *         row);
const gchar *      hud_client_query_results_get_command_name (HudClientQuery *     cquery,
                                                            DeeModelIter *         row);
GVariant *         hud_client_query_results_get_command_highlights (HudClientQuery * cquery,
                                                            DeeModelIter *         row);
const gchar *      hud_client_query_results_get_description (HudClientQuery *      cquery,
                                                            DeeModelIter *         row);
GVariant *         hud_client_query_results_get_description_highlights (HudClientQuery * cquery,
                                                            DeeModelIter *         row);
const gchar *      hud_client_query_results_get_shortcut   (HudClientQuery *       cquery,
                                                            DeeModelIter *         row);
gboolean           hud_client_query_results_is_parameterized (HudClientQuery *     cquery,
                                                            DeeModelIter *         row);

/**
	SECTION:query
	@short_description: Query the HUD service for entries
	@stability: Unstable
	@include: libhud-client/query.h

	A query is an open query to the HUD service which provides
	Dee models for the results.  The query can update without changing
	the search string (the application changes the entires) or can
	be udated by calling hud_client_query_set_query().

	When the usage of the Query is complete it should be unreferenced
	as that will communicate to the applications that the HUD is closed
	and they should not update their items.
*/

G_END_DECLS

#pragma GCC visibility pop

#endif
