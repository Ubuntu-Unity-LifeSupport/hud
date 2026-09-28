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

#include <common/WindowInfo.h>

#include <QDBusMetaType>

using namespace hud::common;

QDBusArgument & operator<<(QDBusArgument &a, const WindowInfo &wi) {
	a.beginStructure();
	a << wi.window_id << wi.app_id << wi.focused << wi.stage;
	a.endStructure();
	return a;
}

const QDBusArgument & operator>>(const QDBusArgument &a, WindowInfo &wi) {
	a.beginStructure();
	uint stage;
	a >> wi.window_id >> wi.app_id >> wi.focused >> stage;
	a.endStructure();
	wi.stage = static_cast<WindowInfo::Stage>(stage);
	return a;
}

WindowInfo::WindowInfo() :
		window_id(0), focused(false), stage(MAIN) {
}

WindowInfo::WindowInfo(unsigned int window_id, const QString &app_id,
		bool focused, Stage stage) :
		window_id(window_id), app_id(app_id), focused(focused), stage(stage) {
}

WindowInfo::~WindowInfo() {
}

bool WindowInfo::operator==(const WindowInfo &other) const {
	return (window_id == other.window_id) && (app_id == other.app_id)
			&& (focused == other.focused) && (stage == other.stage);
}

void WindowInfo::registerMetaTypes() {
	qRegisterMetaType<WindowInfo>();
	qRegisterMetaType<WindowInfoList>();
	qDBusRegisterMetaType<WindowInfo>();
	qDBusRegisterMetaType<WindowInfoList>();
}
