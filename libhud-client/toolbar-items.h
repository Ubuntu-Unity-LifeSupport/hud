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

#ifndef LIBHUD_CLIENT_TOOLBAR_ITEMS_H_
#define LIBHUD_CLIENT_TOOLBAR_ITEMS_H_

/**
 * HudClientQueryToolbarItems:
 * @HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN: Make the application fullscreen
 * @HUD_CLIENT_QUERY_TOOLBAR_HELP: Help the user use the application
 * @HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES: Configure the application
 * @HUD_CLIENT_QUERY_TOOLBAR_UNDO: Revert the last user action
 * @HUD_CLIENT_QUERY_TOOLBAR_QUIT: Quit the application
 *
 * @short_description: Preconfigured toolbar items for the application
 *
 * The toolbar has a set of preconfigured items in it for the
 * application.  This enum represents them.
 */
typedef enum { /*< prefix=HUD_CLIENT_QUERY_TOOLBAR >*/
	HUD_CLIENT_QUERY_TOOLBAR_INVALID = -1,
	HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN,
	HUD_CLIENT_QUERY_TOOLBAR_HELP,
	HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES,
	HUD_CLIENT_QUERY_TOOLBAR_UNDO,
	HUD_CLIENT_QUERY_TOOLBAR_QUIT,
} HudClientQueryToolbarItems;

#endif /* LIBHUD_CLIENT_TOOLBAR_ITEMS_H_ */
