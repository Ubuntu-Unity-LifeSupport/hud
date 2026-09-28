/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of either or both of the following licences:
 *
 * 1) the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation; and/or
 * 2) the GNU Lesser General Public License version 2.1, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 and version 2.1 along with this program.  If not,
 * see <http://www.gnu.org/licenses/>
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "manager.h"
#include "action-publisher.h"
#include "service-iface.h"
#include "app-iface.h"

struct _HudManagerPrivate {
	gchar * application_id;

	GApplication * application;
	HudActionPublisher * app_pub;

	GCancellable * connection_cancel;
	GDBusConnection * session;
	_HudServiceIfaceComCanonicalHud * service_proxy;
	_HudAppIfaceComCanonicalHudApplication * app_proxy;

	GVariantBuilder * todo_add_acts;
	GVariantBuilder * todo_add_desc;
	GHashTable      * todo_active_contexts;
	guint todo_idle;

	GList * publishers;
	GHashTable * active_contexts;
};

#define HUD_MANAGER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_TYPE_MANAGER, HudManagerPrivate))

enum {
	PROP_0 = 0,
	PROP_APP_ID,
	PROP_APPLICATION
};

static void hud_manager_class_init (HudManagerClass *klass);
static void hud_manager_init       (HudManager *self);
static void hud_manager_constructed (GObject *object);
static void hud_manager_dispose    (GObject *object);
static void hud_manager_finalize   (GObject *object);
static void set_property (GObject * obj, guint id, const GValue * value, GParamSpec * pspec);
static void get_property (GObject * obj, guint id, GValue * value, GParamSpec * pspec);
static void bus_get_cb (GObject * obj, GAsyncResult * res, gpointer user_data);

G_DEFINE_TYPE (HudManager, hud_manager, G_TYPE_OBJECT)

