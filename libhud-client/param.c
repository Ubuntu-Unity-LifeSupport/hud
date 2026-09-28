/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ted Gould <ted@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "param.h"
#include <libhud-client/action-muxer.h>
#include <gio/gio.h>

struct _HudClientParamPrivate {
	GDBusConnection * session;

	gchar * dbus_address;
	gchar * base_action;
	gchar * action_path;
	gchar * model_path;
	gint model_section;

	/* These are the ones we care about */
	GMenuModel * model;
	GActionGroup * base_actions;
	GActionGroup * actions;

	/* This is what we need to get those */
	GDBusMenuModel * base_model;
	gulong base_model_changes;

	gulong action_added;
	GList * queued_commands;
};

/* Signals */
enum {
	MODEL_READY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Prototypes */
static void hud_client_param_class_init (HudClientParamClass *klass);
static void hud_client_param_init       (HudClientParam *self);
static void hud_client_param_dispose    (GObject *object);
static void hud_client_param_finalize   (GObject *object);
static void action_write_state          (HudClientParam *  param,
                                         const gchar *     action);
static void base_model_items            (GMenuModel *      model,
                                         gint              position,
                                         gint              removed,
                                         gint              added,
                                         gpointer          user_data);

/* Boiler plate */
#define HUD_CLIENT_PARAM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_CLIENT_TYPE_PARAM, HudClientParamPrivate))

G_DEFINE_TYPE (HudClientParam, hud_client_param, G_TYPE_OBJECT)

/* Code */
static void
hud_client_param_class_init (HudClientParamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudClientParamPrivate));

	object_class->dispose = hud_client_param_dispose;
	object_class->finalize = hud_client_param_finalize;

	/**
	 * HudClientParam::model-ready:
	 * @arg0: The #HudClientParam object.
	 * 
	 * Emitted when the model can be used.  It may also be updating, but
	 * the base item is there.
	 */
	signals[MODEL_READY] =  g_signal_new(HUD_CLIENT_PARAM_SIGNAL_MODEL_READY,
	                                     G_TYPE_FROM_CLASS(klass),
	                                     G_SIGNAL_RUN_LAST,
	                                     G_STRUCT_OFFSET(HudClientParamClass, model_ready),
	                                     NULL, NULL,
	                                     g_cclosure_marshal_VOID__VOID,
	                                     G_TYPE_NONE, 0, G_TYPE_NONE);

	return;
}

static void
hud_client_param_init (HudClientParam *self)
{
	self->priv = HUD_CLIENT_PARAM_GET_PRIVATE(self);

	self->priv->session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	return;
}

static void
hud_client_param_dispose (GObject *object)
{
	HudClientParam * param = HUD_CLIENT_PARAM(object);

	action_write_state(param, "end");

	if (param->priv->base_model_changes != 0) {
		g_signal_handler_disconnect(param->priv->base_model, param->priv->base_model_changes);
		param->priv->base_model_changes = 0;
	}

	if (param->priv->action_added != 0) {
		g_signal_handler_disconnect(param->priv->actions, param->priv->action_added);
		param->priv->action_added = 0;
	}

	g_clear_object(&param->priv->base_model);
	g_clear_object(&param->priv->model);
	g_clear_object(&param->priv->base_actions);
	g_clear_object(&param->priv->actions);
	g_clear_object(&param->priv->session);

	G_OBJECT_CLASS (hud_client_param_parent_class)->dispose (object);
	return;
}

static void
hud_client_param_finalize (GObject *object)
{
	HudClientParam * param = HUD_CLIENT_PARAM(object);

	g_list_free_full(param->priv->queued_commands, g_free);
	param->priv->queued_commands = NULL;

	g_clear_pointer(&param->priv->dbus_address, g_free);
	g_clear_pointer(&param->priv->base_action, g_free);
	g_clear_pointer(&param->priv->action_path, g_free);
	g_clear_pointer(&param->priv->model_path, g_free);

	G_OBJECT_CLASS (hud_client_param_parent_class)->finalize (object);
	return;
}

