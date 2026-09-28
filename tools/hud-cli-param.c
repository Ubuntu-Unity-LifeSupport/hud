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

#include <glib.h>
#include <gio/gunixinputstream.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <hud-client.h>


static void print_suggestions(const char * query);
static void models_ready (HudClientQuery * client_query, gpointer user_data);
static HudClientQuery * client_query = NULL;

int
main (int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init ();
#endif

	const gchar * search = "";

	if (argc == 2) {
		search = argv[1];
	}

	printf("\nsearch token: %s\n", search);
	print_suggestions(search);

	g_clear_object(&client_query);

	return 0;
}

/* Notification from Dee that we can stop waiting */
static void
wait_for_sync_notify (GObject * object, GParamSpec * pspec, gpointer user_data)
{
	GMainLoop * loop = (GMainLoop *)user_data;
	g_main_loop_quit(loop);
	return;
}

/* Waits for the DeeModel of results to get synchoronized from the
   HUD service */
static gboolean
wait_for_sync (DeeModel * model)
{
	if (dee_shared_model_is_synchronized(DEE_SHARED_MODEL(model))) {
		return TRUE;
	}

	GMainLoop * loop = g_main_loop_new(NULL, FALSE);

	glong sig = g_signal_connect(G_OBJECT(model), "notify::synchronized", G_CALLBACK(wait_for_sync_notify), loop);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	g_signal_handler_disconnect(G_OBJECT(model), sig);

	return dee_shared_model_is_synchronized(DEE_SHARED_MODEL(model));
}

/* Prints out the label for each item in the model.  Makes it easier
   to see what we've got */
static void
print_model (GMenuModel * model)
{
	int i;
	for (i = 0; i < g_menu_model_get_n_items(model); i++) {
		GVariant * vlabel = g_menu_model_get_item_attribute_value(model, i, G_MENU_ATTRIBUTE_LABEL, G_VARIANT_TYPE_STRING);

		if (vlabel == NULL) {
			g_print("\t\t(null)\n");
			continue;
		}

		g_print("\t\t%s\n", g_variant_get_string(vlabel, NULL));
		g_variant_unref(vlabel);
	}
	return;
}

/* Quit the mainloop if we get anything */
static void
population_update (GMenuModel * model, gint position, gint removed, gint added, GMainLoop * loop)
{
	g_main_loop_quit(loop);
	return;
}

/* Waiting here for the model to get populated with something
   before printing.  In a real world scenario you'd probably
   be responsive to the items being added.  We're a CLI tool. */
static void
populated_model (GMenuModel * model)
{
	if (g_menu_model_get_n_items(model) == 0) {
		GMainLoop * loop = g_main_loop_new(NULL, FALSE);

		gulong sig = g_signal_connect(model, "items-changed", G_CALLBACK(population_update), loop);

		g_main_loop_run(loop);
		g_main_loop_unref(loop);

		g_signal_handler_disconnect(model, sig);
	}

	return print_model(model);
}

/* Signal from the Client Param that it now has a model ready
   for us to look at.  This doesn't mean that the model has
   any data in it, but that the object is available. */
static void
model_ready (HudClientParam * param, GMainLoop * loop)
{
	GMenuModel * model = hud_client_param_get_model(param);
	if (model != NULL) {
		populated_model(model);
	} else {
		g_warning("Unable to get model even after it was 'ready'");
	}

	g_main_loop_quit(loop);
	return;
}

/* Prints out the entries from the search, but only the parameterized
   ones.  Then it looks at each parameterized one prints out the items
   that are available as parameters */
static void 
print_suggestions (const char *query)
{
	if (client_query == NULL) {
		client_query = hud_client_query_new(query);
		
		GMainLoop * loop = g_main_loop_new(NULL, FALSE);

		guint sig = g_signal_connect(G_OBJECT(client_query), HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED, G_CALLBACK(models_ready), loop);

		g_main_loop_run(loop);
		g_main_loop_unref(loop);

		g_signal_handler_disconnect(G_OBJECT(client_query), sig);
	} else {
		hud_client_query_set_query(client_query, query);
		models_ready(client_query, NULL);
	}

	return;
}

static void
models_ready (HudClientQuery * client_query, gpointer user_data)
{
	DeeModel * model = hud_client_query_get_results_model(client_query);
	g_return_if_fail(wait_for_sync(model));

	DeeModelIter * iter = NULL;
	int i = 0;
	for (iter = dee_model_get_first_iter(model); !dee_model_is_last(model, iter); iter = dee_model_next(model, iter), i++) {
		if (!hud_client_query_results_is_parameterized(client_query, iter)) {
			/* Only want parameterized */
			continue;
		}

		const gchar * suggestion = hud_client_query_results_get_command_name(client_query, iter);
		gchar * clean_line = NULL;
		pango_parse_markup(suggestion, -1, 0, NULL, &clean_line, NULL, NULL);
		printf("\t%s\n", clean_line);
		free(clean_line);

		HudClientParam * param = hud_client_query_execute_param_command(client_query, hud_client_query_results_get_command_id(client_query, iter), 0);
		g_return_if_fail(param != NULL);

		GMenuModel * model = hud_client_param_get_model(param);
		if (model == NULL) {
			GMainLoop * loop = g_main_loop_new(NULL, FALSE);

			gulong sig = g_signal_connect(param, "model-ready", G_CALLBACK(model_ready), loop);

			g_main_loop_run(loop);
			g_main_loop_unref(loop);

			g_signal_handler_disconnect(param, sig);
		} else {
			populated_model(model);
		}

		hud_client_param_send_cancel(param);
		g_object_unref(param);
	}

	if (user_data != NULL) {
		GMainLoop * loop = (GMainLoop *)user_data;
		g_main_loop_quit(loop);
	}

	return;
}

