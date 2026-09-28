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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "action-publisher.h"

#include "manager.h"
#include "marshal.h"

#include <string.h>

typedef GMenuModelClass HudAuxClass;

typedef struct
{
  GMenuModel parent_instance;

  HudActionPublisher *publisher;
} HudAux;

static void hud_aux_init_action_group_iface (GActionGroupInterface *iface);
static void hud_aux_init_remote_action_group_iface (GRemoteActionGroupInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudAux, _hud_aux, G_TYPE_MENU_MODEL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_GROUP, hud_aux_init_action_group_iface)
                         G_IMPLEMENT_INTERFACE (G_TYPE_REMOTE_ACTION_GROUP, hud_aux_init_remote_action_group_iface))

typedef GObjectClass HudActionPublisherClass;

struct _HudActionPublisher
{
  GObject parent_instance;

  guint window_id;
  gchar * context_id;

  GDBusConnection *bus;
  GApplication *application;
  gint export_id;
  gchar *path;

  GSequence *descriptions;
  HudAux *aux;

  GList * action_groups;
};

enum
{
  SIGNAL_ACTION_GROUP_ADDED,
  SIGNAL_ACTION_GROUP_REMOVED,
  N_SIGNALS
};

guint hud_action_publisher_signals[N_SIGNALS];

G_DEFINE_TYPE (HudActionPublisher, hud_action_publisher, G_TYPE_OBJECT)

typedef GObjectClass HudActionDescriptionClass;

struct _HudActionDescription
{
  GObject parent_instance;

  gchar *identifier;
  gchar *action;
  GVariant *target;
  GHashTable *attrs;
  GHashTable *links;
};

guint hud_action_description_changed_signal;

G_DEFINE_TYPE (HudActionDescription, hud_action_description, G_TYPE_OBJECT)


static gboolean
hud_aux_is_mutable (G_GNUC_UNUSED GMenuModel *model)
{
  return TRUE;
}

static gint
hud_aux_get_n_items (GMenuModel *model)
{
  HudAux *aux = (HudAux *) model;

  return g_sequence_get_length (aux->publisher->descriptions);
}

static void
hud_aux_get_item_attributes (GMenuModel  *model,
                             gint         item_index,
                             GHashTable **attributes)
{
  HudAux *aux = (HudAux *) model;
  GSequenceIter *iter;
  HudActionDescription *description;

  iter = g_sequence_get_iter_at_pos (aux->publisher->descriptions, item_index);
  description = g_sequence_get (iter);

  *attributes = g_hash_table_ref (description->attrs);
}

static void
hud_aux_get_item_links (GMenuModel  *model,
                        gint         item_index,
                        GHashTable **links)
{
  HudAux *aux = (HudAux *) model;
  GSequenceIter *iter;
  HudActionDescription *description;

  iter = g_sequence_get_iter_at_pos (aux->publisher->descriptions, item_index);
  description = g_sequence_get (iter);

  if (description->links != NULL)
    *links = g_hash_table_ref(description->links);
  else
    *links = g_hash_table_new (NULL, NULL);
}

static void
_hud_aux_init (G_GNUC_UNUSED HudAux *aux)
{
}

static void
hud_aux_init_action_group_iface (G_GNUC_UNUSED GActionGroupInterface *iface)
{
}

static void
hud_aux_init_remote_action_group_iface (G_GNUC_UNUSED GRemoteActionGroupInterface *iface)
{
}

static void
hud_action_publisher_action_group_set_destroy (gpointer data)
{
  HudActionPublisherActionGroupSet *group = data;
  g_free(group->prefix);
  g_free(group->path);
  g_free(group);
}

static void
_hud_aux_class_init (HudAuxClass *class)
{
  class->is_mutable = hud_aux_is_mutable;
  class->get_n_items = hud_aux_get_n_items;
  class->get_item_attributes = hud_aux_get_item_attributes;
  class->get_item_links = hud_aux_get_item_links;
}

