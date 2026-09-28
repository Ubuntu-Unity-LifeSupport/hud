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

#ifndef HUD_COMMON_WINDOWINFO_H_
#define HUD_COMMON_WINDOWINFO_H_

#include <QObject>
#include <QDBusArgument>

namespace hud {
namespace common {

class WindowInfo {
public:
	enum Stage {
		MAIN, SIDE, WINDOWED
	};

	unsigned int window_id;
	QString app_id;
	bool focused;
	unsigned int stage;

	explicit WindowInfo();

	explicit WindowInfo(unsigned int window_id, const QString &app_id,
			bool focused, Stage stage = MAIN);

	virtual ~WindowInfo();

	bool operator==(const WindowInfo &other) const;

	static void registerMetaTypes();
};

typedef QList<WindowInfo> WindowInfoList;

}
}

Q_DECL_EXPORT
QDBusArgument &operator<<(QDBusArgument &a,
		const hud::common::WindowInfo &aidf);

Q_DECL_EXPORT
const QDBusArgument &operator>>(const QDBusArgument &a,
		hud::common::WindowInfo &aidf);

Q_DECLARE_METATYPE(hud::common::WindowInfo)
Q_DECLARE_METATYPE(hud::common::WindowInfoList)

#endif /* HUD_COMMON_WINDOWINFO_H_ */
