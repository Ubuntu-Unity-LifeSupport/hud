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
#include <test-iface.h>

static void action_callback(GSimpleAction *simple,
		G_GNUC_UNUSED GVariant *parameter, G_GNUC_UNUSED gpointer user_data) {
	gchar *name = NULL;
	g_object_get(simple, "name", &name, NULL);
	hud_test_com_canonical_hud_test_emit_action_invoked(
			HUD_TEST_COM_CANONICAL_HUD_TEST(user_data), name);
	g_free(name);
}

int main(int argv, char ** argc) {
	if (argv != 3) {
		g_print(
				"'%s <DBus name> <Object Path>' is how you should use this program.\n",
				argc[0]);
		return 1;
	}

#ifndef GLIB_VERSION_2_36
	g_type_init ();
#endif

	GMenu * menu = g_menu_new();
	GMenuItem * mi = NULL;

	mi = g_menu_item_new("Save", "unity.save");
	g_menu_item_set_attribute_value(mi, "accel",
			g_variant_new_string("<Control>S"));
	g_menu_append_item(menu, mi);

	mi = g_menu_item_new("Quiter", "unity.quiter");
	g_menu_item_set_attribute_value(mi, "accel",
			g_variant_new_string("<Primary><Alt>Q"));
	g_menu_append_item(menu, mi);

	mi = g_menu_item_new("Close", "unity.close");
	g_menu_item_set_attribute_value(mi, "accel",
			g_variant_new_string("<Super>W"));
	g_menu_append_item(menu, mi);

	mi = g_menu_item_new("Nothing", "unity.nothing");
	g_menu_append_item(menu, mi);

	GSimpleAction *save = g_simple_action_new("save", NULL);
	GSimpleAction *quiter = g_simple_action_new("quiter",
	NULL);
	GSimpleAction *close = g_simple_action_new("close", NULL);
	GSimpleAction *nothing = g_simple_action_new("nothing",
	NULL);

	GSimpleActionGroup * ag = g_simple_action_group_new();
	g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(save));
	g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(quiter));
	g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(close));
	g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(nothing));

	HudTestComCanonicalHudTest *test_interface =
			hud_test_com_canonical_hud_test_skeleton_new();
	GError *error = NULL;
	if (!g_dbus_interface_skeleton_export(
			G_DBUS_INTERFACE_SKELETON (test_interface),
			g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL), argc[2], &error)) {
		g_error("Failed to export test interface");
	}

	g_signal_connect(save, "activate", G_CALLBACK(action_callback), test_interface);
	g_signal_connect(quiter, "activate", G_CALLBACK(action_callback), test_interface);
	g_signal_connect(close, "activate", G_CALLBACK(action_callback), test_interface);
	g_signal_connect(nothing, "activate", G_CALLBACK(action_callback), test_interface);

	GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	g_dbus_connection_export_action_group(session, argc[2], G_ACTION_GROUP(ag),
			NULL);
	g_dbus_connection_export_menu_model(session, argc[2], G_MENU_MODEL(menu),
			NULL);

	g_bus_own_name(G_BUS_TYPE_SESSION, argc[1], 0, NULL, NULL, NULL, NULL,
			NULL);

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;
}
