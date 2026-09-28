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

#include <libhud-client/HudClient.h>
#include <common/DBusTypes.h>
#include <common/WindowStackInterface.h>
#include <tests/testutils/HudTestInterface.h>

#include <QAction>
#include <QDebug>
#include <QDBusConnection>
#include <QMenu>
#include <QString>
#include <QSignalSpy>
#include <QTestEventLoop>
#include <QAbstractListModel>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace hud::common;
using namespace hud::client;
using namespace std;
using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;

namespace {

class TestHud: public Test {
protected:
	typedef QPair<QString, QString> ResultPair;

	TestHud() :
			mock(dbus) {

		mock.registerCustomMock(DBusTypes::BAMF_DBUS_NAME,
				DBusTypes::BAMF_MATCHER_DBUS_PATH, "org.ayatana.bamf.control",
				QDBusConnection::SessionBus);

		mock.registerCustomMock(DBusTypes::WINDOW_STACK_DBUS_NAME,
				DBusTypes::WINDOW_STACK_DBUS_PATH,
				ComCanonicalUnityWindowStackInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);

		mock.registerCustomMock(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
				DBusTypes::APPMENU_REGISTRAR_DBUS_PATH,
				"com.canonical.AppMenu.Registrar", QDBusConnection::SessionBus);

		dbus.startServices();
	}

	virtual ~TestHud() {
	}

	static void EXPECT_RESULT(const QList<QVariantList> &results, int index,
			const QString &name, const QVariant &value) {
		const QVariantList &result(results.at(index));
		EXPECT_EQ(name, result.at(0).toString());
		EXPECT_EQ(value, result.at(1).value<QDBusVariant>().variant());
	}

	OrgFreedesktopDBusMockInterface & bamfMatcherMock() {
		return mock.mockInterface(DBusTypes::BAMF_DBUS_NAME,
				DBusTypes::BAMF_MATCHER_DBUS_PATH, "org.ayatana.bamf.control",
				QDBusConnection::SessionBus);
	}

	OrgFreedesktopDBusMockInterface & windowStackMock() {
		return mock.mockInterface(DBusTypes::WINDOW_STACK_DBUS_NAME,
				DBusTypes::WINDOW_STACK_DBUS_PATH,
				ComCanonicalUnityWindowStackInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);
	}

	OrgFreedesktopDBusMockInterface & appmenuRegstrarMock() {
		return mock.mockInterface(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
				DBusTypes::APPMENU_REGISTRAR_DBUS_PATH,
				"com.canonical.AppMenu.Registrar", QDBusConnection::SessionBus);
	}

	void startHud() {
		hud.reset(
				new QProcessDBusService(DBusTypes::HUD_SERVICE_DBUS_NAME,
						QDBusConnection::SessionBus,
						HUD_SERVICE_BINARY, QStringList()));
		hud->start(dbus.sessionConnection());
	}

	void startDBusMenu(const QString &name, const QString &path,
			const QString &json) {
		menuService.reset(
				new QProcessDBusService(name, QDBusConnection::SessionBus,
				DBUSMENU_JSON_LOADER, QStringList() << name << path << json));
		menuService->start(dbus.sessionConnection());
	}

	void startGMenu(const QString &name, const QString &path,
			const QString &model) {
		menuService.reset(
				new QProcessDBusService(name, QDBusConnection::SessionBus,
						model, QStringList() << name << path));
		menuService->start(dbus.sessionConnection());
	}

	void startLibHud(const QString &applicationId, const QString &name,
			const QString &path, const QString &model) {
		menuService.reset(
				new QProcessDBusService(name, QDBusConnection::SessionBus,
						model, QStringList() << applicationId << name << path));
		menuService->start(dbus.sessionConnection());
	}

	QDBusConnection connection() {
		return dbus.sessionConnection();
	}

	static ResultPair result(const QAbstractListModel &results, uint i) {
		QModelIndex index = results.index(i);
		return ResultPair(results.data(index, 1).toString(),
				results.data(index, 3).toString());
	}

