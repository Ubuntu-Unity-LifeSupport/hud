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

#include <service/Factory.h>
#include <service/WindowContextImpl.h>
#include <QDebug>

#include <libqtgmenu/QtGMenuImporter.h>

using namespace hud::service;
using namespace qtgmenu;

WindowContextImpl::WindowContextImpl(Factory &factory) :
		m_factory(factory) {
}

WindowContextImpl::~WindowContextImpl() {
}

void WindowContextImpl::setContext(const QString &context) {
	if (m_context == context) {
		return;
	}

	m_context = context;
	m_activeCollector = m_collectors[context];

	contextChanged();
}

void WindowContextImpl::addMenu(const QString &context,
		const MenuDefinition &menuDefinition) {

	QMap<QString, QDBusObjectPath> actions;
	actions[menuDefinition.actionPrefix] = menuDefinition.actionPath;

	m_collectors[context] = m_factory.newGMenuCollector(menuDefinition.name,
			actions, menuDefinition.menuPath);
}

Collector::Ptr WindowContextImpl::activeCollector() {
	return m_activeCollector;
}
