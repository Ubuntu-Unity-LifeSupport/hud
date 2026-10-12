/*
 Copyright 2026 Ubuntu Unity Life Support

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

/* A menu whose items carry targets (G_MENU_ATTRIBUTE_TARGET), as GTK
 * applications export them: an action with a (ss) parameter (gnome-terminal's
 * New Window), one with a (uu) parameter (its window sizes), a stateful
 * string radio item, a stateful boolean toggle without a target, and a
 * parameterless action whose item has a target (GTK leaves that item
 * insensitive). Every activation emits ActionInvoked with the action name and
 * the parameter printed with its type, or "NULL". */

#include <glib-object.h>
#include <gio/gio.h>
#include <test-iface.h>

static void action_callback(GSimpleAction *simple, GVariant *parameter,
		gpointer user_data) {
	gchar *name = NULL;
	g_object_get(simple, "name", &name, NULL);
	gchar *printed = parameter != NULL ?
			g_variant_print(parameter, TRUE) : g_strdup("NULL");
	gchar *text = g_strdup_printf("%s %s", name, printed);
	hud_test_com_canonical_hud_test_emit_action_invoked(
			HUD_TEST_COM_CANONICAL_HUD_TEST(user_data), text);
	g_free(text);
	g_free(printed);
	g_free(name);
}

static GSimpleAction *add_action(GSimpleActionGroup *ag, GSimpleAction *action,
		HudTestComCanonicalHudTest *test_interface) {
	g_action_map_add_action(G_ACTION_MAP(ag), G_ACTION(action));
	g_signal_connect(action, "activate", G_CALLBACK(action_callback),
			test_interface);
	return action;
}

static void append(GMenu *menu, const gchar *label, const gchar *action,
		GVariant *target) {
	GMenuItem *mi = g_menu_item_new(label, NULL);
	g_menu_item_set_action_and_target_value(mi, action, target);
	g_menu_append_item(menu, mi);
	g_object_unref(mi);
}

int main(int argv, char ** argc) {
	if (argv != 3) {
		g_print(
				"'%s <DBus name> <Object Path>' is how you should use this program.\n",
				argc[0]);
		return 1;
	}

	GMenu *menu = g_menu_new();
	append(menu, "Window Open", "unity.new-terminal",
			g_variant_new("(ss)", "window", "current"));
	append(menu, "Small Size", "unity.size-to", g_variant_new("(uu)", 80, 24));
	append(menu, "List View", "unity.view", g_variant_new_string("list"));
	append(menu, "Bold Toggle", "unity.bold", NULL);
	append(menu, "Plain Target", "unity.plain", g_variant_new_string("x"));

	GSimpleActionGroup *ag = g_simple_action_group_new();
	HudTestComCanonicalHudTest *test_interface =
			hud_test_com_canonical_hud_test_skeleton_new();
	GError *error = NULL;
	GDBusConnection *session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if (!g_dbus_interface_skeleton_export(
			G_DBUS_INTERFACE_SKELETON (test_interface), session, argc[2],
			&error)) {
		g_error("Failed to export test interface");
	}

	add_action(ag, g_simple_action_new("new-terminal", G_VARIANT_TYPE("(ss)")),
			test_interface);
	add_action(ag, g_simple_action_new("size-to", G_VARIANT_TYPE("(uu)")),
			test_interface);
	add_action(ag, g_simple_action_new_stateful("view", G_VARIANT_TYPE_STRING,
			g_variant_new_string("grid")), test_interface);
	add_action(ag, g_simple_action_new_stateful("bold", NULL,
			g_variant_new_boolean(FALSE)), test_interface);
	add_action(ag, g_simple_action_new("plain", NULL), test_interface);

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