static void
hud_action_publisher_dispose (GObject *object)
{
  HudActionPublisher *self = HUD_ACTION_PUBLISHER(object);

  g_clear_object(&self->aux);
  g_clear_object(&self->application);

  if (self->export_id != 0) {
    g_debug("Un-exporting menu model at with id [%d]", self->export_id);
    g_dbus_connection_unexport_menu_model (self->bus, self->export_id);
  }

  g_clear_pointer(&self->path, g_free);
  g_clear_pointer(&self->descriptions, g_sequence_free);

  g_list_free_full(self->action_groups, (GDestroyNotify)hud_action_publisher_action_group_set_destroy);

  g_clear_object(&self->bus);

  G_OBJECT_CLASS (hud_action_publisher_parent_class)->dispose (object);
}

static void
hud_action_publisher_finalize (GObject *object)
{
  g_clear_pointer (&HUD_ACTION_PUBLISHER(object)->context_id, g_free);

  G_OBJECT_CLASS (hud_action_publisher_parent_class)->finalize (object);
  return;
}

static void
hud_action_publisher_init (HudActionPublisher *publisher)
{
  static guint64 next_id;

  publisher->descriptions = g_sequence_new (g_object_unref);
  publisher->aux = g_object_new (_hud_aux_get_type (), NULL);
  publisher->aux->publisher = publisher;
  publisher->bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  if (!publisher->bus)
    return;

  do
    {
      guint64 id = next_id++;
      GError *error = NULL;

      if (id == 0)
        publisher->path = g_strdup_printf ("/com/canonical/hud/publisher");
      else
        publisher->path = g_strdup_printf ("/com/canonical/hud/publisher%" G_GUINT64_FORMAT, id);

      publisher->export_id = g_dbus_connection_export_menu_model (publisher->bus, publisher->path,
                                                                  G_MENU_MODEL (publisher->aux), &error);
      g_debug("Exporting menu model at [%s] with id [%d]", publisher->path, publisher->export_id);

      if (!publisher->export_id) {
        /* try again... */
        g_debug("Exporting failed: %s", error->message);
        g_error_free(error);
        error = NULL;
        g_free (publisher->path);
        publisher->path = NULL;
      }
    }
  while (publisher->path == NULL);
}

