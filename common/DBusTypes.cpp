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

#include <common/DBusTypes.h>
#include <common/Action.h>
#include <common/ActionGroup.h>
#include <common/Description.h>
#include <common/MenuModel.h>
#include <common/NameObject.h>
#include <common/Suggestion.h>
#include <common/WindowInfo.h>

#include <QList>

using namespace hud::common;

const QString DBusTypes::HUD_SERVICE_DBUS_NAME("com.canonical.hud");
const QString DBusTypes::HUD_SERVICE_DBUS_PATH("/com/canonical/hud");

const QString DBusTypes::WINDOW_STACK_DBUS_NAME(
		"com.canonical.Unity.WindowStack");
const QString DBusTypes::WINDOW_STACK_DBUS_PATH(
		"/com/canonical/Unity/WindowStack");

const QString DBusTypes::APPMENU_REGISTRAR_DBUS_NAME(
		"com.canonical.AppMenu.Registrar");
const QString DBusTypes::APPMENU_REGISTRAR_DBUS_PATH(
		"/com/canonical/AppMenu/Registrar");

const QString DBusTypes::BAMF_DBUS_NAME("org.ayatana.bamf");
const QString DBusTypes::BAMF_MATCHER_DBUS_PATH("/org/ayatana/bamf/matcher");

void DBusTypes::registerMetaTypes() {
	NameObject::registerMetaTypes();
	Suggestion::registerMetaTypes();
	WindowInfo::registerMetaTypes();
	Action::registerMetaTypes();
	ActionGroup::registerMetaTypes();
	Description::registerMetaTypes();
	MenuModel::registerMetaTypes();
}

QString DBusTypes::queryPath(unsigned int id) {
	return QString("/com/canonical/hud/query/%1").arg(id);
}

QString DBusTypes::applicationPath(const QString &applicationId) {
	QString path("/com/canonical/hud/applications/");
	for (const unsigned char &c : applicationId.toUtf8()) {
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9')) {
			path.append(c);
		} else {
			path.append("_");
			path.append(QString().sprintf("%02x", c));
		}
	}
	return path;
}
