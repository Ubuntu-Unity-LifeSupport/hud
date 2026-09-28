/*
 * Copyright (C) 2013-2016 Canonical, Ltd.
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
 *         Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <service/DBusMenuCollector.h>

#include <dbusmenuimporter.h>
#include <QDebug>
#include <QMenu>
#include <stdexcept>

using namespace hud::service;

DBusMenuCollector::DBusMenuCollector(const QString &service,
		const QDBusObjectPath &menuObjectPath) :
		m_service(service), m_path(menuObjectPath) {

	if (m_service.isEmpty()) {
		return;
	}

	m_menuImporter.reset(
		new DBusMenuImporter(m_service, m_path.path(), DBusMenuImporterType::SYNCHRONOUS));

	CollectorToken::Ptr collectorToken(m_collectorToken);
	if(collectorToken) {
		collectorToken->changed();
	}
}

DBusMenuCollector::~DBusMenuCollector() {
}

bool DBusMenuCollector::isValid() const {
	return !m_menuImporter.isNull();
}

void DBusMenuCollector::openMenu(QMenu *menu, unsigned int &limit) {
	--limit;
	if (limit == 0) {
		QString error = "Hit DBusMenu safety valve opening menu at " + m_service
				+ " " + m_path.path();
		throw std::logic_error(error.toStdString());
	}

	if (!menu) {
		return;
	}

	menu->aboutToShow();

	for (int i(0); m_menuImporter && i < menu->actions().size(); ++i) {
		QAction *action = menu->actions().at(i);
		if (!action->isEnabled()) {
			continue;
		}
		if (action->isSeparator()) {
			continue;
		}

		QMenu *child(action->menu());
		if (child) {
			openMenu(child, limit);
		}
	}
}

void DBusMenuCollector::hideMenu(QMenu *menu, unsigned int &limit) {
	--limit;
	if (limit == 0) {
		QString error = "Hit DBusMenu safety valve closing menu at " + m_service
				+ " " + m_path.path();
		throw std::logic_error(error.toStdString());
	}

	for (int i(0); i < menu->actions().size(); ++i) {
		QAction *action = menu->actions().at(i);
		QMenu *child(action->menu());
		if (child) {
			hideMenu(child, limit);
		}
	}

	menu->aboutToHide();

	if(!m_menuImporter) {
		return;
	}
}

QList<CollectorToken::Ptr> DBusMenuCollector::activate() {
	CollectorToken::Ptr collectorToken(m_collectorToken);

	if(m_menuImporter.isNull()) {
		return QList<CollectorToken::Ptr>();
	}

	if (collectorToken.isNull()) {
		try {
			unsigned int limit(50);
			openMenu(m_menuImporter->menu(), limit);
		} catch (std::logic_error &e) {
			qDebug() << e.what();
		}

		if(m_menuImporter.isNull()) {
			return QList<CollectorToken::Ptr>();
		}

		collectorToken.reset(
				new CollectorToken(shared_from_this(), m_menuImporter->menu()));
		m_collectorToken = collectorToken;
	}

	return QList<CollectorToken::Ptr>() << collectorToken;
}

void DBusMenuCollector::deactivate() {
	if(m_menuImporter.isNull()) {
		return;
	}
	try {
		unsigned int limit(50);
		hideMenu(m_menuImporter->menu(), limit);
	} catch (std::logic_error &e) {
		qDebug() << e.what();
	}
}