static void
hud_action_publisher_class_init (HudActionPublisherClass *class)
{
  class->dispose = hud_action_publisher_dispose;
  class->finalize = hud_action_publisher_finalize;

  /**
   * HudActionPublisher::action-group-added:
   * @param1: Prefix for the action group
   * @param2: Path group is exported on DBus
   *
   * Emitted when a new action group is added to the publisher
   */
  hud_action_publisher_signals[SIGNAL_ACTION_GROUP_ADDED] = g_signal_new (HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_ADDED, HUD_TYPE_ACTION_PUBLISHER,
                                                                  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                                  _hud_marshal_VOID__STRING_STRING,
                                                                  G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
  /**
   * HudActionPublisher::action-group-removed:
   * @param1: Prefix for the action group
   * @param2: Path group is exported on DBus
   *
   * Emitted when a new action group is removed from the publisher
   */
  hud_action_publisher_signals[SIGNAL_ACTION_GROUP_REMOVED] = g_signal_new (HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_REMOVED, HUD_TYPE_ACTION_PUBLISHER,
                                                                  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                                  _hud_marshal_VOID__STRING_STRING,
                                                                  G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}

/**
 * hud_action_publisher_new_for_application:
 * @application: A #GApplication object
 *
 * Creates a new #HudActionPublisher and automatically registers the
 * default actions under the "app" prefix.
 *
 * Return value: (transfer full): A new #HudActionPublisher object
 */
HudActionPublisher *
hud_action_publisher_new_for_application (GApplication *application)
{
  HudActionPublisher *publisher;

  g_return_val_if_fail (G_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (g_application_get_application_id (application), NULL);
  g_return_val_if_fail (g_application_get_is_registered (application), NULL);
  g_return_val_if_fail (!g_application_get_is_remote (application), NULL);

  publisher = g_object_new (HUD_TYPE_ACTION_PUBLISHER, NULL);
  publisher->application = g_object_ref (application);

  hud_action_publisher_add_action_group (publisher, "app",
                           g_application_get_dbus_object_path (application));

  return publisher;
}

static guint context_count = 0;

/**
 * hud_action_publisher_new:
 * @window_id: (allow-none): A window ID
 * @context_id: (allow-none): A context ID
 *
 * Creates a new #HudActionPublisher based on the window ID passed
 * in via @window_id and context ID from @context_id.
 *
 * Either one of them can be not used by passing in eithe of the
 * defines #HUD_ACTION_PUBLISHER_ALL_WINDOWS or #HUD_ACTION_PUBLISHER_NO_CONTEXT
 * for the appropriate parameter.
 *
 * Return value: (transfer full): A new #HudActionPublisher object
 */
HudActionPublisher *
hud_action_publisher_new (guint window_id, const gchar * context_id)
{
	HudActionPublisher * publisher;
	publisher = g_object_new (HUD_TYPE_ACTION_PUBLISHER, NULL);
	publisher->window_id = window_id;

	if (context_id != NULL) {
		publisher->context_id = g_strdup(context_id);
	} else {
		publisher->context_id = g_strdup_printf("action-publisher-context-%d", context_count++);
	}

	return publisher;
}

static gchar *
format_identifier (const gchar *action_name,
                   GVariant    *action_target)
{
  gchar *targetstr;
  gchar *identifier;

  if (action_target)
    {
      targetstr = g_variant_print (action_target, TRUE);
      identifier = g_strdup_printf ("%s(%s)", action_name, targetstr);
      g_free (targetstr);
    }

  else
    identifier = g_strdup_printf ("%s()", action_name);

  return identifier;
}

static gint
compare_descriptions (gconstpointer a,
                      gconstpointer b,
                      G_GNUC_UNUSED gpointer      user_data)
{
  const HudActionDescription *da = a;
  const HudActionDescription *db = b;

  return strcmp (da->identifier, db->identifier);
}

static void
description_changed (HudActionDescription *description,
                     G_GNUC_UNUSED const gchar *attribute_name,
                     gpointer              user_data)
{
  HudActionPublisher *publisher = user_data;
  GSequenceIter *iter;

  iter = g_sequence_lookup (publisher->descriptions, description, compare_descriptions, NULL);
  g_assert (g_sequence_get (iter) == description);

  g_menu_model_items_changed (G_MENU_MODEL (publisher->aux), g_sequence_iter_get_position (iter), 1, 1);
}

static void
disconnect_handler (gpointer data,
                    gpointer user_data)
{
  HudActionPublisher *publisher = user_data;
  HudActionDescription *description = data;

#pragma GCC diagnostic ignored "-Wpedantic"
  g_signal_handlers_disconnect_by_func (description, description_changed, publisher);
#pragma GCC diagnostic pop
}

/**
 * hud_action_publisher_add_description:
 * @publisher: the #HudActionPublisher
 * @description: an action description
 *
 * Adds @description to the list of actions that the application exports
 * to the HUD.
 *
 * If the application is already exporting an action with the same name
 * and target value as @description then it will be replaced.
 *
 * You should only use this API for situations like recent documents and
 * bookmarks.
 */
void
hud_action_publisher_add_description (HudActionPublisher   *publisher,
                                      HudActionDescription *description)
{
  GSequenceIter *iter;

  iter = g_sequence_lookup (publisher->descriptions, description, compare_descriptions, NULL);

  if (iter == NULL)
    {
      /* We are not replacing -- add new. */
      iter = g_sequence_insert_sorted (publisher->descriptions, description, compare_descriptions, NULL);

      /* Signal that we added the items */
      g_menu_model_items_changed (G_MENU_MODEL (publisher->aux), g_sequence_iter_get_position (iter), 0, 1);
    }
  else
    {
      /* We are replacing an existing item. */
      disconnect_handler (g_sequence_get (iter), publisher);
      g_sequence_set (iter, description);

      /* A replace is 1 remove and 1 add */
      g_menu_model_items_changed (G_MENU_MODEL (publisher->aux), g_sequence_iter_get_position (iter), 1, 1);
    }

  g_object_ref (description);

  g_signal_connect (description, "changed", G_CALLBACK (description_changed), publisher);
}

/**
 * hud_action_publisher_remove_description:
 * @publisher: the #HudActionPublisher
 * @action_name: an action name
 * @action_target: (allow-none): an action target
 *
 * Removes the action descriptions that has the name @action_name and
 * the target value @action_target (including the possibility of %NULL).
 **/
void
hud_action_publisher_remove_description (HudActionPublisher *publisher,
                                         const gchar        *action_name,
                                         GVariant           *action_target)
{
  HudActionDescription tmp;
  GSequenceIter *iter;

  tmp.identifier = format_identifier (action_name, action_target);
  iter = g_sequence_lookup (publisher->descriptions, &tmp, compare_descriptions, NULL);
  g_free (tmp.identifier);

  if (iter)
    {
      gint position;

      position = g_sequence_iter_get_position (iter);
      disconnect_handler (g_sequence_get (iter), publisher);
      g_sequence_remove (iter);

      g_menu_model_items_changed (G_MENU_MODEL (publisher->aux), position, 1, 0);
    }
}

/**
 * hud_action_publisher_remove_descriptions:
 * @publisher: the #HudActionPublisher
 * @action_name: an action name
 *
 * Removes all action descriptions that has the name @action_name and
 * any target value.
 **/
void
hud_action_publisher_remove_descriptions (HudActionPublisher *publisher,
                                          const gchar        *action_name)
{
  HudActionDescription before, after;
  GSequenceIter *start, *end;

  before.identifier = (gchar *) action_name;
  after.identifier = g_strconcat (action_name, "~", NULL);
  start = g_sequence_search (publisher->descriptions, &before, compare_descriptions, NULL);
  end = g_sequence_search (publisher->descriptions, &after, compare_descriptions, NULL);
  g_free (after.identifier);

  if (start != end)
    {
      gint s, e;

      s = g_sequence_iter_get_position (start);
      e = g_sequence_iter_get_position (end);
      g_sequence_foreach_range (start, end, disconnect_handler, publisher);
      g_sequence_remove_range (start, end);

      g_menu_model_items_changed (G_MENU_MODEL (publisher->aux), s, e - s, 0);
    }
}

/**
 * hud_action_publisher_add_action_group:
 * @publisher: a #HudActionPublisher
 * @prefix: the action prefix for the group (like "app")
 * @object_path: the object path of the exported group
 *
 * Informs the HUD of the existance of an action group.
 *
 * The action group must be published on the shared session bus
 * connection of this process at @object_path.
 *
 * The @prefix defines the type of action group.  Currently "app" and
 * "win" are supported.  For example, if the exported action group
 * contained a "quit" action and you wanted to refer to it as "app.quit"
 * from action descriptions, then you would use the @prefix "app" here.
 *
 * @identifier is a piece of identifying information, depending on which
 * @prefix is used.  Currently, this should be %NULL for the "app"
 * @prefix and should be a uint32 specifying the window ID (eg: XID) for
 * the "win" @prefix.
 *
 * You do not need to manually export your action groups if you are
 * using #GApplication.
 **/
void
hud_action_publisher_add_action_group (HudActionPublisher *publisher,
                                       const gchar        *prefix,
                                       const gchar        *object_path)
{
	g_return_if_fail(HUD_IS_ACTION_PUBLISHER(publisher));
	g_return_if_fail(prefix != NULL);
	g_return_if_fail(object_path != NULL);

	HudActionPublisherActionGroupSet * group = g_new0(HudActionPublisherActionGroupSet, 1);

	group->prefix = g_strdup(prefix);
	group->path = g_strdup(object_path);

	publisher->action_groups = g_list_prepend(publisher->action_groups, group);

	return;
}

/**
 * hud_action_publisher_remove_action_group:
 * @publisher: a #HudActionPublisher
 * @prefix: the action prefix for the group (like "app")
 * @identifier: (allow-none): an identifier, or %NULL
 *
 * Informs the HUD that an action group no longer exists.
 *
 * This reverses the effect of a previous call to
 * hud_action_publisher_add_action_group() with the same parameters.
 */
void
hud_action_publisher_remove_action_group (G_GNUC_UNUSED HudActionPublisher *publisher,
                                          G_GNUC_UNUSED const gchar        *prefix,
                                          G_GNUC_UNUSED GVariant           *identifier)
{
  //hud_manager_remove_actions (publisher->application_id, prefix, identifier);
}

/**
 * hud_action_publisher_get_window_id:
 * @publisher: A #HudActionPublisher object
 *
 * Gets the window ID for this publisher
 *
 * Return value: The Window ID associtaed with this action publisher
 */
guint
hud_action_publisher_get_window_id (HudActionPublisher    *publisher)
{
	g_return_val_if_fail(HUD_IS_ACTION_PUBLISHER(publisher), 0);
	return publisher->window_id;
}

/**
 * hud_action_publisher_get_context_id:
 * @publisher: A #HudActionPublisher object
 *
 * Gets the context ID for this publisher
 *
 * Return value: The context ID associtaed with this action publisher
 */
const gchar *
hud_action_publisher_get_context_id (HudActionPublisher    *publisher)
{
	g_return_val_if_fail(HUD_IS_ACTION_PUBLISHER(publisher), 0);
	return publisher->context_id;
}


/**
 * hud_action_publisher_get_action_groups:
 * @publisher: A #HudActionPublisher object
 *
 * Grabs the action groups for this publisher
 *
 * Return value: (transfer container) (element-type HudActionPublisherActionGroupSet *): The groups in this publisher
 */
GList *
hud_action_publisher_get_action_groups (HudActionPublisher    *publisher)
{
	g_return_val_if_fail(HUD_IS_ACTION_PUBLISHER(publisher), NULL);
	/* TODO: Flesh out */

	return publisher->action_groups;
}

/**
 * hud_action_publisher_get_description_path:
 * @publisher: A #HudActionPublisher object
 *
 * Grabs the object path of the description for this publisher
 *
 * Return value: The object path of the descriptions
 */
const gchar *
hud_action_publisher_get_description_path (HudActionPublisher    *publisher)
{
	g_return_val_if_fail(HUD_IS_ACTION_PUBLISHER(publisher), NULL);
	return publisher->path;
}

/**
 * HudActionDescription:
 *
 * A description of an action that is accessible from the HUD.  This is
 * an opaque structure type and all accesses must be made via the API.
 **/

static void
hud_action_description_finalize (GObject *object)
{
  HudActionDescription *description = HUD_ACTION_DESCRIPTION (object);

  g_free (description->identifier);
  g_free (description->action);
  if (description->target)
    g_variant_unref (description->target);
  g_hash_table_unref (description->attrs);
  g_clear_pointer(&description->links, g_hash_table_unref);

  G_OBJECT_CLASS (hud_action_description_parent_class)
    ->finalize (object);
}

static void
hud_action_description_init (HudActionDescription *description)
{
  description->attrs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
}

static void
hud_action_description_class_init (HudActionDescriptionClass *class)
{
  class->finalize = hud_action_description_finalize;

  /**
   * HudActionDescription::changed:
   * @param1: Name of changed property
   *
   * Emitted when a property of the action description gets
   * changed.
   */
  hud_action_description_changed_signal = g_signal_new ("changed", HUD_TYPE_ACTION_DESCRIPTION,
                                                        G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL,
                                                        g_cclosure_marshal_VOID__STRING,
                                                        G_TYPE_NONE, 1, G_TYPE_STRING);
}

/**
 * hud_action_description_new:
 * @action_name: a (namespaced) action name
 * @action_target: an action target
 *
 * Creates a new #HudActionDescription.
 *
 * The situations in which you want to do this are limited to "dynamic"
 * types of actions -- things like bookmarks or recent documents.
 *
 * Use hud_action_publisher_add_descriptions_from_file() to take care of
 * the bulk of the actions in your application.
 *
 * Returns: a new #HudActionDescription with no attributes
 **/
HudActionDescription *
hud_action_description_new (const gchar *action_name,
                            GVariant    *action_target)
{
  HudActionDescription *description;

  g_return_val_if_fail (action_name != NULL, NULL);

  description = g_object_new (HUD_TYPE_ACTION_DESCRIPTION, NULL);
  description->action = g_strdup (action_name);
  description->target = action_target ? g_variant_ref_sink (action_target) : NULL;
  description->identifier = format_identifier (action_name, action_target);

  g_hash_table_insert (description->attrs, g_strdup ("action"),
                       g_variant_ref_sink (g_variant_new_string (action_name)));

  if (action_target)
    g_hash_table_insert (description->attrs, g_strdup ("target"), g_variant_ref_sink (action_target));

  return description;
}

/**
 * hud_action_description_set_attribute_value:
 * @description: a #HudActionDescription
 * @attribute_name: an attribute name
 * @value: (allow-none): the new value for the attribute
 *
 * Sets or unsets an attribute on @description.
 *
 * You may not change the "action" or "target" attributes.
 *
 * If @value is non-%NULL then it is the new value for attribute.  A
 * %NULL @value unsets @attribute_name.
 *
 * @value is consumed if it is floating.
 **/
void
hud_action_description_set_attribute_value (HudActionDescription *description,
                                            const gchar          *attribute_name,
                                            GVariant             *value)
{
  /* Don't allow setting the action or target as these form the
   * identity of the description and are stored separately...
   */
  g_return_if_fail (!g_str_equal (attribute_name, "action"));
  g_return_if_fail (!g_str_equal (attribute_name, "target"));

  if (value)
    g_hash_table_insert (description->attrs, g_strdup (attribute_name), g_variant_ref_sink (value));
  else
    g_hash_table_remove (description->attrs, attribute_name);

  g_signal_emit (description, hud_action_description_changed_signal,
                 g_quark_try_string (attribute_name), attribute_name);
}

/**
 * hud_action_description_set_attribute:
 * @description: a #HudActionDescription
 * @attribute_name: an attribute name
 * @format_string: (allow-none): a #GVariant format string
 * @...: arguments to @format_string
 *
 * Sets or unsets an attribute on @description.
 *
 * You may not change the "action" or "target" attributes.
 *
 * If @format_string is non-%NULL then this call is equivalent to
 * g_variant_new() and hud_action_description_set_attribute_value().
 *
 * If @format_string is %NULL then this call is equivalent to
 * hud_action_description_set_attribute_value() with a %NULL value.
 **/
void
hud_action_description_set_attribute (HudActionDescription *description,
                                      const gchar          *attribute_name,
                                      const gchar          *format_string,
                                      ...)
{
  GVariant *value;

  if (format_string != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      value = g_variant_new_va (format_string, NULL, &ap);
      va_end (ap);
    }
  else
    value = NULL;

  hud_action_description_set_attribute_value (description, attribute_name, value);
}

/**
 * hud_action_description_get_action_name:
 * @description: a #HudActionDescription
 *
 * Gets the action name of @description.
 *
 * This, together with the action target, uniquely identify an action
 * description.
 *
 * Returns: (transfer none): the action name
 **/
const gchar *
hud_action_description_get_action_name (HudActionDescription *description)
{
  return description->action;
}

/**
 * hud_action_description_get_action_target:
 * @description: a #HudActionDescription
 *
 * Gets the action target of @description (ie: the #GVariant that will
 * be passed to invocations of the action).
 *
 * This may be %NULL.
 *
 * This, together with the action name, uniquely identify an action
 * description.
 *
 * Returns: (transfer none): the target value
 **/
GVariant *
hud_action_description_get_action_target (HudActionDescription *description)
{
  return description->target;
}

/**
 * hud_action_description_set_parameterized:
 * @parent: a #HudActionDescription
 * @child: The child #GMenuModel to add
 *
 * A function to put one action description as a child for the first
 * one.  This is used for parameterized actions where one can set up
 * children that are displayed on the 'dialog' mode of the HUD.
 */
void
hud_action_description_set_parameterized (HudActionDescription * parent, GMenuModel * child)
{
	g_return_if_fail(HUD_IS_ACTION_DESCRIPTION(parent));
	g_return_if_fail(child == NULL || G_IS_MENU_MODEL(child)); /* NULL is allowed to clear it */

	if (parent->links == NULL) {
		parent->links = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	}

	if (child != NULL) {
		g_hash_table_insert(parent->links, g_strdup(G_MENU_LINK_SUBMENU), g_object_ref(child));
	} else {
		g_hash_table_remove(parent->links, G_MENU_LINK_SUBMENU);
	}

	g_signal_emit (parent, hud_action_description_changed_signal,
	               g_quark_try_string (G_MENU_LINK_SUBMENU), G_MENU_LINK_SUBMENU);

	return;
}

/**
 * hud_action_description_ref:
 * @description: A #HudActionDescription
 *
 * Increase the reference count to an action description
 *
 * Return value: (transfer none): Value of @description
 */
HudActionDescription *
hud_action_description_ref (HudActionDescription  * description)
{
	g_return_val_if_fail(HUD_IS_ACTION_DESCRIPTION(description), NULL);

	return HUD_ACTION_DESCRIPTION(g_object_ref(description));
}

/**
 * hud_action_description_unref:
 * @description: A #HudActionDescription
 *
 * Decrease the reference count to an action description
 */
void
hud_action_description_unref (HudActionDescription * description)
{
	g_return_if_fail(HUD_IS_ACTION_DESCRIPTION(description));

	g_object_unref(description);
}
