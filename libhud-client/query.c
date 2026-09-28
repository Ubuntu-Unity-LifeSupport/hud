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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dee.h>

#include "query.h"
#include "connection.h"
#include "connection-private.h"
#include "query-iface.h"
#include "enum-types.h"
#include "common/query-columns.h"
#include "common/shared-values.h"

struct _HudClientQueryPrivate {
	_HudQueryComCanonicalHudQuery * proxy;
	HudClientConnection * connection;
	guint connection_changed_sig;
	gchar * query;
	DeeModel * results;
	DeeModel * appstack;
	GArray * toolbar;
};

#define HUD_CLIENT_QUERY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_CLIENT_TYPE_QUERY, HudClientQueryPrivate))

enum {
	PROP_0 = 0,
	PROP_CONNECTION,
	PROP_QUERY
};

#define PROP_CONNECTION_S  "connection"
#define PROP_QUERY_S       "query"

static void hud_client_query_class_init  (HudClientQueryClass *klass);
static void hud_client_query_init        (HudClientQuery *self);
static void hud_client_query_constructed (GObject *object);
static void hud_client_query_dispose     (GObject *object);
static void hud_client_query_finalize    (GObject *object);
static void set_property (GObject * obj, guint id, const GValue * value, GParamSpec * pspec);
static void get_property (GObject * obj, guint id, GValue * value, GParamSpec * pspec);
static void connection_status (HudClientConnection * connection, gboolean connected, HudClientQuery * query);
static void new_query_cb (HudClientConnection * connection, const gchar * path, const gchar * results, const gchar * appstack, gpointer user_data);

G_DEFINE_TYPE (HudClientQuery, hud_client_query, G_TYPE_OBJECT)

static guint signal_toolbar_updated = 0;
static guint hud_client_query_signal_voice_query_loading;
static guint hud_client_query_signal_voice_query_failed;
static guint hud_client_query_signal_voice_query_listening;
static guint hud_client_query_signal_voice_query_heard_something;
static guint hud_client_query_signal_voice_query_finished;
static guint hud_client_query_signal_models_changed = 0;

