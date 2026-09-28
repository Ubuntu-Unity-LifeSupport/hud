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
#include <libhud-client/HudClient.h>
#include <common/DBusTypes.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <QSignalSpy>
#include <QTestEventLoop>
#include <gtest/gtest.h>

using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::client;
using namespace hud::common;
using namespace hud::testutils;

namespace {

class TestHudClient: public Test {

protected:
	TestHudClient() :
			mock(dbus), hud(dbus, mock) {
		dbus.startServices();
		hud.loadMethods();
	}

	void createQuery() {
		client.reset(new HudClient());
		QSignalSpy modelsChangedSpy(client.data(), SIGNAL(modelsChanged()));
		modelsChangedSpy.wait();
	}

	virtual ~TestHudClient() {
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

	void updateInterfaceProperty(const QString &name, const QString &path,
			const QString &interface, const QString &property,
			const QVariant &value) {
		QDBusInterface propertyInterface(name, path,
				"org.freedesktop.DBus.Properties", dbus.sessionConnection());
		propertyInterface.callWithArgumentList(QDBus::Block, "Set",
				QVariantList() << interface << property << value);

		OrgFreedesktopDBusMockInterface mockInterface(name, path,
				dbus.sessionConnection());
		QVariantMap propertyMap;
		propertyMap[property] = value;
		mockInterface.EmitSignal("org.freedesktop.DBus.Properties",
				"PropertiesChanged", "sa{sv}as",
				QVariantList() << interface << propertyMap << QStringList()).waitForFinished();
	}

Q_SIGNALS:
	void modelsReady();

	void queryFinished();

protected:

	DBusTestRunner dbus;

	DBusMock mock;

	MockHudService hud;

	QScopedPointer<HudClient> client;
};

TEST_F(TestHudClient, Update) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();
	client->setQuery("test2");

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "UpdateQuery", QVariantList() << "test2");
}

TEST_F(TestHudClient, Voice) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Call the voice query */
	QSignalSpy queryFinishedSpy(client.data(),
	SIGNAL(voiceQueryFinished(const QString &)));
	client->startVoiceQuery();

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "VoiceQuery", QVariantList());

	queryFinishedSpy.wait();
	ASSERT_EQ(1, queryFinishedSpy.size());
	EXPECT_EQ(QVariantList() << "voice query", queryFinishedSpy.at(0));
}

TEST_F(TestHudClient, UpdateApp) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Set App ID */
	client->setAppstackApp("application-id");

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "UpdateApp",
			QVariantList() << "application-id");
}

//TEST_F(TestHudClient, ExecuteCommand) {
//	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
//	SIGNAL(MethodCalled(const QString &, const QVariantList &)));
//
//	createQuery();
//
//	/* Execute a command */
//	client->executeCommand(0);
//
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteCommand",
//			QVariantList() << qulonglong(0) << uint(0));
//}
//
//TEST_F(TestHudClient, ExecuteParameterized) {
//	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
//	SIGNAL(MethodCalled(const QString &, const QVariantList &)));
//
//	createQuery();
//
//	/* Execute a parameterized command */
//	HudClientParam *param = hud_client_query_execute_param_command(query,
//			g_variant_new_variant(g_variant_new_uint64(4321)), 1234);
//
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteParameterized",
//			QVariantList() << qulonglong(4321) << uint(1234));
//
//	g_object_unref(param);
//}
//
//TEST_F(TestHudClient, ExecuteToolbar) {
//	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
//	SIGNAL(MethodCalled(const QString &, const QVariantList &)));
//
//	createQuery();
//
//	/* Start attacking the toolbar */
//	hud_client_query_execute_toolbar_item(query,
//			HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN, 12345);
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
//			QVariantList() << "fullscreen" << uint(1234));
//	remoteQuerySpy.clear();
//
//	hud_client_query_execute_toolbar_item(query, HUD_CLIENT_QUERY_TOOLBAR_HELP,
//			12);
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
//			QVariantList() << "help" << uint(12));
//	remoteQuerySpy.clear();
//
//	hud_client_query_execute_toolbar_item(query,
//			HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES, 312);
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
//			QVariantList() << "preferences" << uint(312));
//	remoteQuerySpy.clear();
//
//	hud_client_query_execute_toolbar_item(query, HUD_CLIENT_QUERY_TOOLBAR_UNDO,
//			53312);
//	remoteQuerySpy.wait();
//	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
//			QVariantList() << "undo" << uint(53312));
//	remoteQuerySpy.clear();
//}
//

}// namespace
