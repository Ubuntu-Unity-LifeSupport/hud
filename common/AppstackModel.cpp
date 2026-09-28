/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU Lesser General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <common/query-columns.h>
#include <common/AppstackModel.h>

#include <QString>
#include <glib.h>

using namespace hud::common;

const QString APPSTACK_FORMAT_STRING("com.canonical.hud.query%1.appstack");

AppstackModel::AppstackModel(unsigned int id) :
		HudDee(APPSTACK_FORMAT_STRING.arg(id).toStdString()) {
	setSchema(appstack_model_schema, G_N_ELEMENTS(appstack_model_schema));
}

AppstackModel::~AppstackModel() {
}

static gint appstack_sort(GVariant **row1, GVariant **row2,
		gpointer user_data) {
	Q_UNUSED(user_data);

	gint32 type1 = g_variant_get_int32(row1[HUD_QUERY_APPSTACK_ITEM_TYPE]);
	gint32 type2 = g_variant_get_int32(row2[HUD_QUERY_APPSTACK_ITEM_TYPE]);

	/* If the types are the same, we'll sort by ID */
	if (type1 == type2) {
		const gchar * app_id1 = g_variant_get_string(
				row1[HUD_QUERY_APPSTACK_APPLICATION_ID], NULL);
		const gchar * app_id2 = g_variant_get_string(
				row2[HUD_QUERY_APPSTACK_APPLICATION_ID], NULL);

		return g_strcmp0(app_id1, app_id2);
	}

	return type1 - type2;
}

void AppstackModel::addApplication(const QString &applicationId,
		const QString &iconName, ItemType itemType) {
	GVariant * columns[HUD_QUERY_APPSTACK_COUNT + 1];
	columns[HUD_QUERY_APPSTACK_APPLICATION_ID] = g_variant_new_string(
			applicationId.toUtf8().data());
	columns[HUD_QUERY_APPSTACK_ICON_NAME] = g_variant_new_string(
			iconName.toUtf8().data());
	columns[HUD_QUERY_APPSTACK_ITEM_TYPE] = g_variant_new_int32(itemType);
	columns[HUD_QUERY_APPSTACK_COUNT] = NULL;

	insertRowSorted(columns, appstack_sort);
}
