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

#ifndef HUD_COMMON_DBUSTYPES_H_
#define HUD_COMMON_DBUSTYPES_H_

#include <common/Action.h>
#include <common/ActionGroup.h>
#include <common/Description.h>
#include <common/MenuModel.h>
#include <common/NameObject.h>
#include <common/Suggestion.h>

namespace hud {
namespace common {

class DBusTypes {
public:
	Q_DECL_EXPORT
	static const QString HUD_SERVICE_DBUS_NAME;

	Q_DECL_EXPORT
	static const QString HUD_SERVICE_DBUS_PATH;

	Q_DECL_EXPORT
	static const QString WINDOW_STACK_DBUS_NAME;

	Q_DECL_EXPORT
	static const QString WINDOW_STACK_DBUS_PATH;

	Q_DECL_EXPORT
	static const QString APPMENU_REGISTRAR_DBUS_NAME;

	Q_DECL_EXPORT
	static const QString APPMENU_REGISTRAR_DBUS_PATH;

	Q_DECL_EXPORT
	static const QString BAMF_DBUS_NAME;

	Q_DECL_EXPORT
	static const QString BAMF_MATCHER_DBUS_PATH;

	Q_DECL_EXPORT
	static const QString UNITY_VOICE_DBUS_NAME;

	Q_DECL_EXPORT
	static const QString UNITY_VOICE_DBUS_PATH;

	Q_DECL_EXPORT
	static void registerMetaTypes();

	Q_DECL_EXPORT
	static QString queryPath(unsigned int id);

	Q_DECL_EXPORT
	static QString applicationPath(const QString &applicationId);
};

}
}

#endif /* HUD_COMMON_DBUSTYPES_H_ */
