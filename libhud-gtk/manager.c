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
 * Author: Ted Gould <ted@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "manager.h"

#include <gdk/gdkx.h>

struct _HudGtkManagerPrivate {
	/* We'll need this later */
	int dummy;
};

#define HUD_GTK_MANAGER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_GTK_TYPE_MANAGER, HudGtkManagerPrivate))

static void hud_gtk_manager_class_init (HudGtkManagerClass *klass);
static void hud_gtk_manager_init       (HudGtkManager *self);
static void hud_gtk_manager_constructed (GObject *object);
static void hud_gtk_manager_dispose    (GObject *object);
static void hud_gtk_manager_finalize   (GObject *object);
static void window_added (GApplication *application, gpointer window, gpointer user_data);
static void window_removed (GApplication *application, gpointer window, gpointer user_data);

G_DEFINE_TYPE (HudGtkManager, hud_gtk_manager, HUD_TYPE_MANAGER);

/* Initialize Class */
static void
hud_gtk_manager_class_init (HudGtkManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudGtkManagerPrivate));

	object_class->constructed = hud_gtk_manager_constructed;
	object_class->dispose = hud_gtk_manager_dispose;
	object_class->finalize = hud_gtk_manager_finalize;

	return;
}

/* Initialize Instance */
static void
hud_gtk_manager_init (HudGtkManager *self)
{

	return;
}

/* Get all our vars and start building! */
static void
hud_gtk_manager_constructed (GObject *object)
{
	GtkApplication * app = NULL;

	g_object_get(object,
	             HUD_MANAGER_PROP_APPLICATION, &app,
	             NULL);

	g_return_if_fail(GTK_IS_APPLICATION(app));

	g_signal_connect (app, "window-added",
	                  G_CALLBACK (window_added), object);
	g_signal_connect (app, "window-removed",
	                  G_CALLBACK (window_removed), object);

	return;
}

/* Clean up refs */
static void
hud_gtk_manager_dispose (GObject *object)
{

	G_OBJECT_CLASS (hud_gtk_manager_parent_class)->dispose (object);
	return;
}

/* Free Memory */
static void
hud_gtk_manager_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_gtk_manager_parent_class)->finalize (object);
	return;
}

static void
get_window_details (gpointer   window, /* GtkApplicationWindow */
                    gchar    **object_path,
                    guint     *xid)
{
	GApplication *application;
	gpointer gdk_window;

	g_object_get (window,
	              "application", &application,
	              "window", &gdk_window,
	              NULL);
	g_return_if_fail (window != NULL); /* we're inside a realize/unrealize pair... */
	g_return_if_fail (application != NULL); /* otherwise why are we here...? */

	if (GDK_IS_X11_WINDOW (window)) {
		*xid = gdk_x11_window_get_xid(gdk_window);

		if (object_path) {
			guint id;

			id = gtk_application_window_get_id (window);
			*object_path = g_strdup_printf ("%s/window/%u",
			g_application_get_dbus_object_path (application), id);
		}
	/* Put support for other backends here */
	} else {
		/* Fallback to an error */
		g_error ("Unsupported GDK backend");
	}

	return;
}

static void
window_realize (gpointer window, /* GtkWidget */
                gpointer user_data)
{
	HudGtkManager * manager = user_data;
	gchar *object_path;
	guint xid;

	get_window_details (window, &object_path, &xid);

	GVariant * id = g_variant_new_uint32(xid);

	HudActionPublisher * publisher = hud_gtk_manager_get_publisher(manager, id);

	hud_action_publisher_add_action_group (publisher, "win", object_path);

	g_free (object_path);

	return;
}

static void
window_unrealize (gpointer window, /* GtkWidget */
                  gpointer user_data)
{
	HudGtkManager * manager = user_data;
	guint xid;

	get_window_details (window, NULL, &xid);

	GVariant * id = g_variant_new_uint32(xid);

	HudActionPublisher * publisher = hud_gtk_manager_get_publisher(manager, id);

	hud_action_publisher_remove_action_group (publisher, "win", id);

	return;
}

static void
window_added (GApplication *application, /* really GtkApplication */
              gpointer      window,      /* GtkWindow */
              gpointer      user_data)
{
	HudActionPublisher *publisher = user_data;

	/* Can't do anything with this if it's not a GtkApplicationWindow */
	if (!GTK_IS_APPLICATION_WINDOW(window))
		return;

	/* It's not possible that the window is already realized at the time
	 * that it is added to the application, so just watch for the signal.
	 */
	g_signal_connect (window, "realize",
	                  G_CALLBACK (window_realize), publisher);
	g_signal_connect (window, "unrealize",
	                  G_CALLBACK (window_unrealize), publisher);

	return;
}

static void
window_removed (GApplication *application, /* really GtkApplication */
                gpointer      window,      /* GtkWindow */
                gpointer      user_data)
{
	HudActionPublisher *publisher = user_data;

	/* Just indiscriminately disconnect....
	 * If we didn't connect anything then nothing will happen.
	 *
	 * We're probably well on our way to destruction at this point, but it
	 * can't hurt to clean up properly just incase someone is doing
	 * something weird.
	 */
	g_signal_handlers_disconnect_by_func (window, window_realize, publisher);
	g_signal_handlers_disconnect_by_func (window, window_unrealize, publisher);

	return;
}

/**
 * hud_gtk_manager_new:
 * @app: A #GtkApplication object
 *
 * Creates a #HudGtkManager object that is connected to the @app so that
 * new windows get tracked and their actions automatically added to so
 * the HUD can access them.
 *
 * From the #GtkApplication passed as @app any #GtkApplicationWindow
 * added to the application will also be added as a potential target
 * ("win") for actions.  For example, if a #GtkApplicationWindow
 * features an action "fullscreen" then action descriptions can speak of
 * "win.fullscreen".
 *
 * @app must have no windows at the time that this function is
 * called.
 *
 * Return value: (transfer full): A new #HudGtkManager object
 */
HudGtkManager *
hud_gtk_manager_new (GtkApplication * app)
{
	g_return_val_if_fail(GTK_IS_APPLICATION(app), NULL);

	return g_object_new(HUD_GTK_TYPE_MANAGER,
	                    HUD_MANAGER_PROP_APPLICATION, app,
	                    NULL);
}

/**
 * hud_gtk_manager_get_publisher:
 * @manager: A #HudGtkManager object
 * @id: ID of the item to find the publisher for
 *
 * Finds or creates a publisher for the specific #GtkApplicationWindow that
 * is referenced by @id.  This can be used to add descriptions on the publisher
 * or additional action groups as needed.
 *
 * Return value: (transfer none): A #HudActionPublisher for the window
 */
HudActionPublisher *
hud_gtk_manager_get_publisher (HudGtkManager * manager, GVariant * id)
{
	/* TODO: Uhm, yes */
	return NULL;
}
