/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY{} without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <service/Factory.h>
#include <service/ApplicationImpl.h>
#include <service/ApplicationListImpl.h>
#include <service/AppmenuRegistrarInterface.h>
#include <service/HardCodedSearchSettings.h>
#include <service/HudServiceImpl.h>
#include <service/WindowImpl.h>
#include <service/QGSettingsSearchSettings.h>
#include <service/QueryImpl.h>
#include <service/SqliteUsageTracker.h>
#include <service/VoiceImpl.h>
#include <service/WindowImpl.h>
#include <common/DBusTypes.h>

#include <libqtgmenu/QtGMenuImporter.h>

#include <QDBusConnection>
#include <QDBusServiceWatcher>

using namespace hud::common;
using namespace hud::service;

Factory::Factory() :
		m_sessionBus(QDBusConnection::sessionBus()), m_queryCounter(0) {
	DBusTypes::registerMetaTypes();
}

Factory::~Factory() {
}

void Factory::setSessionBus(const QDBusConnection &sessionBus) {
	m_sessionBus = sessionBus;
}

HudService::Ptr Factory::singletonHudService() {
	if (m_hudService.isNull()) {
		m_hudService.reset(
				new HudServiceImpl(*this, singletonApplicationList(),
						sessionBus()));
	}
	return m_hudService;
}

QSharedPointer<ComCanonicalUnityWindowStackInterface> Factory::singletonWindowStack() {
	if (m_windowStack.isNull()) {
		m_windowStack.reset(
				new ComCanonicalUnityWindowStackInterface(
						DBusTypes::WINDOW_STACK_DBUS_NAME,
						DBusTypes::WINDOW_STACK_DBUS_PATH, sessionBus()));
	}
	return m_windowStack;
}

QSharedPointer<QDBusServiceWatcher> Factory::windowStackWatcher() {
	return QSharedPointer<QDBusServiceWatcher>(
			new QDBusServiceWatcher(DBusTypes::WINDOW_STACK_DBUS_NAME,
					sessionBus(), QDBusServiceWatcher::WatchForUnregistration));
}

QSharedPointer<ComCanonicalAppMenuRegistrarInterface> Factory::singletonAppmenu() {
	if (m_appmenu.isNull()) {
		m_appmenu.reset(
				new ComCanonicalAppMenuRegistrarInterface(
						DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
						DBusTypes::APPMENU_REGISTRAR_DBUS_PATH, sessionBus()));
	}
	return m_appmenu;
}

QDBusConnection Factory::sessionBus() {
	return m_sessionBus;
}

QSharedPointer<GDBusConnection> Factory::gSessionBus() {
	return QSharedPointer<GDBusConnection>(g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr), &g_object_unref);
}

Query::Ptr Factory::newQuery(const QString &query, const QString &sender,
		Query::EmptyBehaviour emptyBehaviour) {
	return Query::Ptr(
			new QueryImpl(m_queryCounter++, query, sender, emptyBehaviour,
					*singletonHudService(), singletonApplicationList(),
					singletonVoice(), sessionBus()));
}

ApplicationList::Ptr Factory::singletonApplicationList() {
	if (m_applicationList.isNull()) {
		m_applicationList.reset(
				new ApplicationListImpl(*this, singletonWindowStack(),
						windowStackWatcher()));
	}
	return m_applicationList;
}

UsageTracker::Ptr Factory::singletonUsageTracker() {
	if (m_usageTracker.isNull()) {
		m_usageTracker.reset(new SqliteUsageTracker());
	}
	return m_usageTracker;
}

SearchSettings::Ptr Factory::singletonSearchSettings() {
	if (m_searchSettings.isNull()) {
		if (qEnvironmentVariableIsSet("HUD_IGNORE_SEARCH_SETTINGS")) {
			m_searchSettings.reset(new HardCodedSearchSettings());
		} else {
			m_searchSettings.reset(new QGSettingsSearchSettings());
		}
	}
	return m_searchSettings;
}

Voice::Ptr Factory::singletonVoice() {
	if (m_voice.isNull()) {
		m_voice.reset(new VoiceImpl());
	}
	return m_voice;
}

Application::Ptr Factory::newApplication(const QString &applicationId) {
	return Application::Ptr(
			new ApplicationImpl(applicationId, *this, sessionBus()));
}

ItemStore::Ptr Factory::newItemStore(const QString &applicationId) {
	return ItemStore::Ptr(
			new ItemStore(applicationId, singletonUsageTracker(),
					singletonSearchSettings()));
}

Window::Ptr Factory::newWindow(unsigned int windowId,
		const QString &applicationId, WindowContext::Ptr allwindowsContext) {
	return Window::Ptr(
			new WindowImpl(windowId, applicationId, allwindowsContext, *this));
}

WindowToken::Ptr Factory::newWindowToken(const QString &applicationId,
		QList<CollectorToken::Ptr> tokens) {
	return WindowToken::Ptr(new WindowTokenImpl(tokens, newItemStore(applicationId)));
}

WindowContext::Ptr Factory::newWindowContext() {
	return WindowContext::Ptr(new WindowContextImpl(*this));
}

Collector::Ptr Factory::newDBusMenuCollector(const QString &service,
		const QDBusObjectPath &menuObjectPath) {
	return Collector::Ptr(new DBusMenuCollector(service, menuObjectPath));
}

Collector::Ptr Factory::newGMenuCollector(const QString &name,
		const QMap<QString, QDBusObjectPath> &actions,
		const QDBusObjectPath &menuPath) {
	return Collector::Ptr(new GMenuCollector(name, actions, menuPath, *this));
}

QSharedPointer<qtgmenu::QtGMenuImporter> Factory::newQtGMenuImporter(
		const QString& service, const QDBusObjectPath& menu_path,
		const QMap<QString, QDBusObjectPath>& action_paths) {
	return QSharedPointer<qtgmenu::QtGMenuImporter>(
			new qtgmenu::QtGMenuImporter(service, menu_path, action_paths,
					sessionBus(), gSessionBus()));
}

Collector::Ptr Factory::newGMenuWindowCollector(unsigned int windowId,
		const QString &applicationId) {
	return Collector::Ptr(
			new GMenuWindowCollector(windowId, applicationId,
					singletonWindowStack(), *this));
}

Collector::Ptr Factory::newDBusMenuWindowCollector(unsigned int windowId) {
	return Collector::Ptr(
			new DBusMenuWindowCollector(windowId,
					singletonWindowStack(), singletonAppmenu(), *this));
}