/* Writes to the base action a particular state */
static void
action_write_state (HudClientParam * param, const gchar * action)
{
	/* This shows that they're not interested in these events,
	   which is fine, perhaps a bit lonely, but fine. */
	if (param->priv->base_action == NULL) {
		return;
	}

	if (!g_action_group_has_action(param->priv->actions, param->priv->base_action)) {
		if (param->priv->action_added != 0) {
			/* We're looking for the action, queue the command */
			param->priv->queued_commands = g_list_append(param->priv->queued_commands, g_strdup(action));
			return;
		} else {
			/* Uhm, oh, my!  We shouldn't be here */
			g_warning("We've got an action name, but it doesn't exist, and we've given up looking.  Why would we give up?");
			return;
		}
	}

	const GVariantType * type = g_action_group_get_action_parameter_type(param->priv->actions, param->priv->base_action);

	GVariant * action_param = NULL;

	if (g_variant_type_equal(type, G_VARIANT_TYPE_STRING)) {
		action_param = g_variant_new_string(action);
	} else if (g_variant_type_equal(type, G_VARIANT_TYPE("(ssav)"))) {
		GVariantBuilder tuple;

		g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
		g_variant_builder_add_value(&tuple, g_variant_new_string(action));
		g_variant_builder_add_value(&tuple, g_variant_new_string(""));
		g_variant_builder_add_value(&tuple, g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0));

		action_param = g_variant_builder_end(&tuple);
	}

	if (action_param == NULL) {
		gchar * typestring = g_variant_type_dup_string(type);
		g_warning("Unable to signal to a action with the parameter type of: %s", typestring);
		g_free(typestring);
		return;
	}

	g_action_group_activate_action(param->priv->actions, param->priv->base_action, action_param);

	return;
}

/* Look at the items changed and make sure we're getting the
   item that we expect.  Then signal. */
static void
base_model_items (G_GNUC_UNUSED GMenuModel * model, gint position, gint removed, G_GNUC_UNUSED gint added, gpointer user_data)
{
	g_return_if_fail(position == 0);
	g_return_if_fail(removed == 0);
	g_return_if_fail(HUD_CLIENT_IS_PARAM(user_data));

	HudClientParam * param = HUD_CLIENT_PARAM(user_data);

	int i = 0;
	if(param->priv->base_action != NULL) {
		for(; i < g_menu_model_get_n_items(G_MENU_MODEL(param->priv->base_model)); ++i) {
			gchar *action = NULL;
			g_menu_model_get_item_attribute(G_MENU_MODEL(param->priv->base_model), i, "action", "s", &action);
			if(g_str_has_suffix(action, param->priv->base_action)) {
				g_free(action);
				break;
			}
			g_free(action);
		}
	}

	param->priv->model = g_menu_model_get_item_link(G_MENU_MODEL(param->priv->base_model), i, G_MENU_LINK_SUBMENU);
	if(param->priv->model == NULL) {
		g_warning("linked sub-model could not be found");
		return;
	}

	g_signal_emit(param, signals[MODEL_READY], 0);

	if (param->priv->base_model_changes != 0) {
		g_signal_handler_disconnect(param->priv->base_model, param->priv->base_model_changes);
		param->priv->base_model_changes = 0;
	}

	return;
}

/* Look to see if our base item gets added to the actions
   list on the service side */
static void
action_added (G_GNUC_UNUSED GActionGroup * group, const gchar * action_name, gpointer user_data)
{
	g_return_if_fail(HUD_CLIENT_IS_PARAM(user_data));

	HudClientParam * param = HUD_CLIENT_PARAM(user_data);

	if (g_strcmp0(param->priv->base_action, action_name) != 0) {
		/* This is not the action we're looking for */
		return;
	}

	/* We don't need to know again */
	if (param->priv->action_added != 0) {
		g_signal_handler_disconnect(param->priv->actions, param->priv->action_added);
		param->priv->action_added = 0;
	}

	/* Now process the queue */
	GList * command;
	for (command = param->priv->queued_commands; command != NULL; command = g_list_next(command)) {
		action_write_state(param, (gchar *)command->data);
	}

	/* And clear it */
	g_list_free_full(param->priv->queued_commands, g_free);
	param->priv->queued_commands = NULL;

	return;
}

/**
 * hud_client_param_new:
 * @dbus_address: The address on dbus to find the actions
 * @prefix: The prefix the menu is using
 * @base_action: The action to send events for the dialog on
 * @action_path: DBus path to the action object
 * @model_path: DBus path to the menu model object
 * @model_section: Section of the model to use
 *
 * Create a new #HudClientParam object for adjusting a specified
 * paramaterized dialog.
 *
 * Return value: (transfer full): A new #HudClientParam dialog
 */
