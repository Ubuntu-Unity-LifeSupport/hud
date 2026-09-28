/*
Copyright 2013 Canonical Ltd.

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

static const gchar * menuitems[] = {
	"One",
	"Two",
	"Three",
	"Four",
	"Five",
	"Six",
	"Seven",
	"Eight",
	"Nine",
	"Ten",
	"Eleven",
	NULL
};

static GMenu *
items (const gchar * action, int entry)
{
	if (menuitems[entry + 1] == NULL) {
		GMenu * menu = g_menu_new();
		g_menu_append(menu, menuitems[entry], action);
		return menu;
	}

	GMenu * recursemenu = items(action, entry + 1);
	GMenu * menu = g_menu_new();
	g_menu_append_submenu(menu, menuitems[entry], G_MENU_MODEL(recursemenu));

	return menu;
}

int
main (int argv, char ** argc)
{
	if (!(argv == 3 || argv == 4)) {
		g_print("'%s <DBus name> <Object Path> [Is Application]' is how you should use this program.\n", argc[0]);
		return 1;
	}

#ifndef GLIB_VERSION_2_36
	g_type_init ();
#endif

	gboolean is_application = (argv == 4 && !g_strcmp0(argc[3], "TRUE"));

	GMenu * menu = NULL;
	if (is_application) {
	  menu = items("app.simple", 0);
	  g_menu_append(menu, "Base", "app.simple");
	} else {
	  menu = items("simple", 0);
	  g_menu_append(menu, "Base", "simple");
	}

	GSimpleActionGroup * ag = g_simple_action_group_new();
	if (is_application)
	  g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(g_simple_action_new("app.simple", G_VARIANT_TYPE_BOOLEAN)));
	else
	  g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(g_simple_action_new("simple", G_VARIANT_TYPE_BOOLEAN)));

	GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	g_dbus_connection_export_action_group(session, argc[2], G_ACTION_GROUP(ag), NULL);
	g_dbus_connection_export_menu_model(session, argc[2], G_MENU_MODEL(menu), NULL);

	g_bus_own_name(G_BUS_TYPE_SESSION, argc[1], 0, NULL, NULL, NULL, NULL, NULL);

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;
}