static void
hud_client_query_class_init (HudClientQueryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudClientQueryPrivate));

	object_class->dispose = hud_client_query_dispose;
	object_class->finalize = hud_client_query_finalize;
	object_class->constructed = hud_client_query_constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	g_object_class_install_property (object_class, PROP_CONNECTION,
	                                 g_param_spec_object(PROP_CONNECTION_S, "Connection to the HUD service",
	                                              "HUD service connection",
	                                              HUD_CLIENT_TYPE_CONNECTION,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class, PROP_QUERY,
	                                 g_param_spec_string(PROP_QUERY_S, "Query to the HUD service",
	                                              "HUD query",
	                                              NULL,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

	/**
	 * HudClientQuery::toolbar-updated:
	 *
	 * The active items in the toolbar changed.  Please requery.
	 */
	signal_toolbar_updated = g_signal_new (HUD_CLIENT_QUERY_SIGNAL_TOOLBAR_UPDATED,
	                                       HUD_CLIENT_TYPE_QUERY,
	                                       G_SIGNAL_RUN_LAST,
	                                       0, /* offset */
	                                       NULL, NULL, /* Accumulator */
	                                       g_cclosure_marshal_VOID__VOID,
	                                       G_TYPE_NONE, 0, G_TYPE_NONE);

	/**
	 * HudClientQuery::voice-query-loading:
	 *
	 * The voice recognition toolkit is loading, and not ready for speech yet.
	 */
	hud_client_query_signal_voice_query_loading = g_signal_new (
		"voice-query-loading", HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
		NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	/**
   * HudClientQuery::voice-query-failed:
   *
   * The voice recognition toolkit has failed to connect to the audio device.
   * The specific cause is provided as an argument.
   */
  hud_client_query_signal_voice_query_failed = g_signal_new (
    "voice-query-failed", HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
    NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING );

	/**
   * HudClientQuery::voice-query-listening:
   *
   * The voice recognition toolkit is active and listening for speech.
   */
	hud_client_query_signal_voice_query_listening = g_signal_new (
		"voice-query-listening", HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0,
		NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	/**
   * HudClientQuery::voice-query-heard-something:
   *
   * The voice recognition toolkit has heard an utterance.
   */
  hud_client_query_signal_voice_query_heard_something = g_signal_new (
    "voice-query-heard-something", HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST,
    0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	/**
   * HudClientQuery::voice-query-finished:
   *
   * The voice recognition toolkit has completed and has a (possibly empty) result.
   */
	hud_client_query_signal_voice_query_finished = g_signal_new (
		"voice-query-finished", HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
		NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING );

	/**
	 * HudClientQuery::models-changed:
   	 *
	 * Something has caused the models to be changed, you should probably
	 * figure out their state again.
	 */
	hud_client_query_signal_models_changed = g_signal_new (
		HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED, HUD_CLIENT_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
		NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE );

	return;
}

static void
hud_client_query_init (HudClientQuery *self)
{
	self->priv = HUD_CLIENT_QUERY_GET_PRIVATE(self);

	self->priv->toolbar = g_array_new(FALSE, /* zero terminated */
	                                  FALSE, /* clear */
	                                  sizeof(HudClientQueryToolbarItems));

	return;
}

static void
set_property (GObject * obj, guint id, const GValue * value, G_GNUC_UNUSED GParamSpec * pspec)
{
	HudClientQuery * self = HUD_CLIENT_QUERY(obj);

	switch (id) {
	case PROP_CONNECTION:
		g_clear_object(&self->priv->connection);
		self->priv->connection = g_value_dup_object(value);
		break;
	case PROP_QUERY:
		hud_client_query_set_query(self, g_value_get_string(value));
		break;
	default:
		g_warning("Unknown property %d.", id);
		return;
	}

	return;
}

static void
get_property (GObject * obj, guint id, GValue * value, G_GNUC_UNUSED GParamSpec * pspec)
{
	HudClientQuery * self = HUD_CLIENT_QUERY(obj);

	switch (id) {
	case PROP_CONNECTION:
		g_value_set_object(value, self->priv->connection);
		break;
	case PROP_QUERY:
		g_value_set_string(value, self->priv->query);
		break;
	default:
		g_warning("Unknown property %d.", id);
		return;
	}

	return;
}

static void
hud_client_query_voice_query_loading (G_GNUC_UNUSED _HudQueryComCanonicalHudQuery *object, gpointer user_data)
{
	g_signal_emit(user_data, hud_client_query_signal_voice_query_loading, 0);
}

static void
hud_client_query_voice_query_listening (G_GNUC_UNUSED _HudQueryComCanonicalHudQuery *object, gpointer user_data)
{
	g_signal_emit(user_data, hud_client_query_signal_voice_query_listening, 0);
}

static void
hud_client_query_voice_query_heard_something (G_GNUC_UNUSED _HudQueryComCanonicalHudQuery *object, gpointer user_data)
{
  g_signal_emit(user_data, hud_client_query_signal_voice_query_heard_something, 0);
}

static void
hud_client_query_constructed (GObject *object)
{
	HudClientQuery * cquery = HUD_CLIENT_QUERY(object);

	G_OBJECT_CLASS (hud_client_query_parent_class)->constructed (object);

	if (cquery->priv->connection == NULL) {
		cquery->priv->connection = hud_client_connection_get_ref();
	}

	cquery->priv->connection_changed_sig = g_signal_connect(cquery->priv->connection, HUD_CLIENT_CONNECTION_SIGNAL_CONNECTION_STATUS, G_CALLBACK(connection_status), cquery);

	if(cquery->priv->query == NULL) {
		cquery->priv->query = g_strdup("");
	}

	connection_status(cquery->priv->connection, hud_client_connection_connected(cquery->priv->connection), cquery);
	
	return;
}

/* Handles the connection status of the HUD service, once
   we're connected we can do all kinds of fun stuff */
static void
connection_status (G_GNUC_UNUSED HudClientConnection * connection, gboolean connected, HudClientQuery * cquery)
{
	g_clear_object(&cquery->priv->results);
	g_clear_object(&cquery->priv->appstack);
	g_clear_object(&cquery->priv->proxy);

	g_signal_emit(G_OBJECT(cquery), hud_client_query_signal_models_changed, 0);

	if (!connected) {
		return;
	}

	hud_client_connection_new_query(cquery->priv->connection, cquery->priv->query, new_query_cb, g_object_ref(cquery));
	return;
}

/* Go through the toolbar and put the right items in the array */
static void
parse_toolbar (_HudQueryComCanonicalHudQuery * proxy, G_GNUC_UNUSED GParamSpec * paramspec, HudClientQuery * query)
{
	if (query->priv->toolbar->len > 0) {
		g_array_remove_range(query->priv->toolbar, 0, query->priv->toolbar->len);
	}

	const gchar * const * items = NULL;
	items = _hud_query_com_canonical_hud_query_get_toolbar_items(proxy);

	int i;
	for (i = 0; items != NULL && items[i] != NULL; i++) {
		HudClientQueryToolbarItems item = hud_client_query_toolbar_items_get_value_from_nick(items[i]);
		if (item == HUD_CLIENT_QUERY_TOOLBAR_INVALID) continue;
		g_array_append_val(query->priv->toolbar, item);
	}

	g_signal_emit(G_OBJECT(query), signal_toolbar_updated, 0, NULL);

	return;
}

static void
new_query_cb (G_GNUC_UNUSED HudClientConnection * connection, const gchar * path, const gchar * results, const gchar * appstack, gpointer user_data)
{
	if (path == NULL || results == NULL || appstack == NULL) {
		g_object_unref(user_data);
		return;
	}

	HudClientQuery * cquery = HUD_CLIENT_QUERY(user_data);
	GError * error = NULL;

	cquery->priv->proxy = _hud_query_com_canonical_hud_query_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		hud_client_connection_get_address(cquery->priv->connection),
		path,
		NULL, /* GCancellable */
		&error  /* GError */
	);

	if (cquery->priv->proxy == NULL) {
		g_debug("Unable to get proxy after getting query path: %s", error->message);
		g_error_free(error);
		g_object_unref(cquery);
		return;
	}

	/* Set up our models */
	cquery->priv->results = dee_shared_model_new(results);
	dee_model_set_schema_full(cquery->priv->results, results_model_schema, G_N_ELEMENTS(results_model_schema));
	cquery->priv->appstack = dee_shared_model_new(appstack);
	dee_model_set_schema_full(cquery->priv->appstack, appstack_model_schema, G_N_ELEMENTS(appstack_model_schema));

	/* Watch for voice signals */
	g_signal_connect_object (cquery->priv->proxy, "voice-query-loading",
		G_CALLBACK (hud_client_query_voice_query_loading), G_OBJECT(cquery), 0);
	g_signal_connect_object (cquery->priv->proxy, "voice-query-listening",
		G_CALLBACK (hud_client_query_voice_query_listening), G_OBJECT(cquery), 0);
	g_signal_connect_object (cquery->priv->proxy, "voice-query-heard-something",
	    G_CALLBACK (hud_client_query_voice_query_heard_something), G_OBJECT(cquery), 0);

	/* Watch for toolbar callbacks */
	g_signal_connect (cquery->priv->proxy, "notify::toolbar-items", G_CALLBACK(parse_toolbar), cquery);

	/* Figure out toolbar */
	parse_toolbar(cquery->priv->proxy, NULL, cquery);

	g_signal_emit(G_OBJECT(cquery), hud_client_query_signal_models_changed, 0);

	g_object_unref(cquery);

	return;
}

static void
hud_client_query_dispose (GObject *object)
{
	HudClientQuery * self = HUD_CLIENT_QUERY(object);

	/* We don't care anymore, we're dying! */
	if (self->priv->connection_changed_sig != 0) {
		g_signal_handler_disconnect(self->priv->connection, self->priv->connection_changed_sig);
		self->priv->connection_changed_sig = 0;
	}

	if (self->priv->proxy != NULL) {
		_hud_query_com_canonical_hud_query_call_close_query_sync(self->priv->proxy, NULL, NULL);
	}

	g_clear_object(&self->priv->results);
	g_clear_object(&self->priv->appstack);
	g_clear_object(&self->priv->proxy);
	g_clear_object(&self->priv->connection);

	G_OBJECT_CLASS (hud_client_query_parent_class)->dispose (object);
	return;
}

static void
hud_client_query_finalize (GObject *object)
{
	HudClientQuery * self = HUD_CLIENT_QUERY(object);

	g_clear_pointer(&self->priv->query, g_free);
	g_clear_pointer(&self->priv->toolbar, g_array_unref);

	G_OBJECT_CLASS (hud_client_query_parent_class)->finalize (object);
	return;
}

/* Try and start the service using dbus activation.  If we're waiting
   for it to start and send this again, we should be fine. */
static void
dbus_start_service (HudClientQuery * query)
{
	if (query->priv->connection == NULL) {
		/* Oh, we don't even have a connection, let's not try */
		return;
	}

	GDBusConnection * bus = hud_client_connection_get_bus(query->priv->connection);
	if (bus == NULL) {
		/* No bus yet */
		return;
	}

	g_dbus_connection_call (bus,
	                        "org.freedesktop.DBus",
	                        "/",
	                        "org.freedesktop.DBus",
	                        "StartServiceByName",
	                        g_variant_new ("(su)", DBUS_NAME, 0), 
	                        G_VARIANT_TYPE ("(u)"),
	                        G_DBUS_CALL_FLAGS_NONE,
	                        -1,
	                        NULL,
	                        NULL, NULL); /* callback */

	return;
}

/**
 * hud_client_query_new:
 * @query: String to build the initial set of results from
 *
 * Startes a query with the HUD using a specific string.  This
 * will block until the query is created.
 *
 * Return value: (transfer full): A new #HudClientQuery object
 */
HudClientQuery *
hud_client_query_new (const gchar * query)
{
	return HUD_CLIENT_QUERY(g_object_new(HUD_CLIENT_TYPE_QUERY,
		PROP_QUERY_S, query,
		NULL
	));
}

/**
 * hud_client_query_new_for_connection:
 * @query: String to build the initial set of results from
 * @connection: A custom #HudClientConnection to a non-default HUD service
 *
 * Very similar to hud_client_query_new() except that it uses a
 * custom connection.  This is mostly for testing, though it is
 * available if you need it.
 *
 * Return value: (transfer full): A new #HudClientQuery object
 */
HudClientQuery *
hud_client_query_new_for_connection (const gchar * query, HudClientConnection * connection)
{
	return HUD_CLIENT_QUERY(g_object_new(HUD_CLIENT_TYPE_QUERY,
		PROP_CONNECTION_S, connection,
		PROP_QUERY_S, query,
		NULL
	));
}

/**
 * hud_client_query_set_query:
 * @cquery: A #HudClientQuery
 * @query: New query string
 *
 * This revises the query to be the new query string.  Updates can
 * be seen through the #DeeModel's.
 */
void
hud_client_query_set_query (HudClientQuery * cquery, const gchar * query)
{
	g_return_if_fail(HUD_CLIENT_IS_QUERY(cquery));

	g_clear_pointer(&cquery->priv->query, g_free);
	cquery->priv->query = g_strdup(query);

	if (cquery->priv->proxy != NULL) {
		gint revision = 0;
		_hud_query_com_canonical_hud_query_call_update_query_sync(cquery->priv->proxy, cquery->priv->query, &revision, NULL, NULL);
	} else {
		dbus_start_service(cquery);
	}

	g_object_notify(G_OBJECT(cquery), PROP_QUERY_S);

	return;
}

/**
 * hud_client_query_get_query:
 * @cquery: A #HudClientQuery
 * 
 * Accessor for the current query string.
 *
 * Return value: (transfer none): Query string
 */
const gchar *
hud_client_query_get_query (HudClientQuery * cquery)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);

	return cquery->priv->query;
}

static void
hud_client_query_voice_query_callback (G_GNUC_UNUSED GObject *source, GAsyncResult *result, gpointer user_data)
{
	g_assert(HUD_CLIENT_IS_QUERY(user_data));
	HudClientQuery *cquery = HUD_CLIENT_QUERY(user_data);
	gint revision = 0;
	gchar *query = NULL;
	GError *error = NULL;
	if (!_hud_query_com_canonical_hud_query_call_voice_query_finish (cquery->priv->proxy, &revision, &query, result, &error))
	{
		g_warning("Voice query failed to finish: [%s]", error->message);
		g_signal_emit (user_data, hud_client_query_signal_voice_query_failed,
		      0 /* details */, error->message);
		g_error_free(error);
		return;
	}

	g_clear_pointer(&cquery->priv->query, g_free);
	cquery->priv->query = query;
	g_object_notify (G_OBJECT(cquery), PROP_QUERY_S);

	g_signal_emit (user_data, hud_client_query_signal_voice_query_finished,
      0 /* details */, query);
}

/**
 * hud_client_query_voice_query:
 * @cquery: A #HudClientQuery
 *
 * Execute a HUD query using voice recognition.
 *
 * Will cause a series of signals to be emitted indicating progress:
 * - voice-query-loading - the voice recognition toolkit is loading.
 * - voice-query-failed - the voice recognition toolkit has failed to initialize.
 * - voice-query-listening - the voice recognition toolkit is listening to speech.
 * - voice-query-heard-something - the voice recognition toolkit has heard a complete utterance.
 * - voice-query-finished - the voice recognition toolkit has completed, and has a (possibly empty) result.
 */
void
hud_client_query_voice_query (HudClientQuery * cquery)
{
	g_return_if_fail(HUD_CLIENT_IS_QUERY(cquery));

	if (cquery->priv->proxy != NULL) {
		g_debug("Running voice query");
		_hud_query_com_canonical_hud_query_call_voice_query (cquery->priv->proxy, NULL, hud_client_query_voice_query_callback, cquery);
	}
}

/**
 * hud_client_query_get_results_model:
 * @cquery: A #HudClientQuery
 *
 * Accessor for the current results model.
 *
 * Return value: (transfer none): Results Model
 */
DeeModel *
hud_client_query_get_results_model (HudClientQuery * cquery)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);

	return cquery->priv->results;
}