/* Initialize Class */
static void
hud_manager_class_init (HudManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudManagerPrivate));

	object_class->constructed = hud_manager_constructed;
	object_class->dispose = hud_manager_dispose;
	object_class->finalize = hud_manager_finalize;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	g_object_class_install_property (object_class, PROP_APP_ID,
	                                 g_param_spec_string(HUD_MANAGER_PROP_APP_ID, "ID for the application, typically the desktop file name",
	                                              "A unique identifier for the application.  Usually this aligns with the desktop file name or package name of the app.",
	                                              NULL,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_APPLICATION,
	                                 g_param_spec_object(HUD_MANAGER_PROP_APPLICATION, "Instance of #GApplication if used for this application",
	                                              "GApplication object",
	                                              G_TYPE_APPLICATION,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	return;
}

/* Initialize Instance */
static void
hud_manager_init (HudManager *self)
{
	self->priv = HUD_MANAGER_GET_PRIVATE(self);

	self->priv->connection_cancel = g_cancellable_new();

	g_bus_get(G_BUS_TYPE_SESSION,
	          self->priv->connection_cancel,
	          bus_get_cb,
	          self);

	self->priv->todo_active_contexts = g_hash_table_new_full(g_direct_hash,
								 g_direct_equal,
								 NULL,
								 g_object_unref);
	self->priv->active_contexts = g_hash_table_new_full(g_direct_hash,
						       g_direct_equal,
						       NULL,
						       g_object_unref);

	return;
}

/* Construct the object */
static void
hud_manager_constructed (GObject * object)
{
	HudManager * manager = HUD_MANAGER(object);

	manager->priv->todo_idle = 0;

	if (manager->priv->application) {
		manager->priv->app_pub = hud_action_publisher_new(HUD_ACTION_PUBLISHER_ALL_WINDOWS, HUD_ACTION_PUBLISHER_NO_CONTEXT);

		hud_action_publisher_add_action_group(manager->priv->app_pub, "app", g_application_get_dbus_object_path (manager->priv->application));
		hud_manager_add_actions(manager, manager->priv->app_pub);
	}

	return;
}

/* Clearing out a builder by finishing and freeing the results */
static void
variant_builder_dispose (gpointer user_data)
{
	if (user_data == NULL) {
		return;
	}

	GVariantBuilder * builder;
	builder = (GVariantBuilder *)user_data;

	GVariant * output;
	output = g_variant_builder_end(builder);

	g_variant_ref_sink(output);
	g_variant_unref(output);

	g_variant_builder_unref(builder);
	return;
}

/* Clean up refs */
static void
hud_manager_dispose (GObject *object)
{
	HudManager * manager = HUD_MANAGER(object);

	if (manager->priv->connection_cancel) {
		g_cancellable_cancel(manager->priv->connection_cancel);
		g_clear_object(&manager->priv->connection_cancel);
	}

	g_clear_pointer(&manager->priv->todo_add_acts, variant_builder_dispose);
	g_clear_pointer(&manager->priv->todo_add_desc, variant_builder_dispose);

	if (manager->priv->todo_idle != 0) {
		g_source_remove(manager->priv->todo_idle);
		manager->priv->todo_idle = 0;
	}

	g_clear_object(&manager->priv->session);
	g_clear_object(&manager->priv->service_proxy);
	g_clear_object(&manager->priv->app_proxy);

	g_list_free_full(manager->priv->publishers, g_object_unref);
	g_hash_table_remove_all(manager->priv->todo_active_contexts);
	g_hash_table_remove_all(manager->priv->active_contexts);

	g_clear_object(&manager->priv->app_pub);
	g_clear_object(&manager->priv->application);

	G_OBJECT_CLASS (hud_manager_parent_class)->dispose (object);
	return;
}

/* Free Memory */
static void
hud_manager_finalize (GObject *object)
{
	HudManager * manager = HUD_MANAGER(object);

	g_clear_pointer(&manager->priv->application_id, g_free);

	g_clear_pointer(&manager->priv->todo_active_contexts, g_hash_table_destroy);
	g_clear_pointer(&manager->priv->active_contexts, g_hash_table_destroy);

	G_OBJECT_CLASS (hud_manager_parent_class)->finalize (object);
	return;
}

/* Standard setting of props */
static void
set_property (GObject * obj, guint id, const GValue * value, G_GNUC_UNUSED GParamSpec * pspec)
{
	HudManager * manager = HUD_MANAGER(obj);

	switch (id) {
	case PROP_APP_ID:
		if (manager->priv->application == NULL) {
			g_clear_pointer(&manager->priv->application_id, g_free);
			manager->priv->application_id = g_value_dup_string(value);
		} else {
			g_debug("Application ID being set on HUD Manager already initialized with a GApplication");
		}
		break;
	case PROP_APPLICATION:
		g_clear_object(&manager->priv->application);

		if (g_value_get_object(value) != NULL) {
			g_clear_pointer(&manager->priv->application_id, g_free);

			manager->priv->application = g_value_dup_object(value);
			manager->priv->application_id = g_strdup(g_application_get_application_id(manager->priv->application));
		}
		break;
	default:
		g_warning("Unknown property %d.", id);
		return;
	}

	return;
}

/* Standard getting of props */
static void
get_property (GObject * obj, guint id, GValue * value, G_GNUC_UNUSED GParamSpec * pspec)
{
	HudManager * manager = HUD_MANAGER(obj);

	switch (id) {
	case PROP_APP_ID:
		g_value_set_string(value, manager->priv->application_id);
		break;
	case PROP_APPLICATION:
		g_value_set_object(value, manager->priv->application);
		break;
	default:
		g_warning("Unknown property %d.", id);
		return;
	}

	return;
}

/* Callback from adding sources */
static void
add_sources_cb (GObject * obj, GAsyncResult * res, G_GNUC_UNUSED gpointer user_data)
{
	GError * error = NULL;
	_hud_app_iface_com_canonical_hud_application_call_add_sources_finish((_HudAppIfaceComCanonicalHudApplication *)obj, res, &error);

	if (error != NULL) {
		g_warning("Unable to add sources: %s", error->message);
		g_error_free(error);

		/* TODO: Handle error */
		return;
	}

	return;
}

static gboolean
activate_todo_context (G_GNUC_UNUSED gpointer key,
		       gpointer value,
                       gpointer user_data)
{
	HudManager * manager = HUD_MANAGER(user_data);
	HudActionPublisher *pub = HUD_ACTION_PUBLISHER(value);
	hud_manager_switch_window_context(manager, pub);
	return TRUE;
}

/* Take the todo queues and make them into DBus messages */
static void
process_todo_queues (HudManager * manager)
{
	if (manager->priv->todo_add_acts == NULL
	    && manager->priv->todo_add_desc == NULL
	    && g_hash_table_size(manager->priv->todo_active_contexts) == 0) {
		/* Nothing to process */
		return;
	}

	if (manager->priv->app_proxy == NULL) {
		g_warning("Can't process TODO queues without an application proxy");
		return;
	}

	GVariant * actions = NULL;
	GVariant * descriptions = NULL;

	/* Build an actions list */
	if (manager->priv->todo_add_acts != NULL) {
		actions = g_variant_builder_end(manager->priv->todo_add_acts);
		g_variant_builder_unref(manager->priv->todo_add_acts);
		manager->priv->todo_add_acts = NULL;
	} else {
		actions = g_variant_new_array(G_VARIANT_TYPE("(usso)"), NULL, 0);
	}

	/* Build a descriptions list */
	if (manager->priv->todo_add_desc != NULL) {
		descriptions = g_variant_builder_end(manager->priv->todo_add_desc);
		g_variant_builder_unref(manager->priv->todo_add_desc);
		manager->priv->todo_add_desc = NULL;
	} else {
		descriptions = g_variant_new_array(G_VARIANT_TYPE("(uso)"), NULL, 0);
	}

	/* Should never happen, but let's get useful error messages if it does */
	g_return_if_fail(actions != NULL);
	g_return_if_fail(descriptions != NULL);

	_hud_app_iface_com_canonical_hud_application_call_add_sources(manager->priv->app_proxy,
		actions,
		descriptions,
		NULL, /* cancelable */
		add_sources_cb,
		manager);

	g_hash_table_foreach_remove(manager->priv->todo_active_contexts,
				    activate_todo_context,
				    manager);

	return;
}

/* Application proxy callback */
static void
application_proxy_cb (G_GNUC_UNUSED GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	_HudAppIfaceComCanonicalHudApplication * proxy = _hud_app_iface_com_canonical_hud_application_proxy_new_finish(res, &error);

	if (error != NULL) {
		g_warning("Unable to get app proxy: %s", error->message);
		g_error_free(error);
		return;
	}

	HudManager * manager = HUD_MANAGER(user_data);
	manager->priv->app_proxy = proxy;

	g_clear_object(&manager->priv->connection_cancel);

	process_todo_queues(manager);

	return;
}

/* Callback from getting the HUD service proxy */
static void
register_app_cb (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	gchar * object = NULL;

	_hud_service_iface_com_canonical_hud_call_register_application_finish ((_HudServiceIfaceComCanonicalHud *)obj,
		&object,
		res,
		&error);

	if (error != NULL) {
		g_warning("Unable to register app: %s", error->message);
		g_error_free(error);
		return;
	}

	HudManager * manager = HUD_MANAGER(user_data);
	_hud_app_iface_com_canonical_hud_application_proxy_new(manager->priv->session,
		G_DBUS_PROXY_FLAGS_NONE,
		"com.canonical.hud",
		object,
		manager->priv->connection_cancel,
		application_proxy_cb,
		manager);

	g_free(object);

	return;
}

static gboolean
insert_active_context_to_todo (G_GNUC_UNUSED gpointer key,
			       gpointer value,
			       gpointer user_data)
{
	HudManager * manager = HUD_MANAGER(user_data);
	HudActionPublisher *pub = HUD_ACTION_PUBLISHER(value);
	g_hash_table_insert(manager->priv->todo_active_contexts,
			    GUINT_TO_POINTER(hud_action_publisher_get_window_id(pub)),
			    g_object_ref(pub));
	return TRUE;
}

/* Watch the name change to make sure we're robust to it */
static void
notify_name_owner (GObject * gobject, G_GNUC_UNUSED GParamSpec * pspec, gpointer user_data)
{
	HudManager * manager = HUD_MANAGER(user_data);
	gchar * name_owner = g_dbus_proxy_get_name_owner(G_DBUS_PROXY(gobject));

	if (name_owner == NULL) {
		/* We've lost it */

		/* Remove the stale proxy */
		g_clear_object(&manager->priv->app_proxy);

		/* Make sure we're set upto do the async dance again */
		if (manager->priv->connection_cancel == NULL) {
			manager->priv->connection_cancel = g_cancellable_new();
		}

		/* Put the current publishers on the todo list */
		GList * old_publishers = manager->priv->publishers;
		manager->priv->publishers = NULL;
		GList * pub;

		for (pub = old_publishers; pub != NULL; pub = g_list_next(pub)) {
			hud_manager_add_actions(manager, HUD_ACTION_PUBLISHER(pub->data));
		}

		g_list_free_full(old_publishers, g_object_unref);

		g_hash_table_foreach_remove(manager->priv->active_contexts,
					    insert_active_context_to_todo,
					    manager);
		return;
	}

	g_free(name_owner);

	/* We've got a new name owner, so we need to register */
	_hud_service_iface_com_canonical_hud_call_register_application(manager->priv->service_proxy,
		manager->priv->application_id,
		manager->priv->connection_cancel,
		register_app_cb,
		manager);

	return;
}

/* Callback from getting the HUD service proxy */
static void
service_proxy_cb (G_GNUC_UNUSED GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	_HudServiceIfaceComCanonicalHud * proxy = _hud_service_iface_com_canonical_hud_proxy_new_finish(res, &error);

	if (error != NULL) {
		g_critical("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return;
	}

	HudManager * manager = HUD_MANAGER(user_data);
	manager->priv->service_proxy = proxy;

	g_signal_connect(G_OBJECT(proxy), "notify::g-name-owner", G_CALLBACK(notify_name_owner), manager);
	notify_name_owner(G_OBJECT(proxy), NULL, manager);

	return;
}

/* Callback from getting the session bus */
static void
bus_get_cb (G_GNUC_UNUSED GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GDBusConnection * con = g_bus_get_finish(res, &error);

	if (error != NULL) {
		g_critical("Unable to get session bus: %s", error->message);
		g_error_free(error);
		return;
	}

	HudManager * manager = HUD_MANAGER(user_data);
	manager->priv->session = con;

	_hud_service_iface_com_canonical_hud_proxy_new(con,
	                                         G_DBUS_PROXY_FLAGS_NONE,
	                                         "com.canonical.hud",
	                                         "/com/canonical/hud",
	                                         manager->priv->connection_cancel,
	                                         service_proxy_cb,
	                                         manager);

	return;
}

/**
 * hud_manager_new:
 * @application_id: Unique identifier of the application, usually desktop file name
 *
 * Creates a new #HudManager object.
 *
 * Return value: (transfer full): New #HudManager
 */
HudManager *
hud_manager_new (const gchar * application_id)
{
	g_return_val_if_fail(application_id != NULL, NULL);

	return g_object_new(HUD_TYPE_MANAGER,
	                    HUD_MANAGER_PROP_APP_ID, application_id,
	                    NULL);
}

/**
 * hud_manager_new_for_application:
 * @application: #GApplication that we can use to get the ID and some actions from
 *
 * Creates a new #HudManager object using the application ID in th
 * @application object.  Also exports the default actions there in
 * the "app" namespace.
 *
 * Return value: (transfer full): New #HudManager
 */
HudManager *
hud_manager_new_for_application (GApplication * application)
{
	g_return_val_if_fail(G_IS_APPLICATION(application), NULL);

	return g_object_new(HUD_TYPE_MANAGER,
	                    HUD_MANAGER_PROP_APPLICATION, application,
	                    NULL);
}

/* Idle handler */
static gboolean
todo_handler (gpointer user_data)
{
	HudManager * manager = HUD_MANAGER(user_data);

	process_todo_queues(manager);

	manager->priv->todo_idle = 0;

	return FALSE;
}

/**
 * hud_manager_add_actions:
 * @manager: A #HudManager object
 * @pub: Action publisher object tracking the descriptions and action groups
 *
 * Sets up a set of actions and descriptions for a specific user
 * context.  This could be a window or a tab, depending on how the
 * application works.
 */
void
hud_manager_add_actions (HudManager * manager, HudActionPublisher * pub)
{
	g_return_if_fail(HUD_IS_MANAGER(manager));
	g_return_if_fail(HUD_IS_ACTION_PUBLISHER(pub));

	/* Add to our list of publishers */
	manager->priv->publishers = g_list_prepend(manager->priv->publishers, g_object_ref(pub));

	/* Set up watching for new groups */
	/* TODO */

	/* Grab the window and context IDs for each of them */
	GVariant * winid = g_variant_new_uint32(hud_action_publisher_get_window_id(pub));
	GVariant * conid = g_variant_new_string(hud_action_publisher_get_context_id(pub));
	g_variant_ref_sink(winid);
	g_variant_ref_sink(conid);

	/* Send the current groups out */
	GList * ags_list = hud_action_publisher_get_action_groups(pub);

	/* Build the variant builder if it doesn't exist */
	if (ags_list != NULL && manager->priv->todo_add_acts == NULL) {
		manager->priv->todo_add_acts = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
	}

	/* Grab the action groups and add them to the todo queue */
	GList * paction_group = NULL;
	for (paction_group = ags_list; paction_group != NULL; paction_group = g_list_next(paction_group)) {
		HudActionPublisherActionGroupSet * set = (HudActionPublisherActionGroupSet *)paction_group->data;

		g_variant_builder_open(manager->priv->todo_add_acts, G_VARIANT_TYPE_TUPLE);

		g_variant_builder_add_value(manager->priv->todo_add_acts, winid);
		g_variant_builder_add_value(manager->priv->todo_add_acts, conid);
		g_variant_builder_add_value(manager->priv->todo_add_acts, g_variant_new_string(set->prefix));
		g_variant_builder_add_value(manager->priv->todo_add_acts, g_variant_new_object_path(set->path));

		g_variant_builder_close(manager->priv->todo_add_acts);
	}

	/* Grab the description and add them to the todo queue */
	const gchar * descpath = hud_action_publisher_get_description_path(pub);

	/* Build the variant builder if it doesn't exist */
	if (descpath != NULL && manager->priv->todo_add_desc == NULL) {
		manager->priv->todo_add_desc = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
	}

	if (descpath != NULL) {
		g_variant_builder_open(manager->priv->todo_add_desc, G_VARIANT_TYPE_TUPLE);

		g_variant_builder_add_value(manager->priv->todo_add_desc, winid);
		g_variant_builder_add_value(manager->priv->todo_add_desc, conid);
		g_variant_builder_add_value(manager->priv->todo_add_desc, g_variant_new_object_path(descpath));

		g_variant_builder_close(manager->priv->todo_add_desc);
	}

	/* Should be if we're all set up */
	if (manager->priv->connection_cancel == NULL && manager->priv->todo_idle == 0) {
		manager->priv->todo_idle = g_idle_add(todo_handler, manager);
	}

	g_variant_unref(winid);  winid = NULL;
	g_variant_unref(conid);  conid = NULL;

	return;
}

/**
 * hud_manager_remove_actions:
 * @manager: A #HudManager object
 * @pub: Action publisher object tracking the descriptions and action groups
 *
 * Removes actions for being watched by the HUD.  Should be done when the object
 * is remove.  Does not require @pub to be a valid object so it can be used
 * with weak pointer style destroy.
 */
void
hud_manager_remove_actions (HudManager * manager, G_GNUC_UNUSED HudActionPublisher * pub)
{
	g_return_if_fail(HUD_IS_MANAGER(manager));

	/* TODO: We need DBus API for this */
	/* TODO: make sure to check if the removed publisher was in the active_contexts
	         or todo_active_contexts */

	return;
}

/* Callback from setting the window context.  Not much we can do, just
   reporting errors */
static void
set_window_context_cb (G_GNUC_UNUSED GObject * obj, GAsyncResult *res, G_GNUC_UNUSED gpointer user_data)
{
	GError * error = NULL;

	_hud_app_iface_com_canonical_hud_application_call_set_window_context_finish((_HudAppIfaceComCanonicalHudApplication *)obj, res, &error);
	if (error != NULL) {
		g_warning("Unable to set context for window: %s", error->message);
		g_error_free(error);
	}

	return;
}

/**
 * hud_manager_switch_window_context:
 * @manager: A #HudManager object
 * @pub: Action publisher object tracking the descriptions and action groups
 *
 * Tells the HUD service that a window should use a different context of
 * actions with the current window.  This allows the application to export
 * sets of actions and switch them easily with a single dbus message.
 */
void
hud_manager_switch_window_context (HudManager * manager, HudActionPublisher * pub)
{
	g_return_if_fail(HUD_IS_MANAGER(manager));
	g_return_if_fail(HUD_IS_ACTION_PUBLISHER(pub));

	if (manager->priv->app_proxy == NULL) {
		g_debug("Unable to send context change now, caching for reconnection");
		g_hash_table_insert(manager->priv->todo_active_contexts,
				    GUINT_TO_POINTER(hud_action_publisher_get_window_id(pub)),
				    g_object_ref(pub));
		return;
	}


	g_hash_table_insert(manager->priv->active_contexts,
			    GUINT_TO_POINTER(hud_action_publisher_get_window_id(pub)),
			    g_object_ref(pub));
	_hud_app_iface_com_canonical_hud_application_call_set_window_context(manager->priv->app_proxy,
		hud_action_publisher_get_window_id(pub),
		hud_action_publisher_get_context_id(pub),
		NULL, /* cancellable */
		set_window_context_cb,
		NULL);

	return;
}
