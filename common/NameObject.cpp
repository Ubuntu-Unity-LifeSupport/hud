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

#include <common/NameObject.h>

#include <QDBusMetaType>

using namespace hud::common;

QDBusArgument & operator<<(QDBusArgument &argument,
		const NameObject &nameObject) {
	argument.beginStructure();
	argument << nameObject.m_name << nameObject.m_object;
	argument.endStructure();
	return argument;
}

const QDBusArgument & operator>>(const QDBusArgument &argument,
		NameObject &nameObject) {
	argument.beginStructure();
	argument >> nameObject.m_name >> nameObject.m_object;
	argument.endStructure();
	return argument;
}

namespace hud {
namespace common {

NameObject::NameObject() {
}

NameObject::NameObject(const QString &name, const QDBusObjectPath &object) :
		m_name(name), m_object(object) {
}

NameObject::NameObject(const NameObject &other) :
		m_name(other.m_name), m_object(other.m_object) {
}

NameObject & NameObject::operator=(const NameObject &other) {
	m_name = other.m_name;
	m_object = other.m_object;
	return *this;
}

bool NameObject::operator==(const NameObject &other) const {
	return m_name == other.m_name && m_object == other.m_object;
}

NameObject::~NameObject() {
}

void NameObject::registerMetaTypes() {
	qRegisterMetaType<NameObject>();
	qDBusRegisterMetaType<NameObject>();

	qRegisterMetaType<QList<NameObject>>();
	qDBusRegisterMetaType<QList<NameObject>>();
}

}
}