/**
 * hud_client_query_get_appstack_model:
 * @cquery: A #HudClientQuery
 *
 * Accessor for the current appstack model.
 *
 * Return value: (transfer none): Appstack Model
 */
DeeModel *
hud_client_query_get_appstack_model (HudClientQuery * cquery)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);

	return cquery->priv->appstack;
}

/**
 * hud_client_query_toolbar_item_active:
 * @cquery: A #HudClientQuery
 * @item: Item to check for
 *
 * Checks to see if a particular toolbar item is implemented by the
 * application and should be shown to the user as available for use.
 *
 * Return value: Whether this @item is active.
 */
gboolean
hud_client_query_toolbar_item_active (HudClientQuery * cquery, HudClientQueryToolbarItems item)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), FALSE);

	guint i = 0;
	for (i = 0; i < cquery->priv->toolbar->len; i++) {
		HudClientQueryToolbarItems local = g_array_index(cquery->priv->toolbar, HudClientQueryToolbarItems, i);

		if (local == item) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * hud_client_query_get_active_toolbar:
 * @cquery: A #HudClientQuery
 *
 * Gets a list of all the active toolbar items as an array.  Array should be
 * free'd after use.
 *
 * Return value: (transfer full) (element-type HudClientQueryToolbarItems): A
 * list of the active toolbar items.
 */
GArray *
hud_client_query_get_active_toolbar (HudClientQuery * cquery)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);

	return g_array_ref(cquery->priv->toolbar);
}

