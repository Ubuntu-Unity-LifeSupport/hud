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

#include <AbstractWindowStack.h>
#include <WindowStackAdaptor.h>
#include <common/DBusTypes.h>
#include <common/Localisation.h>
#include <common/WindowInfo.h>

#include <stdexcept>
#include <QDBusMetaType>
#include <QDebug>

using namespace hud::common;

AbstractWindowStack::AbstractWindowStack(const QDBusConnection &connection,
		QObject *parent) :
		QObject(parent), m_adaptor(new WindowStackAdaptor(this)), m_connection(
				connection) {
	WindowInfo::registerMetaTypes();
}

void AbstractWindowStack::registerOnBus() {
	if (!m_connection.registerObject(DBusTypes::WINDOW_STACK_DBUS_PATH, this)) {
		throw std::logic_error(
				_("Unable to register window stack object on DBus"));
	}
	if (!m_connection.registerService(DBusTypes::WINDOW_STACK_DBUS_NAME)) {
		throw std::logic_error(
				_("Unable to register window stack service on DBus"));
	}
}

AbstractWindowStack::~AbstractWindowStack() {
	m_connection.unregisterObject(DBusTypes::WINDOW_STACK_DBUS_PATH);
}

