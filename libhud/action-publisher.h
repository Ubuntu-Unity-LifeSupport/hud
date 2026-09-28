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

#if !defined (_HUD_H_INSIDE) && !defined (HUD_COMPILATION)
#error "Only <hud.h> can be included directly."
#endif

#ifndef __HUD_ACTION_PUBLISHER_H__
#define __HUD_ACTION_PUBLISHER_H__

#pragma GCC visibility push(default)

#include <gio/gio.h>

G_BEGIN_DECLS

#define HUD_TYPE_ACTION_PUBLISHER                           (hud_action_publisher_get_type ())
#define HUD_ACTION_PUBLISHER(inst)                          (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_ACTION_PUBLISHER, HudActionPublisher))
#define HUD_IS_ACTION_PUBLISHER(inst)                       (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_ACTION_PUBLISHER))

/**
 * HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_ADDED:
 *
 * Define for the string to access the signal HudActionPublisher::action-group-added
 */
#define HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_ADDED      "action-group-added"
/**
 * HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_REMOVED:
 *
 * Define for the string to access the signal HudActionPublisher::action-group-removed
 */
#define HUD_ACTION_PUBLISHER_SIGNAL_ACTION_GROUP_REMOVED    "action-group-removed"

GType hud_action_description_get_type (void);

#define HUD_TYPE_ACTION_DESCRIPTION                         (hud_action_description_get_type ())
#define HUD_ACTION_DESCRIPTION(inst)                        (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_ACTION_DESCRIPTION, HudActionDescription))
#define HUD_IS_ACTION_DESCRIPTION(inst)                     (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_ACTION_DESCRIPTION))

/**
 * HUD_ACTION_PUBLISHER_NO_CONTEXT:
 *
 * Can be passed to hud_action_publisher_new() to request that it build
 * it's own context.
 */
#define HUD_ACTION_PUBLISHER_NO_CONTEXT                     (NULL)
/**
 * HUD_ACTION_PUBLISHER_ALL_WINDOWS:
 *
 * Can be passed to hud_action_publisher_new() to request that these actions
 * apply to all windows for the application.
 */
#define HUD_ACTION_PUBLISHER_ALL_WINDOWS                    (0)

typedef struct _HudActionDescription                        HudActionDescription;
typedef struct _HudActionPublisher                          HudActionPublisher;
typedef struct _HudActionPublisherActionGroupSet            HudActionPublisherActionGroupSet;

/**
 * HudActionPublisher:
 *
 * An object representing the actions that are published for a
 * particular context within the application.  Most often this is
 * a window, but could also be used for tabs or other modal style
 * user contexts in the application.
 */

/**
 * HudActionPublisherActionGroupSet:
 * @prefix: Action prefix for the action group
 * @path: Path that the action group is exported on DBus
 *
 * A set of properties of that describe the action group.
 */
struct _HudActionPublisherActionGroupSet {
	gchar * prefix;
	gchar * path;
};

GType                   hud_action_publisher_get_type                   (void) G_GNUC_CONST;

HudActionPublisher *    hud_action_publisher_new                        (guint                  window_id,
                                                                         const gchar *          context_id);

HudActionPublisher *    hud_action_publisher_new_for_application        (GApplication          *application);

void                    hud_action_publisher_add_description            (HudActionPublisher    *publisher,
                                                                         HudActionDescription  *description);

void                    hud_action_publisher_add_action_group           (HudActionPublisher    *publisher,
                                                                         const gchar           *prefix,
                                                                         const gchar           *object_path);
void                    hud_action_publisher_remove_action_group        (HudActionPublisher    *publisher,
                                                                         const gchar           *prefix,
                                                                         GVariant              *identifier);
guint                   hud_action_publisher_get_window_id              (HudActionPublisher    *publisher);
const gchar *           hud_action_publisher_get_context_id             (HudActionPublisher    *publisher);
GList *                 hud_action_publisher_get_action_groups          (HudActionPublisher    *publisher);
const gchar *           hud_action_publisher_get_description_path       (HudActionPublisher    *publisher);

/* Description */
HudActionDescription *  hud_action_description_new                      (const gchar           *action_name,
                                                                         GVariant              *action_target);
HudActionDescription *  hud_action_description_ref                      (HudActionDescription  *description);
void                    hud_action_description_unref                    (HudActionDescription  *description);
const gchar *           hud_action_description_get_action_name          (HudActionDescription  *description);
GVariant *              hud_action_description_get_action_target        (HudActionDescription  *description);
void                    hud_action_description_set_attribute_value      (HudActionDescription  *description,
                                                                         const gchar           *attribute_name,
                                                                         GVariant              *value);
void                    hud_action_description_set_attribute            (HudActionDescription  *description,
                                                                         const gchar           *attribute_name,
                                                                         const gchar           *format_string,
                                                                         ...);
void                    hud_action_description_set_parameterized        (HudActionDescription  *parent,
                                                                         GMenuModel            *child);

/**
 * SECTION:action-publisher
 * @short_description: Publish action data to the HUD
 * @stability: Stable
 * @include: libhud/action-publisher.h
 *
 * Each context in the application should have a #HudActionPublisher
 * object to represents the actions that are available to the user
 * when that window and context are visible.  This acts as a set of
 * actions that can be activated by either the window manager changing
 * focus or the application changing contexts.
 *
 * On each action publisher there exits several action groups which can
 * be separated by allowing different prefixes for those action groups.
 * A particular prefix should only be used once per action publisher, but
 * an action group can by used by several action publishers.
 *
 * The values describing the action, including the label and description that
 * show up in the HUD are set via creating a #HudActionDescription for a
 * action.  Each action can have more than one description if there
 * is a reason to do so.  But, it is probably better to use the keywords
 * attribute in the majority cases.
 */

G_END_DECLS

#pragma GCC visibility pop

#endif /* __HUD_ACTION_PUBLISHER_H__ */