/**
 * hud_client_query_set_appstack_app:
 * @cquery: A #HudClientQuery
 * @application_id: New application to get results from
 *
 * This revises the query application to be application_id.  Updates can
 * be seen through the #DeeModel's.
 */
void
hud_client_query_set_appstack_app (HudClientQuery *        cquery,
                                   const gchar *           application_id)
{
	g_return_if_fail(HUD_CLIENT_IS_QUERY(cquery));

	if (cquery->priv->proxy != NULL) {
		gint revision = 0;
		_hud_query_com_canonical_hud_query_call_update_app_sync(cquery->priv->proxy, application_id, &revision, NULL, NULL);
	}

	return;
}

/**
 * hud_client_query_execute_command:
 * @cquery: A #HudClientQuery
 * @command_key: The key from the results model for the entry to activate
 * @timestamp: Timestamp for the user event
 *
 * Executes a particular entry from the results model.  The @command_key
 * should be grabbed from the table and passed to this function to activate
 * it.  This function will block until the command is activated.
 */
void
hud_client_query_execute_command (HudClientQuery * cquery, GVariant * command_key, guint timestamp)
{
	g_return_if_fail(HUD_CLIENT_IS_QUERY(cquery));
	g_return_if_fail(command_key != NULL);

	GError *error = NULL;
	if (!_hud_query_com_canonical_hud_query_call_execute_command_sync(cquery->priv->proxy, command_key, timestamp, NULL, &error))
  {
	  g_warning("Error executing command [%s]", error->message);
	  g_error_free(error);
  }

	return;
}

