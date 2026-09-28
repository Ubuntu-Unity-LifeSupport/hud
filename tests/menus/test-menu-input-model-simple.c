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

    GMenu * menu = g_menu_new();
    GMenuItem * item = NULL;
    GMenuItem * ditem = NULL;
    if (is_application) {
      item = g_menu_item_new("Simple", "app.simple");
      ditem = g_menu_item_new("Disable", "app.disable");
    } else {
      item = g_menu_item_new("Simple", "simple");
      ditem = g_menu_item_new("Disable", "disable");
    }

    g_menu_item_set_attribute_value(item, "description", g_variant_new_string("A simple description"));
    g_menu_item_set_attribute_value(item, "keywords", g_variant_new_string("hard;difficult;;challenging;"));
    g_menu_append_item(menu, item);
    g_menu_append_item(menu, ditem);

    GSimpleActionGroup * ag = g_simple_action_group_new();
    g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(g_simple_action_new("simple", G_VARIANT_TYPE_BOOLEAN)));

    GSimpleAction * disable = g_simple_action_new("disable", NULL);
    g_simple_action_set_enabled(disable, FALSE);
    g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(disable));

    GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

    g_debug("Exporting Simple Action Group");
    g_dbus_connection_export_action_group(session, argc[2], G_ACTION_GROUP(ag), NULL);
    g_debug("Exporting Simple Menu");
    g_dbus_connection_export_menu_model(session, argc[2], G_MENU_MODEL(menu), NULL);

    g_debug("Investing in name ownership '%s'", argc[1]);
    g_bus_own_name(G_BUS_TYPE_SESSION, argc[1], 0, NULL, NULL, NULL, NULL, NULL);

    g_debug("Looping");
    GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(mainloop);

    return 0;
}
