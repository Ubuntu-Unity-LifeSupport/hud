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

#ifndef __QUERY_COLUMNS_H__
#define __QUERY_COLUMNS_H__

enum _HudQueryResultsColumns {
	HUD_QUERY_RESULTS_COMMAND_ID = 0,
	HUD_QUERY_RESULTS_COMMAND_NAME,
	HUD_QUERY_RESULTS_COMMAND_HIGHLIGHTS,
	HUD_QUERY_RESULTS_DESCRIPTION,
	HUD_QUERY_RESULTS_DESCRIPTION_HIGHLIGHTS,
	HUD_QUERY_RESULTS_SHORTCUT,
	HUD_QUERY_RESULTS_DISTANCE,
	HUD_QUERY_RESULTS_PARAMETERIZED,
	/* Last */
	HUD_QUERY_RESULTS_COUNT
};
typedef enum _HudQueryResultsColumns HudQueryResultsColumns;

#define HUD_QUERY_RESULTS_COMMAND_ID_TYPE "v"
#define HUD_QUERY_RESULTS_COMMAND_NAME_TYPE "s"
#define HUD_QUERY_RESULTS_COMMAND_HIGHLIGHTS_TYPE "a(ii)"
#define HUD_QUERY_RESULTS_DESCRIPTION_TYPE "s"
#define HUD_QUERY_RESULTS_DESCRIPTION_HIGHLIGHTS_TYPE "a(ii)"
#define HUD_QUERY_RESULTS_SHORTCUT_TYPE "s"
#define HUD_QUERY_RESULTS_DISTANCE_TYPE "u"
#define HUD_QUERY_RESULTS_PARAMETERIZED_TYPE "b"

/* Schema that is used in the DeeModel representing
 the results */
static const char * results_model_schema[HUD_QUERY_RESULTS_COUNT] = {
HUD_QUERY_RESULTS_COMMAND_ID_TYPE,
HUD_QUERY_RESULTS_COMMAND_NAME_TYPE,
HUD_QUERY_RESULTS_COMMAND_HIGHLIGHTS_TYPE,
HUD_QUERY_RESULTS_DESCRIPTION_TYPE,
HUD_QUERY_RESULTS_DESCRIPTION_HIGHLIGHTS_TYPE,
HUD_QUERY_RESULTS_SHORTCUT_TYPE,
HUD_QUERY_RESULTS_DISTANCE_TYPE,
HUD_QUERY_RESULTS_PARAMETERIZED_TYPE, };

enum _HudQueryAppstackColumns {
	HUD_QUERY_APPSTACK_APPLICATION_ID = 0,
	HUD_QUERY_APPSTACK_ICON_NAME,
	HUD_QUERY_APPSTACK_ITEM_TYPE,
	/* Last */
	HUD_QUERY_APPSTACK_COUNT
};
typedef enum _HudQueryAppstackColumns HudQueryAppstackColumns;

#define HUD_QUERY_APPSTACK_APPLICATION_ID_TYPE "s"
#define HUD_QUERY_APPSTACK_ICON_NAME_TYPE "s"
#define HUD_QUERY_APPSTACK_ITEM_TYPE_TYPE "i"

/* Schema that is used in the DeeModel representing
 the appstack */
static const char * appstack_model_schema[HUD_QUERY_APPSTACK_COUNT] = {
HUD_QUERY_APPSTACK_APPLICATION_ID_TYPE,
HUD_QUERY_APPSTACK_ICON_NAME_TYPE,
HUD_QUERY_APPSTACK_ITEM_TYPE_TYPE, };

#endif /* __QUERY_COLUMNS_H__ */
