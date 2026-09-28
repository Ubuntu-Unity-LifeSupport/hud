/*
Small utility to excersise the HUD from the command line

Copyright 2011 Canonical Ltd.

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

#include <hud-client.h>


int
main (int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init ();
#endif

	const gchar * action = "help";

	if (argc == 2) {
		action = argv[1];
	}

	g_print("\ntoolbar action: %s\n", action);

	HudClientQuery * client_query = NULL;
	client_query = hud_client_query_new("");

	hud_client_query_execute_toolbar_item(client_query, hud_client_query_toolbar_items_get_value_from_nick(action), 0);

	g_clear_object(&client_query);

	return 0;
}

