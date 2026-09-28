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

#include <common/Action.h>

#include <QDBusMetaType>

using namespace hud::common;

QDBusArgument & operator<<(QDBusArgument &argument, const Action &action) {
	argument.beginStructure();
	argument << action.m_windowId << action.m_context << action.m_prefix
			<< action.m_object;
	argument.endStructure();
	return argument;
}

const QDBusArgument & operator>>(const QDBusArgument &argument,
		Action &action) {
	argument.beginStructure();
	argument >> action.m_windowId >> action.m_context >> action.m_prefix
			>> action.m_object;
	argument.endStructure();
	return argument;
}

Action::Action() :
		m_windowId(0) {
}

Action::~Action() {
}

void Action::registerMetaTypes() {
	qRegisterMetaType<Action>();
	qDBusRegisterMetaType<Action>();

	qRegisterMetaType<QList<Action>>();
	qDBusRegisterMetaType<QList<Action>>();
}
