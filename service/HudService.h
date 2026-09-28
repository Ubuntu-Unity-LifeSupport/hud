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

#ifndef HUD_SERVICE_SERVICE_H_
#define HUD_SERVICE_SERVICE_H_

#include <service/Query.h>

#include <QDBusObjectPath>
#include <QSharedPointer>

class HudAdaptor;

namespace hud {
namespace service {

class Factory;

class Q_DECL_EXPORT HudService: public QObject {

public:
	typedef QSharedPointer<HudService> Ptr;

	explicit HudService(QObject *parent = 0);

	virtual ~HudService();

	virtual Query::Ptr closeQuery(const QDBusObjectPath &path) = 0;
};

}
}

#endif /* HUD_SERVICE_SERVICE_H_ */