	void EXPECT_TOOLBAR(const QAbstractListModel &toolbarModel, int i,
			const QString &icon, HudClientQueryToolbarItems item,
			bool enabled) {
		QModelIndex index = toolbarModel.index(i);
		EXPECT_EQ(icon,
				toolbarModel.data(index, Qt::DecorationRole).toString());
		EXPECT_EQ(item, toolbarModel.data(index, Qt::UserRole).toInt());
		if ((toolbarModel.data(index, Qt::UserRole + 1).toBool()) != enabled) {
			QSignalSpy spy(&toolbarModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
			ASSERT_TRUE(spy.wait());
		}
		EXPECT_EQ(enabled, toolbarModel.data(index, Qt::UserRole + 1).toBool());
	}

	DBusTestRunner dbus;

	DBusMock mock;

	QSharedPointer<QProcessDBusService> hud;

	QSharedPointer<QProcessDBusService> menuService;
};

TEST_F(TestHud, SearchDBusMenuContext) {
	startDBusMenu("menu.name", "/menu", JSON_SOURCE);

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	// There are no GMenus in this test
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"for arg in args[2]:\n"
					"  ret.append('')").waitForFinished();

	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('menu.name', '/menu')").waitForFinished();

	startHud();

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));
	client.setQuery("piece hook");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() != 3) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(3, results.rowCount());
	EXPECT_EQ(ResultPair("stray slash", "piece hook"), result(results, 0));
	EXPECT_EQ(ResultPair("swift sad", "piece hook"), result(results, 1));
	EXPECT_EQ(ResultPair("bowl", "link"), result(results, 2));
}

TEST_F(TestHud, SearchDBusMenuOneResult) {
	startDBusMenu("menu.name", "/menu", JSON_SHORTCUTS);

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	// There are no GMenus in this test
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"for arg in args[2]:\n"
					"  ret.append('')").waitForFinished();

	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('menu.name', '/menu')").waitForFinished();

	startHud();

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));
	client.setQuery("quit");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() == 0) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(1, results.rowCount());
	EXPECT_EQ(ResultPair("Quiter", ""), result(results, 0));
}

TEST_F(TestHud, SearchGMenuOneResult) {
	startGMenu("menu.name", "/menu", MODEL_SHORTCUTS);

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"if args[0] == 0:\n"
					"  ret.append('menu.name')\n"
					"  ret.append('')\n"
					"  ret.append('/menu')\n"
					"  ret.append('')\n"
					"  ret.append('')\n"
					"  ret.append('/menu')\n"
					"else:\n"
					"  for arg in args[2]:\n"
					"    ret.append('')").waitForFinished();

	// There are no DBusMenus in this test
	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('', '/')").waitForFinished();

	startHud();

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));
	client.setQuery("closr");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() == 0) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(1, results.rowCount());
	EXPECT_EQ(ResultPair("Close", ""), result(results, 0));
}

TEST_F(TestHud, SearchLibHudOneResult) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	// No GMenu
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"for arg in args[2]:\n"
					"  ret.append('')").waitForFinished();

	// There are no DBusMenus in this test
	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('', '/')").waitForFinished();

	startHud();

	startLibHud("app0", "test.app", "/test", MODEL_LIBHUD);
	ComCanonicalHudTestInterface libhudTestInterface("test.app", "/test",
			dbus.sessionConnection());

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));

	client.setQuery("quitter");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() == 0) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(1, results.rowCount());
	EXPECT_EQ(ResultPair("quiter", ""), result(results, 0));

	const QAbstractListModel &toolbarModel(*client.toolBarModel());
	ASSERT_EQ(5, toolbarModel.rowCount());

	EXPECT_TOOLBAR(toolbarModel, 0, "graphics/close.png",
			HUD_CLIENT_QUERY_TOOLBAR_QUIT, false);
	EXPECT_TOOLBAR(toolbarModel, 1, "graphics/undo.png",
			HUD_CLIENT_QUERY_TOOLBAR_UNDO, true);
	EXPECT_TOOLBAR(toolbarModel, 2, "graphics/help.png",
			HUD_CLIENT_QUERY_TOOLBAR_HELP, true);
	EXPECT_TOOLBAR(toolbarModel, 3, "graphics/view-fullscreen.png",
			HUD_CLIENT_QUERY_TOOLBAR_FULLSCREEN, false);
	EXPECT_TOOLBAR(toolbarModel, 4, "graphics/settings.png",
			HUD_CLIENT_QUERY_TOOLBAR_PREFERENCES, false);

	QSignalSpy actionInvokedSpy(&libhudTestInterface,
				SIGNAL(ActionInvoked(const QString &)));
	client.executeToolBarAction(HUD_CLIENT_QUERY_TOOLBAR_UNDO);
	actionInvokedSpy.wait();
	EXPECT_EQ(QVariantList() << QVariant("undoer"), actionInvokedSpy.at(0));
}

