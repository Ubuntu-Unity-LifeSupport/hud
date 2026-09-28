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

#define _GNU_SOURCE
#include <glib.h>
#include <gio/gunixinputstream.h>
#include <gtk/gtk.h>

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

static void
wait_for_sync_notify (GObject * object, GParamSpec * pspec, gpointer user_data)
{
	GMainLoop * loop = (GMainLoop *)user_data;
	g_main_loop_quit(loop);
	return;
}

static gboolean
wait_for_sync (DeeModel * model)
{
	g_return_val_if_fail(DEE_IS_SHARED_MODEL(model), FALSE);

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
		const gchar * suggestion = dee_model_get_string(model, iter, 1);

		gchar * clean_line = NULL;
		pango_parse_markup(suggestion, -1, 0, NULL, &clean_line, NULL, NULL);
		printf("\t%s\n", clean_line);
		g_free(clean_line);
	}

	if (user_data != NULL) {
		GMainLoop * loop = (GMainLoop *)user_data;
		g_main_loop_quit(loop);
	}

	return;
}
