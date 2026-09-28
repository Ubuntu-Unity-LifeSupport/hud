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

#include <common/Localisation.h>
#include <common/Suggestion.h>

#include <QDebug>
#include <QDBusMetaType>
#include <glib.h>

using namespace hud::common;

QDBusArgument & operator<<(QDBusArgument &argument,
		const Suggestion &suggestion) {
	argument.beginStructure();
	argument << suggestion.m_description << suggestion.m_icon
			<< suggestion.m_unknown1 << suggestion.m_unknown2
			<< suggestion.m_unknown3 << QDBusVariant(suggestion.m_id);
	argument.endStructure();
	return argument;
}

const QDBusArgument & operator>>(const QDBusArgument &argument,
		Suggestion &suggestion) {
	argument.beginStructure();
	argument >> suggestion.m_description >> suggestion.m_icon
			>> suggestion.m_unknown1 >> suggestion.m_unknown2
			>> suggestion.m_unknown3 >> suggestion.m_id;
	argument.endStructure();
	return argument;
}

static void append(QString &result, const QString& input, int start, int end) {
	QStringRef substring(input.midRef(start, end));
	char *temp = g_markup_escape_text(substring.toUtf8().data(), -1);
	result.append(temp);
	g_free(temp);
}

/**
 * Builds a single line pango formatted description for
 * the legacy HUD UI.
 */
static QString buildLegacyHighlights(const QString &input,
		const QList<QPair<int, int>> &highlights) {

	QString result;
	int current(0);

	for (const QPair<int, int> &pair : highlights) {
		int start(pair.first);
		int stop(pair.second);

		// In theory there should be no overlapping, but we
		// want to make sure
		if (start < current) {
			qWarning() << "Overlapping highlighting.  At character" << current
					<< "and asked to highlight from" << start << "to" << stop;
			continue;
		}

		// Get to the start of the highlight
		int initialSkip = start - current;
		if (initialSkip > 0) {
			append(result, input, current, initialSkip);
		}

		result.append("<b>");

		// Copy the characters in the highlight
		int highlightSkip = stop - start;
		if (highlightSkip > 0) {
			append(result, input, start, highlightSkip);
		} else {
			qWarning() << "Zero character highlight!";
		}

		result.append("</b>");

		current = stop;
	}

	int remaining(input.length() - current);
	if (remaining > 0) {
		append(result, input, current, remaining);
	}

	return result;
}

Suggestion::Suggestion() {
}

Suggestion::Suggestion(qulonglong id, const QString &commandName,
		const QList<QPair<int, int>> &commandHighlights,
		const QString &description,
		const QList<QPair<int, int>> &descriptionHighlights,
		const QString &icon) :
		m_icon(icon), m_id(id) {

	if (description.isEmpty()) {
		m_description = buildLegacyHighlights(commandName, commandHighlights);
	} else {
		QString cmdHighlights(
				buildLegacyHighlights(commandName, commandHighlights));
		QString descHighlights(
				buildLegacyHighlights(description, descriptionHighlights));

		// TRANSLATORS: This is what is shown for Unity7 in
		// the HUD entries.  %1 is the command name and %2 is a
		// description or list of keywords that
		// was used to find the entry.
		m_description = QString(_("%1\xE2\x80\x82(%2)")).arg(cmdHighlights,
				descHighlights);
	}
}

Suggestion::~Suggestion() {
}

void Suggestion::registerMetaTypes() {
	qRegisterMetaType<Suggestion>();
	qDBusRegisterMetaType<Suggestion>();

	qRegisterMetaType<QList<Suggestion>>();
	qDBusRegisterMetaType<QList<Suggestion>>();
}