TEST_F(TestHud, ExecuteGMenuAction) {
	startGMenu("menu.name", "/menu", MODEL_SHORTCUTS);
	ComCanonicalHudTestInterface gmenuTestInterface("menu.name", "/menu",
			dbus.sessionConnection());

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"if args[0] == 0:\n"
					"  ret.append('menu.name')\n"
					"  ret.append('')\n"
					"  ret.append('/menu')\n"
					"  ret.append('')\n"
					"  ret.append('')\n"
					"  ret.append('/menu')\n"
					"else:\n"
					"  for arg in args[2]:\n"
					"    ret.append('')").waitForFinished();

	// There are no DBusMenus in this test
	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('', '/')").waitForFinished();

	startHud();

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));
	client.setQuery("closr");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() == 0) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(1, results.rowCount());
	EXPECT_EQ(ResultPair("Close", ""), result(results, 0));

	QSignalSpy actionInvokedSpy(&gmenuTestInterface,
			SIGNAL(ActionInvoked(const QString &)));

	QSignalSpy executedSpy(&client, SIGNAL(commandExecuted()));
	client.executeCommand(0);
	EXPECT_FALSE(executedSpy.isEmpty());

	actionInvokedSpy.wait();
	ASSERT_FALSE(actionInvokedSpy.isEmpty());
	EXPECT_EQ(QVariantList() << QVariant("close"), actionInvokedSpy.at(0));
}

TEST_F(TestHud, ExecuteParameterized) {
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowStack", "", "a(usbu)", "ret = [(0, 'app0', True, 0)]").waitForFinished();

	// No GMenu
	windowStackMock().AddMethod(DBusTypes::WINDOW_STACK_DBUS_NAME,
			"GetWindowProperties", "usas", "as", "ret = []\n"
					"for arg in args[2]:\n"
					"  ret.append('')").waitForFinished();

	// There are no DBusMenus in this test
	appmenuRegstrarMock().AddMethod(DBusTypes::APPMENU_REGISTRAR_DBUS_NAME,
			"GetMenuForWindow", "u", "so", "ret = ('', '/')").waitForFinished();

	startHud();

	startLibHud("app0", "test.app", "/test", MODEL_LIBHUD);
	ComCanonicalHudTestInterface gmenuTestInterface("test.app", "/test",
			dbus.sessionConnection());
	QSignalSpy actionInvokedSpy(&gmenuTestInterface,
			SIGNAL(
					ParameterizedActionInvoked(const QString &, const QDBusVariant &)));

	HudClient client;
	QSignalSpy modelsChangedSpy(&client, SIGNAL(modelsChanged()));
	modelsChangedSpy.wait();

	QSignalSpy countChangedSpy(client.results(), SIGNAL(countChanged()));
	client.setQuery("fruiy");
	countChangedSpy.wait();

	QAbstractListModel &results(*client.results());
	if(results.rowCount() == 0) {
		countChangedSpy.wait();
	}
	ASSERT_EQ(1, results.rowCount());
	ASSERT_EQ(ResultPair("fruit", ""), result(results, 0));

	QSignalSpy executedSpy(&client,
			SIGNAL(showParametrizedAction(const QString &, const QVariant &)));
	client.executeCommand(0);

	executedSpy.wait();
	ASSERT_FALSE(executedSpy.isEmpty());
	QVariantMap parameters;
	parameters["hud.apple"] = 1.0;
	parameters["hud.banana"] = 2.0;
	parameters["hud.cranberry"] = 3.0;
	client.executeParametrizedAction(parameters);

	for (uint count(0); count < 5; ++count) {
		actionInvokedSpy.wait();
		if (actionInvokedSpy.size() == 5) {
			break;
		}
	}

	ASSERT_EQ(5, actionInvokedSpy.size());
	EXPECT_RESULT(actionInvokedSpy, 0, "apple", QVariant(1.0));
	EXPECT_RESULT(actionInvokedSpy, 1, "banana", QVariant(2.0));
	EXPECT_RESULT(actionInvokedSpy, 2, "cranberry", QVariant(3.0));
	EXPECT_RESULT(actionInvokedSpy, 3, "fruit", QVariant("s"));
	EXPECT_RESULT(actionInvokedSpy, 4, "fruit", QVariant("s"));
}

} // namespace
