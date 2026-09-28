/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <common/query-columns.h>
#include <common/ResultsModel.h>
#include <QString>
#include <glib.h>

const QString RESULTS_FORMAT_STRING("com.canonical.hud.query%1.results");

using namespace hud::common;

ResultsModel::ResultsModel(unsigned int id) :
		HudDee(RESULTS_FORMAT_STRING.arg(id).toStdString()) {

	setSchema(results_model_schema, G_N_ELEMENTS(results_model_schema));
}

ResultsModel::~ResultsModel() {
}

void ResultsModel::addResult(qulonglong id, const QString &commandName,
		const QList<QPair<int, int>> &commandHighlights,
		const QString &description,
		const QList<QPair<int, int>> &descriptionHighlights,
		const QString &shortcut, int distance, bool parameterized) {

	GVariant *actionh = NULL;
	if (commandHighlights.isEmpty()) {
		actionh = g_variant_new_array(G_VARIANT_TYPE("(ii)"), NULL, 0);
	} else {
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ii)"));
		for (const QPair<int, int> &highlight : commandHighlights) {
			g_variant_builder_add(&builder, "(ii)", highlight.first,
					highlight.second);
		}
		actionh = g_variant_builder_end(&builder);
	}

	GVariant *desch = NULL;
	if (descriptionHighlights.isEmpty()) {
		desch = g_variant_new_array(G_VARIANT_TYPE("(ii)"), NULL, 0);
	} else {
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ii)"));
		for (const QPair<int, int> &highlight : descriptionHighlights) {
			g_variant_builder_add(&builder, "(ii)", highlight.first,
					highlight.second);
		}
		desch = g_variant_builder_end(&builder);
	}

	GVariant *columns[HUD_QUERY_RESULTS_COUNT + 1];
	columns[HUD_QUERY_RESULTS_COMMAND_ID] = g_variant_new_variant(
			g_variant_new_uint64(id));
	columns[HUD_QUERY_RESULTS_COMMAND_NAME] = g_variant_new_string(
			commandName.toUtf8().data());
	columns[HUD_QUERY_RESULTS_COMMAND_HIGHLIGHTS] = actionh;
	columns[HUD_QUERY_RESULTS_DESCRIPTION] = g_variant_new_string(
			description.toUtf8().data());
	columns[HUD_QUERY_RESULTS_DESCRIPTION_HIGHLIGHTS] = desch;
	columns[HUD_QUERY_RESULTS_SHORTCUT] = g_variant_new_string(
			shortcut.toUtf8().data());
	columns[HUD_QUERY_RESULTS_DISTANCE] = g_variant_new_uint32(distance);
	columns[HUD_QUERY_RESULTS_PARAMETERIZED] = g_variant_new_boolean(
			parameterized);
	columns[HUD_QUERY_RESULTS_COUNT] = NULL;

	appendRow(columns);
}
