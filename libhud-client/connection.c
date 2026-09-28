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

#include "connection.h"
#include "service-iface.h"

#include "common/shared-values.h"

struct _HudClientConnectionPrivate {
	_HudServiceComCanonicalHud * proxy;
	GDBusConnection * bus;
	gchar * address;
	gchar * path;
	gboolean connected;
	gulong name_owner_sig;
	GCancellable * cancellable;
};

#define HUD_CLIENT_CONNECTION_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_CLIENT_TYPE_CONNECTION, HudClientConnectionPrivate))

enum {
	PROP_0 = 0,
	PROP_ADDRESS,
	PROP_PATH
};

#define PROP_ADDRESS_S  "address"
#define PROP_PATH_S     "path"

static void hud_client_connection_class_init (HudClientConnectionClass *klass);
static void hud_client_connection_init       (HudClientConnection *self);
static void hud_client_connection_constructed (GObject *object);
static void hud_client_connection_dispose    (GObject *object);
static void hud_client_connection_finalize   (GObject *object);
static void set_property (GObject * obj, guint id, const GValue * value, GParamSpec * pspec);
static void get_property (GObject * obj, guint id, GValue * value, GParamSpec * pspec);
static void name_owner_changed (GObject * object, GParamSpec * pspec, gpointer user_data);

G_DEFINE_TYPE (HudClientConnection, hud_client_connection, G_TYPE_OBJECT)

static guint signal_connection_status = 0;

