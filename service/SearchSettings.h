/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <QObject>
#include <QSharedPointer>

#ifndef HUD_SERVICE_SEARCHSETTINGS_H_
#define HUD_SERVICE_SEARCHSETTINGS_H_

namespace hud {
namespace service {

class SearchSettings: public QObject {
Q_OBJECT

public:
	typedef QSharedPointer<SearchSettings> Ptr;

	SearchSettings();

	virtual ~SearchSettings();

	virtual uint addPenalty() const = 0;

	virtual uint dropPenalty() const = 0;

	virtual uint endDropPenalty() const = 0;

	virtual uint swapPenalty() const = 0;

Q_SIGNALS:
	void changed();
};

}
}

#endif /* HUD_SERVICE_SEARCHSETTINGS_H_ */
