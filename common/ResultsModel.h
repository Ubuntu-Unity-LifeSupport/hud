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

#ifndef HUD_COMMON_RESULTSMODEL_H_
#define HUD_COMMON_RESULTSMODEL_H_

#include <common/HudDee.h>

#include <QList>
#include <QPair>
#include <QString>

namespace hud {
namespace common {

class ResultsModel: public HudDee {
public:
	explicit ResultsModel(unsigned int id);

	virtual ~ResultsModel();

	void setResults();

	void addResult(qulonglong id, const QString &command,
			const QList<QPair<int, int>> &commandHighlights,
			const QString &description,
			const QList<QPair<int, int>> &descriptionHighlights,
			const QString &shortcut, int distance, bool parameterized);
};

}
}

#endif /* HUD_COMMON_RESULTSMODEL_H_ */