static void
hud_client_connection_class_init (HudClientConnectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudClientConnectionPrivate));

	object_class->dispose = hud_client_connection_dispose;
	object_class->finalize = hud_client_connection_finalize;
	object_class->constructed = hud_client_connection_constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	g_object_class_install_property (object_class, PROP_ADDRESS,
	                                 g_param_spec_string(PROP_ADDRESS_S, "Address on DBus for the HUD service",
	                                              "The DBus address of the HUD service we should connect to.",
	                                              DBUS_NAME,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_PATH,
	                                 g_param_spec_string(PROP_PATH_S, "Path on DBus for the HUD service",
	                                              "The DBus path of the HUD service we should connect to.",
	                                              DBUS_PATH,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * HudClientConnection::connection-status:
	 *
	 * Called when the connection status changes in some way.
	 */
	signal_connection_status = g_signal_new (HUD_CLIENT_CONNECTION_SIGNAL_CONNECTION_STATUS,
	                                         HUD_CLIENT_TYPE_CONNECTION,
	                                         G_SIGNAL_RUN_LAST,
	                                         0, /* offset */
	                                         NULL, NULL, /* Collectors */
	                                         g_cclosure_marshal_VOID__BOOLEAN,
	                                         G_TYPE_NONE, 1, G_TYPE_BOOLEAN);


	return;
}

static void
hud_client_connection_init (HudClientConnection *self)
{
	self->priv = HUD_CLIENT_CONNECTION_GET_PRIVATE(self);
	self->priv->connected = FALSE;
	self->priv->cancellable = g_cancellable_new();

	GError * error = NULL;
	self->priv->bus = g_bus_get_sync(G_BUS_TYPE_SESSION, self->priv->cancellable, &error);

	if (G_UNLIKELY(error != NULL)) { /* really should never happen */
		g_warning("Unable to get session bus: %s", error->message);
		g_error_free(error);
	}

	return;
}

static void
set_property (GObject * obj, guint id, const GValue * value, G_GNUC_UNUSED GParamSpec * pspec)
{
	HudClientConnection * self = HUD_CLIENT_CONNECTION(obj);

	switch (id) {
	case PROP_ADDRESS:
		g_clear_pointer(&self->priv->address, g_free);
		self->priv->address = g_value_dup_string(value);
		break;
	case PROP_PATH:
		g_clear_pointer(&self->priv->path, g_free);
		self->priv->path = g_value_dup_string(value);
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
	HudClientConnection * self = HUD_CLIENT_CONNECTION(obj);

	switch (id) {
	case PROP_ADDRESS:
		g_value_set_string(value, self->priv->address);
		break;
	case PROP_PATH:
		g_value_set_string(value, self->priv->path);
		break;
	default:
		g_warning("Unknown property %d.", id);
		return;
	}

	return;
}

static void
hud_client_connection_constructed (GObject * object)
{
	HudClientConnection * self = HUD_CLIENT_CONNECTION(object);

	g_return_if_fail(self->priv->address != NULL);
	g_return_if_fail(self->priv->path != NULL);

	GError * error = NULL;
	self->priv->proxy = _hud_service_com_canonical_hud_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
		self->priv->address,
		self->priv->path,
		self->priv->cancellable,
		&error
	);

	if (error != NULL) {
		g_warning("Unable to get a HUD proxy: %s", error->message);
		self->priv->proxy = NULL;
		g_error_free(error); error = NULL;
	}

	if (self->priv->proxy != NULL) {
		self->priv->name_owner_sig = g_signal_connect(G_OBJECT(self->priv->proxy), "notify::g-name-owner", G_CALLBACK(name_owner_changed), self);
	}

	name_owner_changed(NULL, NULL, self);

	return;
}

static void
hud_client_connection_dispose (GObject *object)
{
	HudClientConnection * self = HUD_CLIENT_CONNECTION(object);

	if (self->priv->cancellable != NULL) {
		g_cancellable_cancel(self->priv->cancellable);
		g_clear_object(&self->priv->cancellable);
	}

	if (self->priv->name_owner_sig != 0) {
		g_signal_handler_disconnect(self->priv->proxy, self->priv->name_owner_sig);
		self->priv->name_owner_sig = 0;
	}

	g_clear_object(&self->priv->proxy);
	g_clear_object(&self->priv->bus);

	G_OBJECT_CLASS (hud_client_connection_parent_class)->dispose (object);
	return;
}

static void
hud_client_connection_finalize (GObject *object)
{
	HudClientConnection * self = HUD_CLIENT_CONNECTION(object);

	g_clear_pointer(&self->priv->address, g_free);
	g_clear_pointer(&self->priv->path, g_free);

	G_OBJECT_CLASS (hud_client_connection_parent_class)->finalize (object);
	return;
}

/* Called when the HUD service comes on or off the bus */
static void
name_owner_changed (G_GNUC_UNUSED GObject * object, G_GNUC_UNUSED GParamSpec * pspec, gpointer user_data)
{
	HudClientConnection * self = HUD_CLIENT_CONNECTION(user_data);
	gboolean connected = FALSE;
	gchar * owner = NULL;

	if (self->priv->proxy != NULL) {
		owner = g_dbus_proxy_get_name_owner(G_DBUS_PROXY(self->priv->proxy));
	}

	if (owner != NULL) {
		connected = TRUE;
		g_free(owner);
	}

	/* Make sure we set the internal variable before signaling */
	gboolean change = (connected != self->priv->connected);
	self->priv->connected = connected;

	/* Cancel anything we had running */
	if (!self->priv->connected) {
		g_cancellable_cancel(self->priv->cancellable);
	} else {
		g_cancellable_reset(self->priv->cancellable);
	}

	/* If there was a change, make sure others know about it */
	if (change) {
		g_signal_emit(self, signal_connection_status, 0, connected);
	}

	return;
}

/**
 * hud_client_connection_get_ref:
 *
 * Gets a reference to the default object that connects to the
 * default HUD service.
 *
 * Return value: (transfer full): Refence to a #HudClientConnection
 */
HudClientConnection *
hud_client_connection_get_ref (void)
{
	static HudClientConnection * global = NULL;

	if (global == NULL) {
		global = HUD_CLIENT_CONNECTION(g_object_new(HUD_CLIENT_TYPE_CONNECTION, NULL));
		g_object_add_weak_pointer(G_OBJECT(global), (gpointer *)&global);
		return global;
	} else {
		return g_object_ref(global);
	}
}

/**
 * hud_client_connection_new:
 * @dbus_address: Address on DBus for the HUD service
 * @dbus_path: Path to the object to create stuff
 *
 * Builds a HUD Connection object that can be used to connect to a
 * custom HUD service.  For the most part, this should only be used
 * in testing, though there might be other uses.  It is likely if you're
 * using this function you'd also be interested in
 * hud_client_query_new_for_connection()
 *
 * Return value: (transfer full): A new #HudClientConnection
 */
HudClientConnection *
hud_client_connection_new (const gchar * dbus_address, const gchar * dbus_path)
{
	return HUD_CLIENT_CONNECTION(g_object_new(HUD_CLIENT_TYPE_CONNECTION,
			PROP_ADDRESS_S, dbus_address,
			PROP_PATH_S, dbus_path,
			NULL));
}

/* Data to handle the callback */
typedef struct _new_query_data_t new_query_data_t;
struct _new_query_data_t {
	HudClientConnection * con;
	HudClientConnectionNewQueryCallback cb;
	gpointer user_data;
};

/* Called when the new query call comes back */
static void
new_query_complete (GObject * object, GAsyncResult * res, gpointer user_data)
{
	new_query_data_t * data = (new_query_data_t *)user_data;

	gchar * query_object = NULL;
	gchar * results_name = NULL;
	gchar * appstack_name = NULL;
	gint revision = 0;
	GError * error = NULL;

	_hud_service_com_canonical_hud_call_create_query_finish((_HudServiceComCanonicalHud *)object,
	                                                        &query_object,
	                                                        &results_name,
	                                                        &appstack_name,
	                                                        &revision,
	                                                        res,
	                                                        &error);

	if (error != NULL) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED) && 
				!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CLOSED)) {
			g_warning("Unable to allocate query: %s", error->message);
		}
		g_error_free(error);
		return;
	}

	data->cb(data->con, query_object, results_name, appstack_name, data->user_data);

	g_free(data);
	g_free(query_object);
	g_free(results_name);
	g_free(appstack_name);

	return;
}

