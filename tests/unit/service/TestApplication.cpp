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

#include <common/ApplicationInterface.h>
#include <service/Factory.h>
#include <service/ApplicationImpl.h>
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

class TestApplication: public Test {
protected:
	TestApplication() :
			mock(dbus) {
		factory.setSessionBus(dbus.sessionConnection());

		allWindowsContext.reset(new NiceMock<MockWindowContext>());

		EXPECT_CALL(factory, newWindowContext()).WillOnce(
				Return(allWindowsContext));
	}

	virtual ~TestApplication() {
	}

	static void addAction(QList<hud::common::Action> &actions,
			unsigned int windowId, const QString &context,
			const QString &prefix, const QDBusObjectPath &object) {
		hud::common::Action action;
		action.m_windowId = windowId;
		action.m_context = context;
		action.m_prefix = prefix;
		action.m_object = object;
		actions << action;
	}

	static void addMenu(QList<hud::common::Description> &descriptions,
			unsigned int windowId, const QString &context,
			const QDBusObjectPath &object) {
		hud::common::Description description;
		description.m_windowId = windowId;
		description.m_context = context;
		description.m_object = object;
		descriptions << description;
	}

	DBusTestRunner dbus;

	DBusMock mock;

	NiceMock<MockFactory> factory;

	QSharedPointer<MockWindowContext> allWindowsContext;
};

TEST_F(TestApplication, DBusInterfaceIsExported) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	ComCanonicalHudApplicationInterface applicationInterface(
			dbus.sessionConnection().baseService(),
			DBusTypes::applicationPath("application-id"),
			dbus.sessionConnection());

	ASSERT_TRUE(applicationInterface.isValid());

	//FIXME desktop path will return something when it's actually implemented
	EXPECT_EQ(QString(), applicationInterface.desktopPath());
}

TEST_F(TestApplication, AddsWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());
	EXPECT_TRUE(application.isEmpty());

	QSharedPointer<MockWindow> window(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(4567, QString("application-id"), _)).WillOnce(
			Return(window));
	application.addWindow(4567);
	EXPECT_FALSE(application.isEmpty());
}

TEST_F(TestApplication, HandlesDeleteUnknownWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());
	EXPECT_TRUE(application.isEmpty());

	QSharedPointer<MockWindow> window(new NiceMock<MockWindow>());
	application.removeWindow(4567);
	EXPECT_TRUE(application.isEmpty());
}

TEST_F(TestApplication, DeletesWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QSharedPointer<MockWindow> window0(new NiceMock<MockWindow>());
	QSharedPointer<MockWindow> window1(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(0, QString("application-id"), _)).WillOnce(
			Return(window0));
	application.addWindow(0);
	EXPECT_FALSE(application.isEmpty());

	EXPECT_CALL(factory, newWindow(1, QString("application-id"), _)).WillOnce(
			Return(window1));
	application.addWindow(1);
	EXPECT_FALSE(application.isEmpty());

	application.removeWindow(0);
	EXPECT_FALSE(application.isEmpty());

	application.removeWindow(1);
	EXPECT_TRUE(application.isEmpty());
}

TEST_F(TestApplication, AddSourcesToAllWindowsContext) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QList<hud::common::Action> actions;
	addAction(actions, 0, "context1", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 0, "context2", "prefix", QDBusObjectPath("/actions1"));

	QList<Description> descriptions;
	addMenu(descriptions, 0, "context1", QDBusObjectPath("/menu1"));
	addMenu(descriptions, 0, "context2", QDBusObjectPath("/menu2"));

	WindowContext::MenuDefinition menuDefinition1("local");
	menuDefinition1.actionPath = QDBusObjectPath("/actions1");
	menuDefinition1.actionPrefix = "prefix";
	menuDefinition1.menuPath = QDBusObjectPath("/menu1");

	WindowContext::MenuDefinition menuDefinition2("local");
	menuDefinition2.actionPath = QDBusObjectPath("/actions1");
	menuDefinition2.actionPrefix = "prefix";
	menuDefinition2.menuPath = QDBusObjectPath("/menu2");

	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context1"), menuDefinition1));
	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context2"), menuDefinition2));

	application.AddSources(actions, descriptions);
}

TEST_F(TestApplication, AddSourcesToAllWindowsContextAndWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QSharedPointer<MockWindow> window1(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(1, QString("application-id"), _)).WillOnce(
			Return(window1));
	application.addWindow(1);

	QList<hud::common::Action> actions;
	addAction(actions, 0, "context1", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 0, "context2", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 1, "context1", "prefix", QDBusObjectPath("/actions2"));

	QList<Description> descriptions;
	addMenu(descriptions, 0, "context1", QDBusObjectPath("/menu1"));
	addMenu(descriptions, 0, "context2", QDBusObjectPath("/menu2"));
	addMenu(descriptions, 1, "context1", QDBusObjectPath("/menu3"));

	WindowContext::MenuDefinition menuDefinition1("local");
	menuDefinition1.actionPath = QDBusObjectPath("/actions1");
	menuDefinition1.actionPrefix = "prefix";
	menuDefinition1.menuPath = QDBusObjectPath("/menu1");

	WindowContext::MenuDefinition menuDefinition2("local");
	menuDefinition2.actionPath = QDBusObjectPath("/actions1");
	menuDefinition2.actionPrefix = "prefix";
	menuDefinition2.menuPath = QDBusObjectPath("/menu2");

	WindowContext::MenuDefinition menuDefinition3("local");
	menuDefinition3.actionPath = QDBusObjectPath("/actions2");
	menuDefinition3.actionPrefix = "prefix";
	menuDefinition3.menuPath = QDBusObjectPath("/menu3");

	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context1"), menuDefinition1));
	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context2"), menuDefinition2));
	EXPECT_CALL(*window1, addMenu(QString("context1"), menuDefinition3));

	application.AddSources(actions, descriptions);
}

} // namespace
