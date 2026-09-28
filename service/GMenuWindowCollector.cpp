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

#include <common/WindowStackInterface.h>
#include <service/Factory.h>
#include <service/GMenuWindowCollector.h>

#include <libqtgmenu/QtGMenuImporter.h>

#include <QStringList>
#include <QDebug>

using namespace hud::service;
using namespace qtgmenu;

static const QStringList GMENU_WINDOW_PROPERTIES( { "_GTK_UNIQUE_BUS_NAME",
		"_GTK_APP_MENU_OBJECT_PATH", "_GTK_MENUBAR_OBJECT_PATH",
		"_GTK_APPLICATION_OBJECT_PATH", "_GTK_WINDOW_OBJECT_PATH",
		"_UNITY_OBJECT_PATH" });

GMenuWindowCollector::GMenuWindowCollector(unsigned int windowId,
		const QString &applicationId,
		QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack,
		Factory &factory) :
		m_windowStack(windowStack) {

	QDBusPendingReply<QStringList> windowPropertiesReply(
			windowStack->GetWindowProperties(windowId, applicationId,
					GMENU_WINDOW_PROPERTIES));

	windowPropertiesReply.waitForFinished();
	if (windowPropertiesReply.isError()) {
		qWarning() << windowPropertiesReply.error();
		return;
	}
	QStringList windowProperties(windowPropertiesReply);

	if (windowProperties.isEmpty()) {
		return;
	}

	m_busName = windowProperties.at(0);

	// We're using the existence of the bus name property to determine
	// if this window has GMenus available at all.
	if (m_busName.isEmpty()) {
		return;
	}

	QSet<QDBusObjectPath> menus;
	QMap<QString, QDBusObjectPath> actions;

	if (!windowProperties.at(1).isEmpty()) {
		//	_GTK_APP_MENU_OBJECT_PATH -> menu
		menus << QDBusObjectPath(windowProperties.at(1));
	}
	if (!windowProperties.at(2).isEmpty()) {
		//	_GTK_MENUBAR_OBJECT_PATH -> menu
		menus << QDBusObjectPath(windowProperties.at(2));
	}
	if (!windowProperties.at(3).isEmpty()) {
		//	_GTK_APPLICATION_OBJECT_PATH -> actions with prefix "app"
		actions["app"] = QDBusObjectPath(windowProperties.at(3));
	}
	if (!windowProperties.at(4).isEmpty()) {
		//	_GTK_WINDOW_OBJECT_PATH -> actions with prefix "win"
		actions["win"] = QDBusObjectPath(windowProperties.at(4));
	}
	if (!windowProperties.at(5).isEmpty()) {
		//	_UNITY_OBJECT_PATH -> menu + action with prefix "unity"
		menus << QDBusObjectPath(windowProperties.at(5));
		actions["unity"] = QDBusObjectPath(windowProperties.at(5));
	}

	if (!actions.isEmpty()) {
		for (const QDBusObjectPath &menu : menus) {
			m_collectors << factory.newGMenuCollector(m_busName, actions, menu);
		}
	}
}

GMenuWindowCollector::~GMenuWindowCollector() {
}

bool GMenuWindowCollector::isValid() const {
	return !m_collectors.isEmpty();
}

QList<CollectorToken::Ptr> GMenuWindowCollector::activate() {
	QList<CollectorToken::Ptr> tokens;

	for (Collector::Ptr collector : m_collectors) {
		tokens.append(collector->activate());
	}

	return tokens;
}

void GMenuWindowCollector::deactivate() {
}