HudClientParam *
hud_client_param_new (const gchar * dbus_address, const gchar *prefix, const gchar * base_action, const gchar * action_path, const gchar * model_path, gint model_section)
{
	g_return_val_if_fail(dbus_address != NULL, NULL);
	/* NOTE: base_action is not required -- though probably a NULL string */
	g_return_val_if_fail(g_variant_is_object_path(action_path), NULL);
	g_return_val_if_fail(g_variant_is_object_path(model_path), NULL);

	HudClientParam * param = g_object_new(HUD_CLIENT_TYPE_PARAM, NULL);

	param->priv->dbus_address = g_strdup(dbus_address);
	param->priv->action_path = g_strdup(action_path);
	param->priv->model_path = g_strdup(model_path);
	param->priv->model_section = model_section;

	/* Keep the value NULL if we've got an empty string */
	if (base_action != NULL && base_action[0] != '\0') {
		if (prefix != NULL && prefix[0] != '\0') {
			param->priv->base_action = g_strdup_printf("%s.%s", prefix, base_action);
		} else {
			param->priv->base_action = g_strdup(base_action);
		}
	}

	g_warn_if_fail(model_section == 1);
	param->priv->base_model = g_dbus_menu_model_get(param->priv->session, param->priv->dbus_address, param->priv->model_path);

	if (g_menu_model_get_n_items(G_MENU_MODEL(param->priv->base_model)) == 0) {
		param->priv->base_model_changes = g_signal_connect(G_OBJECT(param->priv->base_model), "items-changed", G_CALLBACK(base_model_items), param);
	} else {
		base_model_items(G_MENU_MODEL(param->priv->base_model), 0, 0, 1, param);
	}

	GDBusActionGroup * dbus_ag = g_dbus_action_group_get(param->priv->session, param->priv->dbus_address, param->priv->action_path);
	param->priv->base_actions = G_ACTION_GROUP(dbus_ag);
	GActionMuxer *muxer = g_action_muxer_new();
	g_action_muxer_insert(muxer, "hud", param->priv->base_actions);
	param->priv->actions = G_ACTION_GROUP(muxer);
	
	/* Looking for a base action here */
	if (param->priv->base_action != NULL && !g_action_group_has_action(param->priv->actions, param->priv->base_action)) {
		param->priv->action_added = g_signal_connect(G_OBJECT(param->priv->actions), "action-added", G_CALLBACK(action_added), param);
	} else {
		action_write_state(param, "start");
	}

	return param;
}

/**
 * hud_client_param_get_actions:
 * @param: The #HudClientParam to query
 *
 * The object path to the actions
 *
 * Return value: (transfer none): A #GActionGroup that has the actions in it
 */
GActionGroup *
hud_client_param_get_actions (HudClientParam * param)
{
	g_return_val_if_fail(HUD_CLIENT_IS_PARAM(param), NULL);

	return param->priv->actions;
}

/**
 * hud_client_param_get_model:
 * @param: The #HudClientParam to query
 *
 * The object path to the model
 *
 * Return value: (transfer none): The menu model of the pane
 */
GMenuModel *
hud_client_param_get_model (HudClientParam * param)
{
	g_return_val_if_fail(HUD_CLIENT_IS_PARAM(param), NULL);

	return param->priv->model;
}

/**
 * hud_client_param_send_reset:
 * @param: The #HudClientParam to query
 *
 * Send the command to the application to reset the values
 * of the actions in the pane.
 */
void
hud_client_param_send_reset (HudClientParam * param)
{
	g_return_if_fail(HUD_CLIENT_IS_PARAM(param));

	action_write_state(param, "reset");
	return;
}

/**
 * hud_client_param_send_cancel:
 * @param: The #HudClientParam to query
 *
 * Send the command to the application to cancel the values
 * of the actions in the panel and expect it to close soon.
 */
void
hud_client_param_send_cancel (HudClientParam * param)
{
	g_return_if_fail(HUD_CLIENT_IS_PARAM(param));

	action_write_state(param, "cancel");
	return;
}

/**
 * hud_client_param_send_commit:
 * @param: The #HudClientParam to query
 *
 * Tell the application that the user has requested the values
 * be applied.  This doesn't mean that there isn't a dialog
 * still open, when it closes "end" will be sent.
 */
void
hud_client_param_send_commit (HudClientParam * param)
{
	g_return_if_fail(HUD_CLIENT_IS_PARAM(param));

	action_write_state(param, "commit");
	return;
}
