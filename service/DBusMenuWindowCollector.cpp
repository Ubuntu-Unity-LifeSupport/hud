/*
 * Copyright (C) 2016 Canonical, Ltd.
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
 * Author: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <common/DBusTypes.h>
#include <common/WindowStackInterface.h>
#include <service/AppmenuRegistrarInterface.h>
#include <service/DBusMenuWindowCollector.h>
#include <service/Factory.h>

#include <QStringList>

using namespace hud::common;
using namespace hud::service;

DBusMenuWindowCollector::DBusMenuWindowCollector(unsigned int windowId,
		QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack,
		QSharedPointer<ComCanonicalAppMenuRegistrarInterface> registrar,
		Factory &factory) :
		m_windowId(windowId), m_registrar(registrar), m_factory(factory) {

	connect(registrar.data(),
		SIGNAL(WindowRegistered(uint, const QString &, const QDBusObjectPath &)),
		this,
		SLOT(WindowRegistered(uint, const QString &, const QDBusObjectPath &)));

	QDBusPendingReply<QStringList> windowDBusAddressReply(
		windowStack->GetWindowBusAddress(windowId));

	// Window action menu
	windowDBusAddressReply.waitForFinished();
	if (!windowDBusAddressReply.isError()) {
		QStringList windowDBusAddress(windowDBusAddressReply);

		if (windowDBusAddress.size() == 2) {
			const QString &name = windowDBusAddress.at(0);
			const QString &path = windowDBusAddress.at(1);

			if (!name.isEmpty() && !path.isEmpty())
				m_am_collector = factory.newDBusMenuCollector(name, QDBusObjectPath(path));
		}
	}

	// AppMenu
	QDBusPendingReply<QString, QDBusObjectPath> windowReply =
		registrar->GetMenuForWindow(m_windowId);

	windowReply.waitForFinished();
	if (windowReply.isError()) {
		return;
	}

	windowRegistered(windowReply.argumentAt<0>(), windowReply.argumentAt<1>());
}

DBusMenuWindowCollector::~DBusMenuWindowCollector() {
}

bool DBusMenuWindowCollector::isValid() const {
	return m_collector || m_am_collector;
}

static void setPropertyForAllActions(QMenu *menu) {
	if (!menu)
		return;

	for (QAction *action : menu->actions()) {
		if (!action->isEnabled()) {
			continue;
		}

		if (action->isSeparator()) {
			continue;
		}

		action->setProperty("searchByMnemonic", true);
		setPropertyForAllActions(action->menu());
	}
}

QList<CollectorToken::Ptr> DBusMenuWindowCollector::activate() {
	QList<CollectorToken::Ptr> ret;

	if (m_am_collector) {
		QList<CollectorToken::Ptr> tokens = m_am_collector->activate();

		for (CollectorToken::Ptr token : tokens) {
			setPropertyForAllActions(token->menu());
		}

		ret.append(tokens);
	}

	if (m_collector) {
		ret.append(m_collector->activate());
	}

	return ret;
}

void DBusMenuWindowCollector::deactivate() {
}

void DBusMenuWindowCollector::WindowRegistered(uint windowId, const QString &service,
		const QDBusObjectPath &menuObjectPath) {
	// Simply ignore updates for other windows
	if (windowId != m_windowId) {
		return;
	}

	windowRegistered(service, menuObjectPath);
}

void DBusMenuWindowCollector::windowRegistered(const QString &service,
                                               const QDBusObjectPath &menuObjectPath) {

	if (service.isEmpty()) {
		return;
	}

	disconnect(m_registrar.data(),
		SIGNAL(WindowRegistered(uint, const QString &, const QDBusObjectPath &)),
		this,
		SLOT(WindowRegistered(uint, const QString &, const QDBusObjectPath &)));

	m_collector = m_factory.newDBusMenuCollector(service, menuObjectPath);
}
