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
#include <libhud-client/hud-client.h>
#include <common/shared-values.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <QSignalSpy>
#include <gtest/gtest.h>

using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::common;
using namespace hud::testutils;

namespace {

class TestConnection: public QObject, public Test {
Q_OBJECT

protected:
	TestConnection() :
			mock(dbus), hud(dbus, mock), query(nullptr) {
		dbus.startServices();
		hud.loadMethods();
	}

	virtual ~TestConnection() {
	}

	static void callbackConnectionCreateQuery(HudClientConnection *con,
			const gchar *query_path, const gchar *results_name,
			const gchar *appstack_name, gpointer user_data) {
		Q_UNUSED(con);

		TestConnection *self = static_cast<TestConnection*>(user_data);

		self->queryPath = QString::fromUtf8(query_path);
		self->resultsName = QString::fromUtf8(results_name);
		self->appstackName = QString::fromUtf8(appstack_name);

		self->connectionCreateQuery();
	}

Q_SIGNALS:
	void connectionCreateQuery();

protected:

	DBusTestRunner dbus;

	DBusMock mock;

	MockHudService hud;

	HudClientQuery *query;

	QString queryPath;

	QString resultsName;

	QString appstackName;
};

TEST_F(TestConnection, Create) {
	/* Create a connection */
	HudClientConnection *client_connection = hud_client_connection_new(
	DBUS_NAME, DBUS_PATH);

	ASSERT_TRUE(HUD_CLIENT_IS_CONNECTION(client_connection));

	QSignalSpy querySpy(this, SIGNAL(connectionCreateQuery()));
	hud_client_connection_new_query(client_connection, "test",
			callbackConnectionCreateQuery, this);
	querySpy.wait();

	EXPECT_FALSE(queryPath.isEmpty());
	EXPECT_FALSE(resultsName.isEmpty());
	EXPECT_FALSE(appstackName.isEmpty());

	EXPECT_STREQ(DBUS_NAME,
			hud_client_connection_get_address(client_connection));

	gchar *address = NULL;
	gchar *path = NULL;

	g_object_get(G_OBJECT(client_connection), "address", &address, "path",
			&path,
			NULL);

	EXPECT_STREQ(DBUS_NAME, address);
	EXPECT_STREQ(DBUS_PATH, path);

	g_free(address);
	g_free(path);

	g_object_unref(client_connection);
}

}

#include "TestConnection.moc"