/**
 * hud_client_query_execute_param_command:
 * @cquery: A #HudClientQuery
 * @command_key: The key from the results model for the entry to activate
 * @timestamp: Timestamp for the user event
 *
 * Executes a command that results in a parameterized dialog
 * which is controlled using the returned #HudClientParam object.
 * When created this sends the "opened" event to the application.
 *
 * Return Value: (transfer full): Object to control the parameterized dialog.
 */
HudClientParam *
hud_client_query_execute_param_command (HudClientQuery * cquery, GVariant * command_key, guint timestamp)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(command_key != NULL, NULL);

	gchar * sender = NULL;
	gchar * prefix = NULL;
	gchar * base_action = NULL;
	gchar * action_path = NULL;
	gchar * model_path = NULL;
	gint section = 0;
	GError * error = NULL;

	_hud_query_com_canonical_hud_query_call_execute_parameterized_sync(cquery->priv->proxy, command_key, timestamp, &sender, &prefix, &base_action, &action_path, &model_path, &section, NULL, &error);

	if (error != NULL) {
		g_warning("Unable to execute paramereterized action: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	HudClientParam * param = hud_client_param_new(sender, prefix, base_action, action_path, model_path, section);

	g_free(prefix);
	g_free(sender);
	g_free(base_action);
	g_free(action_path);
	g_free(model_path);

	return param;
}

/**
 * hud_client_query_execute_toolbar_item:
 * @cquery: A #HudClientQuery
 * @item: Which toolbar item is being activated
 * @timestamp: Timestamp for the user event
 *
 * Executes a particular item in the tool bar.  The item should
 * be active before passing this.
 */
void
hud_client_query_execute_toolbar_item (HudClientQuery * cquery, HudClientQueryToolbarItems item, guint timestamp)
{
	g_return_if_fail(HUD_CLIENT_IS_QUERY(cquery));

	_hud_query_com_canonical_hud_query_call_execute_toolbar_sync(cquery->priv->proxy, hud_client_query_toolbar_items_get_nick(item), timestamp, NULL, NULL);

	return;
}

/**
 * hud_client_query_appstack_get_app_id:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the ID from
 *
 * Get the application ID for a given row in the appstack table.
 *
 * Return value: The application ID
 */
const gchar *
hud_client_query_appstack_get_app_id (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_string(cquery->priv->appstack, row, HUD_QUERY_APPSTACK_APPLICATION_ID);
}

/**
 * hud_client_query_appstack_get_app_icon:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the icon from
 *
 * Get the application icon for a given row in the appstack table.
 *
 * Return value: The application icon
 */
const gchar *
hud_client_query_appstack_get_app_icon (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_string(cquery->priv->appstack, row, HUD_QUERY_APPSTACK_ICON_NAME);
}

/**
 * hud_client_query_results_get_command_id:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the ID from
 *
 * Get the command ID for a given row in the results table.
 *
 * Return value: (transfer full): The command ID
 */
GVariant *
hud_client_query_results_get_command_id (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_value(cquery->priv->results, row, HUD_QUERY_RESULTS_COMMAND_ID);
}

/**
 * hud_client_query_results_get_command_name:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the name from
 *
 * Get the human readable command name for a given row in the results table.
 *
 * Return value: The command name
 */
const gchar *
hud_client_query_results_get_command_name (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_string(cquery->priv->results, row, HUD_QUERY_RESULTS_COMMAND_NAME);
}

/**
 * hud_client_query_results_get_command_highlights:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the highlights from
 *
 * Get the command highlights for a row in the table with start and
 * stop characters in an array.
 *
 * Return value: (transfer full): The command highlights as a variant of type "a(ii)"
 */
GVariant *
hud_client_query_results_get_command_highlights (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_value(cquery->priv->results, row, HUD_QUERY_RESULTS_COMMAND_HIGHLIGHTS);
}

/**
 * hud_client_query_results_get_description:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the description from
 *
 * Get the human readable description for the command in the given row in the results table.
 *
 * Return value: The description
 */
const gchar *
hud_client_query_results_get_description (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_string(cquery->priv->results, row, HUD_QUERY_RESULTS_DESCRIPTION);
}

/**
 * hud_client_query_results_get_description_highlights:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the highlights from
 *
 * Get the description highlights for a row in the table with start and
 * stop characters in an array.
 *
 * Return value: (transfer full): The description highlights as a variant of type "a(ii)"
 */
GVariant *
hud_client_query_results_get_description_highlights (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_value(cquery->priv->results, row, HUD_QUERY_RESULTS_DESCRIPTION_HIGHLIGHTS);
}

/**
 * hud_client_query_results_get_shortcut:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to grab the shortcut from
 *
 * Get the human readable shortcut for the command in the given row in the results table.
 *
 * Return value: The shortcut
 */
const gchar *
hud_client_query_results_get_shortcut (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), NULL);
	g_return_val_if_fail(row != NULL, NULL);

	return dee_model_get_string(cquery->priv->results, row, HUD_QUERY_RESULTS_SHORTCUT);
}

/**
 * hud_client_query_results_is_parameterized:
 * @cquery: A #HudClientQuery
 * @row: Which row in the table to check if the command is parameterized
 *
 * Check to see if the given command is parameterized
 *
 * Return value: Whether the command in the row is parameterized
 */
gboolean
hud_client_query_results_is_parameterized (HudClientQuery * cquery, DeeModelIter * row)
{
	g_return_val_if_fail(HUD_CLIENT_IS_QUERY(cquery), FALSE);
	g_return_val_if_fail(row != NULL, FALSE);

	return dee_model_get_bool(cquery->priv->results, row, HUD_QUERY_RESULTS_PARAMETERIZED);
}
