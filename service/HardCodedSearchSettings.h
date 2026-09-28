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

#ifndef HUD_SERVICE_HARDCODEDSEARCHSETTINGS_H_
#define HUD_SERVICE_HARDCODEDSEARCHSETTINGS_H_

#include <service/SearchSettings.h>

namespace hud {
namespace service {

class HardCodedSearchSettings: public SearchSettings {
public:
	HardCodedSearchSettings();

	virtual ~HardCodedSearchSettings();

	uint addPenalty() const;

	void setAddPenalty(uint penalty);

	uint dropPenalty() const;

	void setDropPenalty(uint penalty);

	uint endDropPenalty() const;

	void setEndDropPenalty(uint penalty);

	uint swapPenalty() const;

	void setSwapPenalty(uint penalty);

protected:
	uint m_addPenalty = 100;

	uint m_dropPenalty = 100;

	uint m_endDropPenalty = 20;

	uint m_swapPenalty = 150;
};

}
}

#endif /* HUD_SERVICE_HARDCODEDSEARCHSETTINGS_H_ */