/**
 * hud_client_connection_new_query:
 * @connection: A #HudClientConnection
 * @query: The initial query string
 * @cb: Callback when we've got the query
 * @user_data: Data to pass to the callback
 *
 * Function to create a new query in the HUD service and pass back
 * the information needed to create a #HudClientQuery object.
 */
void
hud_client_connection_new_query (HudClientConnection * connection, const gchar * query, HudClientConnectionNewQueryCallback cb, gpointer user_data)
{
	g_return_if_fail(HUD_CLIENT_IS_CONNECTION(connection));

	new_query_data_t * data = g_new0(new_query_data_t, 1);
	data->con = connection;
	data->cb = cb;
	data->user_data = user_data;

	_hud_service_com_canonical_hud_call_create_query(connection->priv->proxy,
		query,
		connection->priv->cancellable,
		new_query_complete,
		data);
}

/**
 * hud_client_connection_get_address:
 * @connection: A #HudClientConnection
 *
 * Accessor to get the address of the HUD service.
 *
 * Return value: A DBus address
 */
const gchar *
hud_client_connection_get_address (HudClientConnection * connection)
{
	g_return_val_if_fail(HUD_CLIENT_IS_CONNECTION(connection), NULL);

	return connection->priv->address;
}

/**
 * hud_client_connection_connected:
 * @connection: A #HudClientConnection
 *
 * Accessor to get the connected status of the connection
 *
 * Return value: If we're connected or not
 */
gboolean
hud_client_connection_connected (HudClientConnection * connection)
{
	g_return_val_if_fail(HUD_CLIENT_IS_CONNECTION(connection), FALSE);
	return connection->priv->connected;
}

/**
 * hud_client_connection_get_bus:
 * @connection: A #HudClientConnection
 *
 * Grab the bus being used by the connection.
 *
 * Return value: (transfer none): Our bus if we have one
 */
GDBusConnection *
hud_client_connection_get_bus (HudClientConnection * connection)
{
	g_return_val_if_fail(HUD_CLIENT_IS_CONNECTION(connection), NULL);
	return connection->priv->bus;
}

