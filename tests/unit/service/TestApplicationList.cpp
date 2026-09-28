/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <common/DBusTypes.h>
#include <service/Factory.h>
#include <service/ApplicationListImpl.h>
#include <unit/service/Mocks.h>

#include <QDebug>
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::common;
using namespace hud::service;
using namespace hud::service::test;

namespace {

class TestApplicationList: public Test {
protected:
	TestApplicationList() :
			mock(dbus) {

		factory.setSessionBus(dbus.sessionConnection());

		mock.registerCustomMock(DBusTypes::WINDOW_STACK_DBUS_NAME,
				DBusTypes::WINDOW_STACK_DBUS_PATH,
				ComCanonicalUnityWindowStackInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);

		dbus.startServices();

		windowStack.reset(
				new ComCanonicalUnityWindowStackInterface(
						DBusTypes::WINDOW_STACK_DBUS_NAME,
						DBusTypes::WINDOW_STACK_DBUS_PATH,
						dbus.sessionConnection()));

		windowStackWatcher.reset(
				new QDBusServiceWatcher(DBusTypes::WINDOW_STACK_DBUS_NAME,
						dbus.sessionConnection(),
						QDBusServiceWatcher::WatchForUnregistration));

	}

	virtual ~TestApplicationList() {
	}

	virtual OrgFreedesktopDBusMockInterface & windowStackMock() {
		return mock.mockInterface(DBusTypes::WINDOW_STACK_DBUS_NAME,
				DBusTypes::WINDOW_STACK_DBUS_PATH,
				ComCanonicalUnityWindowStackInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);
	}

	DBusTestRunner dbus;

	DBusMock mock;

	NiceMock<MockFactory> factory;

	QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack;

	QSharedPointer<QDBusServiceWatcher> windowStackWatcher;
};

TEST_F(TestApplicationList, CreatesSingleApplicationOnStartup) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	QSharedPointer<MockApplication> application(
			new NiceMock<MockApplication>());
	QDBusObjectPath path(QDBusObjectPath("/path/app/0"));
	ON_CALL(*application, path()).WillByDefault(ReturnRef(path));
	EXPECT_CALL(*application, addWindow(0));

	EXPECT_CALL(factory, newApplication(QString("app0"))).WillOnce(
			Return(application));

	ApplicationListImpl applicationList(factory, windowStack,
				windowStackWatcher);

	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path), applicationList.applications().at(0));
}

TEST_F(TestApplicationList, CreatesSingleApplicationWithMultipleWindowsOnStartup) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)",
			"ret = [(0, 'app0', True, 0), (1, 'app0', False, 0)]").waitForFinished();

	QSharedPointer<MockApplication> application(
			new NiceMock<MockApplication>());
	QDBusObjectPath path(QDBusObjectPath("/path/app/0"));
	ON_CALL(*application, path()).WillByDefault(ReturnRef(path));
	EXPECT_CALL(*application, addWindow(0));
	EXPECT_CALL(*application, addWindow(1));

	EXPECT_CALL(factory, newApplication(QString("app0"))).WillOnce(
			Return(application));

	ApplicationListImpl applicationList(factory, windowStack,
				windowStackWatcher);

	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path), applicationList.applications().at(0));
}

TEST_F(TestApplicationList, CreatesMultipleApplicationOnStartup) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)",
			"ret = [(123, 'app0', True, 0), (456, 'app1', False, 0)]").waitForFinished();

	QSharedPointer<MockApplication> application0(
			new NiceMock<MockApplication>());
	QDBusObjectPath path0(QDBusObjectPath("/path/app/0"));
	ON_CALL(*application0, path()).WillByDefault(ReturnRef(path0));
	EXPECT_CALL(*application0, addWindow(123));

	QSharedPointer<MockApplication> application1(
			new NiceMock<MockApplication>());
	QDBusObjectPath path1(QDBusObjectPath("/path/app/1"));
	ON_CALL(*application1, path()).WillByDefault(ReturnRef(path1));
	EXPECT_CALL(*application1, addWindow(456));

	EXPECT_CALL(factory, newApplication(QString("app0"))).WillOnce(
			Return(application0));
	EXPECT_CALL(factory, newApplication(QString("app1"))).WillOnce(
			Return(application1));

	ApplicationListImpl applicationList(factory, windowStack,
				windowStackWatcher);

	QList<NameObject> applications = applicationList.applications();
	ASSERT_EQ(2, applications.size());
	EXPECT_EQ(NameObject("app0", path0), applications.at(0));
	EXPECT_EQ(NameObject("app1", path1), applications.at(1));
}

