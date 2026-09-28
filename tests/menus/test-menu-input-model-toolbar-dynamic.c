/*
Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <glib-object.h>
#include <gio/gio.h>

GMenu * menu = NULL;
GSimpleActionGroup * ag = NULL;

GMenuItem * undo_item = NULL;
GMenuItem * pref_item = NULL;
GMenuItem * full_item = NULL;
GMenuItem * help_item = NULL;

GAction * undo_action = NULL;
GAction * pref_action = NULL;
GAction * full_action = NULL;
GAction * help_action = NULL;

void
undo_activated (void)
{
	g_debug("Undo Activated");
	g_simple_action_set_enabled(G_SIMPLE_ACTION(full_action), TRUE);
	return;
}

void
prefs_activated (void)
{
	g_debug("Preferences Activated");
	g_menu_append_item(menu, help_item);
	return;
}

int
main (int argv, char ** argc)
{
	if (argv != 3) {
		g_print("'%s <DBus name> <Object Path>' is how you should use this program.\n", argc[0]);
		return 1;
	}

#ifndef GLIB_VERSION_2_36
	g_type_init ();
#endif
	g_debug("Dynamic Toolbar Starting");

	menu = g_menu_new();
	ag = g_simple_action_group_new();

	/* UNDO, enables full item */
	undo_item = g_menu_item_new("Undo Item", "undo");
	g_menu_item_set_attribute_value(undo_item, "hud-toolbar-item", g_variant_new_string("undo"));
	g_menu_append_item(menu, undo_item);

	undo_action = G_ACTION(g_simple_action_new("undo", NULL));
	g_signal_connect(G_OBJECT(undo_action), "activate", G_CALLBACK(undo_activated), NULL);
	g_action_map_add_action(G_ACTION_MAP(ag), undo_action);

	/* Peferences, adds the help item */
	pref_item = g_menu_item_new("Preferences Item", "preferences");
	g_menu_item_set_attribute_value(pref_item, "hud-toolbar-item", g_variant_new_string("preferences"));
	g_menu_append_item(menu, pref_item);

	pref_action = G_ACTION(g_simple_action_new("preferences", NULL));
	g_signal_connect(G_OBJECT(pref_action), "activate", G_CALLBACK(prefs_activated), NULL);
	g_action_map_add_action(G_ACTION_MAP(ag), pref_action);

	/* Fullscreen, starts disabled */
	full_item = g_menu_item_new("Fullscreen Item", "fullscreen");
	g_menu_item_set_attribute_value(full_item, "hud-toolbar-item", g_variant_new_string("fullscreen"));
	g_menu_append_item(menu, full_item);

	full_action = G_ACTION(g_simple_action_new("fullscreen", NULL));
	g_simple_action_set_enabled(G_SIMPLE_ACTION(full_action), FALSE);
	g_action_map_add_action(G_ACTION_MAP(ag), full_action);

	/* Help, not added at first */
	help_item = g_menu_item_new("Help Item", "help");
	g_menu_item_set_attribute_value(help_item, "hud-toolbar-item", g_variant_new_string("help"));
	/* NO!  Can't you read!  g_menu_append_item(menu, help_item); */

	help_action = G_ACTION(g_simple_action_new("help", NULL));
	g_action_map_add_action(G_ACTION_MAP(ag), help_action);


	/* All the rest of the boring stuff */
	GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	g_dbus_connection_export_action_group(session, argc[2], G_ACTION_GROUP(ag), NULL);
	g_dbus_connection_export_menu_model(session, argc[2], G_MENU_MODEL(menu), NULL);

	g_bus_own_name(G_BUS_TYPE_SESSION, argc[1], 0, NULL, NULL, NULL, NULL, NULL);

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;
}
