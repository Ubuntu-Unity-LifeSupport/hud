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

#ifndef HUD_SERVICE_FACTORY_H_
#define HUD_SERVICE_FACTORY_H_

#include <service/HudService.h>
#include <service/Application.h>
#include <service/ApplicationList.h>
#include <service/DBusMenuCollector.h>
#include <service/DBusMenuWindowCollector.h>
#include <service/GMenuWindowCollector.h>
#include <service/GMenuCollector.h>
#include <service/ItemStore.h>
#include <service/UsageTracker.h>
#include <service/SearchSettings.h>
#include <service/Voice.h>
#include <service/Query.h>
#include <service/Window.h>

#include <gio/gio.h>

class ComCanonicalUnityWindowStackInterface;
class ComCanonicalAppMenuRegistrarInterface;

QT_BEGIN_NAMESPACE
class QDBusServiceWatcher;
QT_END_NAMESPACE

namespace hud {
namespace service {

class Factory {
public:
	explicit Factory();

	virtual ~Factory();

	void setSessionBus(const QDBusConnection &sessionBus);

	virtual HudService::Ptr singletonHudService();

	virtual QDBusConnection sessionBus();

	virtual QSharedPointer<GDBusConnection> gSessionBus();

	virtual QSharedPointer<ComCanonicalUnityWindowStackInterface> singletonWindowStack();

	virtual QSharedPointer<QDBusServiceWatcher> windowStackWatcher();

	virtual QSharedPointer<ComCanonicalAppMenuRegistrarInterface> singletonAppmenu();

	virtual Query::Ptr newQuery(const QString &query, const QString &sender,
			Query::EmptyBehaviour emptyBehaviour);

	virtual ApplicationList::Ptr singletonApplicationList();

	virtual UsageTracker::Ptr singletonUsageTracker();

	virtual SearchSettings::Ptr singletonSearchSettings();

	virtual Voice::Ptr singletonVoice();

	virtual Application::Ptr newApplication(const QString &applicationId);

	virtual ItemStore::Ptr newItemStore(const QString &applicationId);

	virtual Window::Ptr newWindow(unsigned int windowId,
			const QString &applicationId, WindowContext::Ptr allwindowsContext);

	virtual WindowContext::Ptr newWindowContext();

	virtual WindowToken::Ptr newWindowToken(const QString &applicationId,
			QList<CollectorToken::Ptr> tokens);

	virtual Collector::Ptr newDBusMenuCollector(const QString &service,
		const QDBusObjectPath &menuObjectPath);

	virtual Collector::Ptr newGMenuCollector(const QString &name,
			const QMap<QString, QDBusObjectPath> &actions,
			const QDBusObjectPath &menuPath);

	QSharedPointer<qtgmenu::QtGMenuImporter> newQtGMenuImporter(
			const QString& service, const QDBusObjectPath& menu_path,
			const QMap<QString, QDBusObjectPath>& action_paths);

	virtual Collector::Ptr newGMenuWindowCollector(unsigned int windowId,
			const QString &applicationId);

	virtual Collector::Ptr newDBusMenuWindowCollector(unsigned int windowId);

protected:
	QDBusConnection m_sessionBus;

	unsigned int m_queryCounter;

	HudService::Ptr m_hudService;

	ApplicationList::Ptr m_applicationList;

	UsageTracker::Ptr m_usageTracker;

	SearchSettings::Ptr m_searchSettings;

	Voice::Ptr m_voice;

	QSharedPointer<ComCanonicalUnityWindowStackInterface> m_windowStack;

	QSharedPointer<ComCanonicalAppMenuRegistrarInterface> m_appmenu;
};

}
}

#endif /* HUD_SERVICE_FACTORY_H_ */
