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

#ifndef HUD_SERVICE_TEST_MOCKS_H_
#define HUD_SERVICE_TEST_MOCKS_H_

#include <service/Factory.h>
#include <service/ApplicationList.h>

#include <gmock/gmock.h>

namespace hud {
namespace service {
namespace test {

class MockFactory: public Factory {
public:
	MOCK_METHOD0(singletonApplicationList, ApplicationList::Ptr());

	MOCK_METHOD0(sessionBus, QDBusConnection());

	MOCK_METHOD3(newQuery, Query::Ptr( const QString &, const QString &, Query::EmptyBehaviour));

	MOCK_METHOD1(newApplication, Application::Ptr(const QString &));

	MOCK_METHOD0(newWindowContext, WindowContext::Ptr());

	MOCK_METHOD3(newWindow, Window::Ptr(unsigned int, const QString &, WindowContext::Ptr));

	MOCK_METHOD0(singletonUsageTracker, UsageTracker::Ptr());

	MOCK_METHOD1(newDBusMenuWindowCollector, Collector::Ptr(unsigned int));

	MOCK_METHOD2(newGMenuWindowCollector, Collector::Ptr(unsigned int, const QString &));

	MOCK_METHOD3(newGMenuCollector, Collector::Ptr(const QString &,
					const QMap<QString, QDBusObjectPath> &,
					const QDBusObjectPath &));
};

class MockHudService: public HudService {
public:
	MOCK_METHOD1(closeQuery, Query::Ptr(const QDBusObjectPath &));
};

class MockQuery: public Query {
public:
	MOCK_CONST_METHOD0(appstackModel, QString());

	MOCK_CONST_METHOD0(currentQuery, QString());

	MOCK_CONST_METHOD0(resultsModel, QString());

	MOCK_CONST_METHOD0(toolbarItems, QStringList());

	MOCK_CONST_METHOD0(path, const QDBusObjectPath &());

	MOCK_CONST_METHOD0(results, const QList<Result> &());

	MOCK_METHOD1(UpdateQuery, int(const QString &));

	MOCK_METHOD2(ExecuteCommand, void(const QDBusVariant &, uint));
};

class MockApplicationList: public ApplicationList {
public:
	MOCK_CONST_METHOD0(applications, QList<hud::common::NameObject>());

	MOCK_CONST_METHOD0(focusedApplication, Application::Ptr());

	MOCK_CONST_METHOD0(focusedWindow, Window::Ptr());

	MOCK_METHOD1(ensureApplication, Application::Ptr(const QString &));
};

class MockApplication: public Application {
public:
	MOCK_CONST_METHOD0(id, const QString &());

	MOCK_METHOD0(icon, const QString &());

	MOCK_METHOD1(addWindow, void(unsigned int));

	MOCK_METHOD1(removeWindow, void(unsigned int));

	MOCK_METHOD1(window, Window::Ptr(unsigned int));

	MOCK_CONST_METHOD0(isEmpty, bool());

	MOCK_CONST_METHOD0(path, const QDBusObjectPath &());
};

class MockWindowToken: public WindowToken {
public:
	MOCK_METHOD3(search, void(const QString &,
					Query::EmptyBehaviour emptyBehaviour, QList<Result> &));

	MOCK_METHOD1(execute, void(unsigned long long));

	MOCK_METHOD1(executeToolbar, void(const QString &));

	MOCK_METHOD5(executeParameterized, QString(unsigned long long,
					QString &, QString &, QDBusObjectPath &, QDBusObjectPath &));

	MOCK_CONST_METHOD0(commands, QList<QStringList>());

	MOCK_CONST_METHOD0(toolbarItems, QStringList());

	MOCK_CONST_METHOD0(tokens, const QList<CollectorToken::Ptr> &());
};

class MockWindowContext: public WindowContext {
public:
	MOCK_METHOD1(setContext, void(const QString &));

	MOCK_METHOD2(addMenu, void(const QString &, const MenuDefinition &));

	MOCK_METHOD0(activeCollector, Collector::Ptr());
};

class MockWindow: public Window {
public:
	MOCK_METHOD0(activate, WindowToken::Ptr());

	MOCK_METHOD1(setContext, void(const QString &));

	MOCK_METHOD2(search, void(const QString &, QList<Result> &));

	MOCK_METHOD2(addMenu, void(const QString &, const MenuDefinition &));

	MOCK_METHOD0(activeCollector, Collector::Ptr());
};

class MockUsageTracker: public UsageTracker {
public:
	MOCK_METHOD2(markUsage, void(const QString &,
					const QString &));

	MOCK_CONST_METHOD2(usage, unsigned int(const QString &,
					const QString &));
};

class MockVoice: public Voice {
public:
	MOCK_METHOD1(listen, QString(const QList<QStringList> &));
};

class MockCollector: public Collector {
public:
	MOCK_CONST_METHOD0(isValid, bool());

	MOCK_METHOD0(activate, QList<CollectorToken::Ptr>());

	MOCK_METHOD2(search, void(const QString &, QList<Result> &));

protected:
	MOCK_METHOD0(deactivate, void());
};

}
}
}

#endif /* HUD_SERVICE_TEST_MOCKS_H_ */
