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
#include <service/GMenuCollector.h>

#include <libqtgmenu/QtGMenuImporter.h>

#include <QStringList>
#include <QDebug>

using namespace hud::service;
using namespace qtgmenu;

GMenuCollector::GMenuCollector(const QString &name,
		const QMap<QString, QDBusObjectPath> &actions,
		const QDBusObjectPath &menuPath,
		Factory& factory) :
		m_name(name), m_actions(actions), m_menuPath(menuPath) {

	m_importer = factory.newQtGMenuImporter(m_name, m_menuPath, actions);

	connect(m_importer.data(), SIGNAL(MenuItemsChanged()), this,
			SLOT(menuItemsChanged()));
}

GMenuCollector::~GMenuCollector() {
}

bool GMenuCollector::isValid() const {
	return !m_menuPath.path().isEmpty();
}

QList<CollectorToken::Ptr> GMenuCollector::activate() {
	CollectorToken::Ptr collectorToken(m_collectorToken);

	std::shared_ptr<QMenu> menu(m_importer->GetQMenu());
	if (collectorToken.isNull() || menu != m_menu) {
		m_menu = menu;
		collectorToken.reset(
				new CollectorToken(shared_from_this(),
						m_menu ? m_menu.get() : nullptr));
		m_collectorToken = collectorToken;
	}

	return QList<CollectorToken::Ptr>() << collectorToken;
}

void GMenuCollector::deactivate() {
}

void GMenuCollector::menuItemsChanged() {
	CollectorToken::Ptr collectorToken(m_collectorToken);
	if (collectorToken) {
		collectorToken->changed();
	}
}
