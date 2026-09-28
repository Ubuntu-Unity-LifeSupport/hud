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

#ifndef HUD_COMMON_APPSTACKMODEL_H_
#define HUD_COMMON_APPSTACKMODEL_H_

#include <common/HudDee.h>

#include <QString>

namespace hud {
namespace common {

class AppstackModel: public HudDee {
public:
	typedef enum {
		ITEM_TYPE_FOCUSED_APP,
		ITEM_TYPE_SIDESTAGE_APP,
		ITEM_TYPE_BACKGROUND_APP,
		ITEM_TYPE_INDICATOR
	} ItemType;

	explicit AppstackModel(unsigned int id);

	virtual ~AppstackModel();

	void addApplication(const QString &applicationId, const QString &iconName,
			ItemType itemType);
protected:
};

}
}

#endif /* HUD_COMMON_APPSTACKMODEL_H_ */