TEST_F(TestApplicationList, RemovesApplicationWhenAllWindowsClosed) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)",
			"ret = [(0, 'app0', True, 0), (1, 'app0', False, 0)]").waitForFinished();

	QSharedPointer<MockApplication> application(
			new NiceMock<MockApplication>());
	QDBusObjectPath path(QDBusObjectPath("/path/app/0"));
	ON_CALL(*application, path()).WillByDefault(ReturnRef(path));
	ON_CALL(*application, isEmpty()).WillByDefault(Return(false));

	EXPECT_CALL(factory, newApplication(QString("app0"))).WillOnce(
			Return(application));

	EXPECT_CALL(*application, addWindow(0));
	EXPECT_CALL(*application, addWindow(1));
	ApplicationListImpl applicationList(factory, windowStack,
				windowStackWatcher);

	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path), applicationList.applications().at(0));

	EXPECT_CALL(*application, removeWindow(1));
	applicationList.WindowDestroyed(1, "app0");
	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path), applicationList.applications().at(0));

	ON_CALL(*application, isEmpty()).WillByDefault(Return(true));
	EXPECT_CALL(*application, removeWindow(0));
	applicationList.WindowDestroyed(0, "app0");
	ASSERT_TRUE(applicationList.applications().isEmpty());
}

TEST_F(TestApplicationList, StartsEmptyThenAddsAndRemovesApplications) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = []").waitForFinished();

	QSharedPointer<MockApplication> application0(
			new NiceMock<MockApplication>());
	QDBusObjectPath path0(QDBusObjectPath("/path/app/0"));
	ON_CALL(*application0, path()).WillByDefault(ReturnRef(path0));
	ON_CALL(*application0, isEmpty()).WillByDefault(Return(false));

	QSharedPointer<MockApplication> application1(
			new NiceMock<MockApplication>());
	QDBusObjectPath path1(QDBusObjectPath("/path/app/1"));
	ON_CALL(*application1, path()).WillByDefault(ReturnRef(path1));
	ON_CALL(*application1, isEmpty()).WillByDefault(Return(false));

	ApplicationListImpl applicationList(factory, windowStack,
				windowStackWatcher);
	ASSERT_TRUE(applicationList.applications().isEmpty());

	EXPECT_CALL(factory, newApplication(QString("app0"))).WillOnce(
			Return(application0));
	EXPECT_CALL(*application0, addWindow(123));
	applicationList.WindowCreated(123, "app0");
	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path0), applicationList.applications().at(0));

	EXPECT_CALL(factory, newApplication(QString("app1"))).WillOnce(
			Return(application1));
	EXPECT_CALL(*application1, addWindow(456));
	applicationList.WindowCreated(456, "app1");
	ASSERT_EQ(2, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path0), applicationList.applications().at(0));
	EXPECT_EQ(NameObject("app1", path1), applicationList.applications().at(1));

	ON_CALL(*application1, isEmpty()).WillByDefault(Return(true));
	EXPECT_CALL(*application1, removeWindow(456));
	applicationList.WindowDestroyed(456, "app1");
	ASSERT_EQ(1, applicationList.applications().size());
	EXPECT_EQ(NameObject("app0", path0), applicationList.applications().at(0));

	ON_CALL(*application0, isEmpty()).WillByDefault(Return(true));
	EXPECT_CALL(*application0, removeWindow(123));
	applicationList.WindowDestroyed(123, "app0");
	ASSERT_TRUE(applicationList.applications().isEmpty());
}

} // namespace
