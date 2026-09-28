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

#include <testutils/MockHudService.h>
#include <testutils/RawDBusTransformer.h>
#include <libhud/hud.h>
#include <common/shared-values.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <QSignalSpy>
#include <QTestEventLoop>
#include <gtest/gtest.h>

using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::common;
using namespace hud::testutils;

namespace {

class TestManager: public Test {
protected:
	TestManager() :
			mock(dbus), hud(dbus, mock) {
		dbus.startServices();
		hud.loadMethods();

		connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	}

	virtual ~TestManager() {
		g_object_unref(connection);
	}

	void EXPECT_CALL(const QList<QVariantList> &spy, int index,
			const QString &name, const QVariantList &args) {
		QVariant args2(QVariant::fromValue(args));
		ASSERT_LT(index, spy.size());
		const QVariantList &call(spy.at(index));
		EXPECT_EQ(name, call.at(0).toString());
		EXPECT_EQ(args2.toString().toStdString(),
				call.at(1).toString().toStdString());
	}

	DBusTestRunner dbus;

	DBusMock mock;

	MockHudService hud;

	GDBusConnection *connection;
};

TEST_F(TestManager, Create) {
	QSignalSpy hudSpy(&hud.hudInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	HudManager *manager = hud_manager_new("test.app");
	hudSpy.wait();

	EXPECT_CALL(hudSpy, 0, "RegisterApplication",
			QVariantList() << QString("test.app"));

	// FIXME Waiting in tests - need to wait for reply to get to the manager
	QTestEventLoop::instance().enterLoopMSecs(100);
	g_object_unref(manager);
}

TEST_F(TestManager, CreateWithApplication) {
	GApplication *application = g_application_new("app.id",
			G_APPLICATION_FLAGS_NONE);
	ASSERT_TRUE(g_application_register(application, NULL, NULL));

	QSignalSpy hudSpy(&hud.hudInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	QSignalSpy applicationSpy(&hud.applicationInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	HudManager *manager = hud_manager_new_for_application(application);

	hudSpy.wait();
	applicationSpy.wait();

	g_object_unref(manager);

	EXPECT_CALL(hudSpy, 0, "RegisterApplication",
			QVariantList() << QString("app.id"));

	QVariantList &args(applicationSpy[0]);
	RawDBusTransformer::transform(args);

	QVariantList expectedArgs;
	QVariantList args1;
	args1 << uint(0) << "action-publisher-context-0" << "app"
			<< QVariant::fromValue(QDBusObjectPath("/app/id"));
	expectedArgs << QVariant::fromValue(args1);
	QVariantList args2;
	args2 << uint(0) << "action-publisher-context-0"
			<< QVariant::fromValue(
					QDBusObjectPath("/com/canonical/hud/publisher"));
	expectedArgs << QVariant::fromValue(args2);

	EXPECT_CALL(applicationSpy, 0, "AddSources", expectedArgs);

	g_object_unref(application);
}

TEST_F(TestManager, AddActions) {
	QSignalSpy hudSpy(&hud.hudInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	QSignalSpy applicationSpy(&hud.applicationInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	HudManager *manager = hud_manager_new("test.app");

	hudSpy.wait();

	HudActionPublisher *publisher = hud_action_publisher_new(
	HUD_ACTION_PUBLISHER_ALL_WINDOWS, "test-add-context");
	hud_action_publisher_add_action_group(publisher, "app", "/app/object");
	hud_manager_add_actions(manager, publisher);

	applicationSpy.wait();

	QVariantList &args(applicationSpy[0]);
	RawDBusTransformer::transform(args);

	QVariantList expectedArgs;
	QVariantList args1;
	args1 << uint(0) << "test-add-context" << "app"
			<< QVariant::fromValue(QDBusObjectPath("/app/object"));
	expectedArgs << QVariant::fromValue(args1);
	QVariantList args2;
	args2 << uint(0) << "test-add-context"
			<< QVariant::fromValue(
					QDBusObjectPath("/com/canonical/hud/publisher1"));
	expectedArgs << QVariant::fromValue(args2);

	EXPECT_CALL(applicationSpy, 0, "AddSources", expectedArgs);

	g_object_unref(publisher);
	g_object_unref(manager);
}

/**
 * the remove_actions method does nothing at the moment
 */
TEST_F(TestManager, RemoveActions) {
	QSignalSpy hudSpy(&hud.hudInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	QSignalSpy applicationSpy(&hud.applicationInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	HudManager *manager = hud_manager_new("app-id");
	hudSpy.wait();

	HudActionPublisher *publisher = hud_action_publisher_new(
	HUD_ACTION_PUBLISHER_ALL_WINDOWS, "test-context");
	hud_action_publisher_add_action_group(publisher, "app", "/app/object");
	hud_manager_add_actions(manager, publisher);
	applicationSpy.wait();

	QVariantList &args(applicationSpy[0]);
	RawDBusTransformer::transform(args);

	QVariantList expectedArgs;
	QVariantList args1;
	args1 << uint(0) << "test-context" << "app"
			<< QVariant::fromValue(QDBusObjectPath("/app/object"));
	expectedArgs << QVariant::fromValue(args1);
	QVariantList args2;
	args2 << uint(0) << "test-context"
			<< QVariant::fromValue(
					QDBusObjectPath("/com/canonical/hud/publisher2"));
	expectedArgs << QVariant::fromValue(args2);

	EXPECT_CALL(applicationSpy, 0, "AddSources", expectedArgs);

	hud_manager_remove_actions(manager, publisher);

	// FIXME Waiting in tests
	QTestEventLoop::instance().enterLoopMSecs(100);

	g_object_unref(publisher);
	g_object_unref(manager);
}

}
