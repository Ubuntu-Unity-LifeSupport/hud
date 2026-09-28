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
#include <libhud-client/hud-client.h>
#include <common/DBusTypes.h>
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

class TestQuery: public QObject, public Test {
Q_OBJECT

protected:
	TestQuery() :
			mock(dbus), hud(dbus, mock) {
		dbus.startServices();
		hud.loadMethods();
	}

	void createQuery() {
		QSignalSpy querySpy(this, SIGNAL(modelsReady()));

		/* Create a query */
		query.reset(hud_client_query_new("test"), &g_object_unref);

		/* Wait for the models to be ready */
		g_signal_connect(G_OBJECT(query.data()),
				HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED,
				G_CALLBACK(callbackModelsReady), this);
		querySpy.wait();

		/* Check the models */
		ASSERT_TRUE(DEE_IS_MODEL(hud_client_query_get_results_model(query.data())));
		ASSERT_TRUE(DEE_IS_MODEL(hud_client_query_get_appstack_model(query.data())));
	}

	void createQuery(HudClientConnection *client_connection) {
		QSignalSpy querySpy(this, SIGNAL(modelsReady()));

		/* Create a query */
		query.reset( hud_client_query_new_for_connection("test", client_connection), &g_object_unref);

		/* Wait for the models to be ready */
		g_signal_connect(G_OBJECT(query.data()),
				HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED,
				G_CALLBACK(callbackModelsReady), this);
		querySpy.wait();

		/* Check the models */
		ASSERT_TRUE(DEE_IS_MODEL(hud_client_query_get_results_model(query.data())));
		ASSERT_TRUE(DEE_IS_MODEL(hud_client_query_get_appstack_model(query.data())));
	}

	virtual ~TestQuery() {
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

	static void callbackModelsReady(HudClientQuery *query, gpointer user_data) {
		Q_UNUSED(query);
		TestQuery *self = static_cast<TestQuery*>(user_data);
		self->modelsReady();
	}

	static void callbackVoiceQueryFinished(HudClientQuery *query,
			GDBusMethodInvocation *invocation, gpointer user_data) {
		Q_UNUSED(query);
		Q_UNUSED(invocation);
		TestQuery *self = static_cast<TestQuery*>(user_data);
		self->queryFinished();
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

	QSharedPointer<HudClientQuery> query;
};

TEST_F(TestQuery, Create) {
	createQuery();

	EXPECT_STREQ("test", hud_client_query_get_query(query.data()));

	HudClientConnection * client_connection = NULL;
	gchar * search = NULL;

	g_object_get(G_OBJECT(query.data()), "query", &search, "connection",
			&client_connection, NULL);

	EXPECT_STREQ("test", search);
	ASSERT_TRUE(HUD_CLIENT_IS_CONNECTION(client_connection));

	g_free(search);

	g_object_unref(client_connection);
}

TEST_F(TestQuery, Custom) {
	/* Create a connection */
	HudClientConnection *clientConnection = hud_client_connection_new(
	DBUS_NAME, DBUS_PATH);
	ASSERT_TRUE(HUD_CLIENT_IS_CONNECTION(clientConnection));

	createQuery(clientConnection);

	EXPECT_STREQ("test", hud_client_query_get_query(query.data()));

	/* Make sure the connection is the same */
	HudClientConnection *testcon = NULL;

	g_object_get(G_OBJECT(query.data()), "connection", &testcon, NULL);

	ASSERT_TRUE(HUD_CLIENT_IS_CONNECTION(testcon));
	ASSERT_EQ(testcon, clientConnection);
	g_object_unref(testcon);

	/* Clean up */
	g_object_unref(clientConnection);
}

TEST_F(TestQuery, Update) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	hud_client_query_set_query(query.data(), "test2");
	EXPECT_STREQ("test2", hud_client_query_get_query(query.data()));

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "UpdateQuery", QVariantList() << "test2");
}

TEST_F(TestQuery, Voice) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Call the voice query */
	g_signal_connect(G_OBJECT(query.data()), "voice-query-finished",
			G_CALLBACK(callbackVoiceQueryFinished), this);

	QSignalSpy queryFinishedSpy(this, SIGNAL(queryFinished()));

	hud_client_query_voice_query(query.data());

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "VoiceQuery", QVariantList());

	queryFinishedSpy.wait();
	EXPECT_EQ(1, queryFinishedSpy.size());
}

TEST_F(TestQuery, UpdateApp) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Set App ID */
	hud_client_query_set_appstack_app(query.data(), "application-id");

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "UpdateApp",
			QVariantList() << "application-id");
}

