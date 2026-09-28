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

#ifndef HUD_SERVICE_QGSETTINGSSEARCHSETTINGS_H_
#define HUD_SERVICE_QGSETTINGSSEARCHSETTINGS_H_

#include <service/SearchSettings.h>

#include <QGSettings/qgsettings.h>

namespace hud {
namespace service {

class QGSettingsSearchSettings: public SearchSettings {
public:
	QGSettingsSearchSettings();

	virtual ~QGSettingsSearchSettings();

	uint addPenalty() const;

	uint dropPenalty() const;

	uint endDropPenalty() const;

	uint swapPenalty() const;

protected:
	QGSettings m_settings;
};

}
}

#endif /* HUD_SERVICE_QGSETTINGSSEARCHSETTINGS_H_ */
