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

#ifndef HUD_SERVICE_RESULT_H_
#define HUD_SERVICE_RESULT_H_

#include <QString>
#include <QList>
#include <QPair>

namespace hud {
namespace service {

class Result {
public:
	typedef QPair<int, int> Highlight;
	typedef QList<Highlight> HighlightList;

	explicit Result();

	Result(qulonglong id, const QString &commandName,
			const HighlightList &commandHighlights, const QString &description,
			const HighlightList &descriptionHighlights, const QString &shortcut,
			int distance, bool parameterized);

	virtual ~Result();

	qulonglong id() const;

	const QString & commandName() const;

	const HighlightList & commandHighlights() const;

	const QString & description() const;

	const HighlightList & descriptionHighlights() const;

	const QString & shortcut() const;

	int distance() const;

	bool parameterized() const;

protected:
	uint64_t m_id;

	QString m_commandName;

	HighlightList m_commandHighlights;

	QString m_description;

	HighlightList m_descriptionHighlights;

	QString m_shortcut;

	int m_distance;

	bool m_parameterized;
};

}
}
#endif /* HUD_SERVICE_RESULT_H_ */
