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

#include <common/Description.h>

#include <QDBusMetaType>

using namespace hud::common;

QDBusArgument & operator<<(QDBusArgument &argument,
		const Description &description) {
	argument.beginStructure();
	argument << description.m_windowId << description.m_context
			<< description.m_object;
	argument.endStructure();
	return argument;
}

const QDBusArgument & operator>>(const QDBusArgument &argument,
		Description &description) {
	argument.beginStructure();
	argument >> description.m_windowId >> description.m_context
			>> description.m_object;
	argument.endStructure();
	return argument;
}

Description::Description() :
		m_windowId(0) {
}

Description::~Description() {
}

void Description::registerMetaTypes() {
	qRegisterMetaType<Description>();
	qDBusRegisterMetaType<Description>();

	qRegisterMetaType<QList<Description>>();
	qDBusRegisterMetaType<QList<Description>>();
}
