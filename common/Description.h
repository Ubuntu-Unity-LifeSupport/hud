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

#ifndef HUD_COMMON_DESCRIPTION_H_
#define HUD_COMMON_DESCRIPTION_H_

#include <QObject>
#include <QDBusArgument>

namespace hud {
namespace common {

class Description {
public:
	Description();

	virtual ~Description();

	static void registerMetaTypes();

	unsigned int m_windowId;

	QString m_context;

	QDBusObjectPath m_object;
};

}
}

Q_DECL_EXPORT
QDBusArgument &operator<<(QDBusArgument &argument,
		const hud::common::Description &description);

Q_DECL_EXPORT
const QDBusArgument &operator>>(const QDBusArgument &argument,
		hud::common::Description &description);

Q_DECLARE_METATYPE(hud::common::Description)

#endif /* HUD_COMMON_DESCRIPTION_H_ */