TEST_F(TestQuery, ExecuteCommand) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Execute a command */
	hud_client_query_execute_command(query.data(),
			g_variant_new_variant(g_variant_new_uint64(4321)), 1234);

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteCommand",
			QVariantList() << qulonglong(4321) << uint(1234));
}

TEST_F(TestQuery, ExecuteParameterized) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Execute a parameterized command */
	HudClientParam *param = hud_client_query_execute_param_command(query.data(),
			g_variant_new_variant(g_variant_new_uint64(4321)), 1234);

	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteParameterized",
			QVariantList() << qulonglong(4321) << uint(1234));

	g_object_unref(param);
}

TEST_F(TestQuery, ExecuteToolbar) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Start attacking the toolbar */
	hud_client_query_execute_toolbar_item(query.data(),
			HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN, 12345);
	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
			QVariantList() << "fullscreen" << uint(1234));
	remoteQuerySpy.clear();

	hud_client_query_execute_toolbar_item(query.data(), HUD_CLIENT_QUERY_TOOLBAR_HELP,
			12);
	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
			QVariantList() << "help" << uint(12));
	remoteQuerySpy.clear();

	hud_client_query_execute_toolbar_item(query.data(),
			HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES, 312);
	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
			QVariantList() << "preferences" << uint(312));
	remoteQuerySpy.clear();

	hud_client_query_execute_toolbar_item(query.data(), HUD_CLIENT_QUERY_TOOLBAR_UNDO,
			53312);
	remoteQuerySpy.wait();
	EXPECT_CALL(remoteQuerySpy, 0, "ExecuteToolbar",
			QVariantList() << "undo" << uint(53312));
	remoteQuerySpy.clear();
}

TEST_F(TestQuery, ToolbarEnabled) {
	QSignalSpy remoteQuerySpy(&hud.queryInterface(),
	SIGNAL(MethodCalled(const QString &, const QVariantList &)));

	createQuery();

	/* Test toolbar disabled */
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_HELP));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_UNDO));

	/* Set an 'undo' item */
	updateInterfaceProperty(DBusTypes::HUD_SERVICE_DBUS_NAME,
			MockHudService::QUERY_PATH, "com.canonical.hud.query",
			"ToolbarItems", QStringList() << "undo");
	QTestEventLoop::instance().enterLoopMSecs(100);

	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_HELP));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES));
	EXPECT_TRUE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_UNDO));

	/* Set an 'invalid' item */
	updateInterfaceProperty(DBusTypes::HUD_SERVICE_DBUS_NAME,
			MockHudService::QUERY_PATH, "com.canonical.hud.query",
			"ToolbarItems", QStringList() << "invalid");
	QTestEventLoop::instance().enterLoopMSecs(100);

	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_HELP));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES));
	EXPECT_FALSE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_UNDO));

	/* Set all items */
	updateInterfaceProperty(DBusTypes::HUD_SERVICE_DBUS_NAME,
			MockHudService::QUERY_PATH, "com.canonical.hud.query",
			"ToolbarItems",
			QStringList() << "fullscreen" << "undo" << "help" << "preferences");
	QTestEventLoop::instance().enterLoopMSecs(100);

	EXPECT_TRUE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN));
	EXPECT_TRUE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_HELP));
	EXPECT_TRUE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES));
	EXPECT_TRUE(
			hud_client_query_toolbar_item_active(query.data(),
					HUD_CLIENT_QUERY_TOOLBAR_UNDO));

	/* Check the array */
	QSharedPointer<GArray> toolbar(hud_client_query_get_active_toolbar(query.data()), &g_array_unref);
	ASSERT_EQ(4, toolbar->len);

	bool found_undo = false;
	bool found_help = false;
	bool found_prefs = false;
	bool found_full = false;

	for (uint i = 0; i < toolbar->len; ++i) {
		switch (g_array_index(toolbar.data(), int, i)) {
		case HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN:
			EXPECT_FALSE(found_full);
			found_full = true;
			break;
		case HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES:
			EXPECT_FALSE(found_prefs);
			found_prefs = true;
			break;
		case HUD_CLIENT_QUERY_TOOLBAR_UNDO:
			EXPECT_FALSE(found_undo);
			found_undo = true;
			break;
		case HUD_CLIENT_QUERY_TOOLBAR_HELP:
			EXPECT_FALSE(found_help);
			found_help = true;
			break;
		default:
			EXPECT_FALSE(true);
		}
	}

	EXPECT_TRUE(found_undo);
	EXPECT_TRUE(found_help);
	EXPECT_TRUE(found_prefs);
	EXPECT_TRUE(found_full);
}

}

#include "TestQuery.moc"
