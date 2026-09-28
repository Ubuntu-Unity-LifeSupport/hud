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

#include <testutils/MockHudService.h>
#include <common/AppstackModel.h>
#include <common/DBusTypes.h>
#include <common/ResultsModel.h>

using namespace hud::common;
using namespace hud::testutils;
using namespace QtDBusTest;
using namespace QtDBusMock;

MockHudService::MockHudService(DBusTestRunner &dbus, DBusMock &mock) :
		m_dbus(dbus), m_mock(mock) {
	mock.registerCustomMock(DBusTypes::HUD_SERVICE_DBUS_NAME,
			DBusTypes::HUD_SERVICE_DBUS_PATH, DBusTypes::HUD_SERVICE_DBUS_NAME,
			QDBusConnection::SessionBus);
}

const QString MockHudService::QUERY_PATH("/com/canonical/hud/query0");

static void addMethod(QList<Method> &methods, const QString &name,
		const QString &inSig, const QString &outSig, const QString &code) {
	Method method;
	method.setName(name);
	method.setInSig(inSig);
	method.setOutSig(outSig);
	method.setCode(code);
	methods << method;
}

OrgFreedesktopDBusMockInterface & MockHudService::hudInterface() {
	return m_mock.mockInterface(DBusTypes::HUD_SERVICE_DBUS_NAME,
			DBusTypes::HUD_SERVICE_DBUS_PATH, DBusTypes::HUD_SERVICE_DBUS_NAME,
			QDBusConnection::SessionBus);
}

OrgFreedesktopDBusMockInterface & MockHudService::applicationInterface() {
	return m_mock.mockInterface(DBusTypes::HUD_SERVICE_DBUS_NAME, "/app/object",
			"com.canonical.hud.Application", QDBusConnection::SessionBus);
}

OrgFreedesktopDBusMockInterface & MockHudService::queryInterface() {
	return m_mock.mockInterface(DBusTypes::HUD_SERVICE_DBUS_NAME, QUERY_PATH,
			"com.canonical.hud.query", QDBusConnection::SessionBus);
}

void MockHudService::loadMethods() {
	OrgFreedesktopDBusMockInterface &hud(hudInterface());

	{
		QVariantMap properties;
		properties["ResultsModel"] = "com.canonical.hud.query0.results";
		properties["AppstackModel"] = "com.canonical.hud.query0.appstack";
		properties["ToolbarItems"] = QStringList();

		QList<Method> methods;
		addMethod(methods, "UpdateQuery", "s", "i", "ret = 1");
		addMethod(methods, "VoiceQuery", "", "is", "ret = (1, 'voice query')");
		addMethod(methods, "UpdateApp", "s", "i", "ret = 1");
		addMethod(methods, "CloseQuery", "", "", "");
		addMethod(methods, "ExecuteCommand", "vu", "", "");
		addMethod(methods, "ExecuteParameterized", "vu", "sssooi",
				"ret = ('" + m_dbus.sessionConnection().baseService()
						+ "', 'hud', 'action', '/action/path', '/model/path', 1)");
		addMethod(methods, "ExecuteToolbar", "su", "", "");

		hud.AddObject(QUERY_PATH, "com.canonical.hud.query", properties,
				methods).waitForFinished();
	}

	m_results.reset(new ResultsModel(0));
	m_results->beginChangeset();
	m_results->addResult(0, "result1", QList<QPair<int, int>>(), "description1",
			QList<QPair<int, int>>(), "shortcut", 1, false);
	m_results->endChangeset();

	m_appstack.reset(new AppstackModel(0));
	m_appstack->beginChangeset();
	m_appstack->addApplication("application-id", "icon",
			AppstackModel::ITEM_TYPE_FOCUSED_APP);
	m_appstack->endChangeset();

	// Mock application
	{
		QVariantMap properties;
		QList<Method> methods;
		addMethod(methods, "AddSources", "a(usso)a(uso)", "", "");
		hud.AddObject("/app/object", "com.canonical.hud.Application",
				properties, methods).waitForFinished();
	}

	/* query */
	hud.AddMethod(DBusTypes::HUD_SERVICE_DBUS_NAME, "CreateQuery", "s", "ossi",
			"ret = ('/com/canonical/hud/query0', 'com.canonical.hud.query0.results', 'com.canonical.hud.query0.appstack', dbus.Int32(0))").waitForFinished();

	/* id */
	hud.AddMethod(DBusTypes::HUD_SERVICE_DBUS_NAME, "RegisterApplication", "s",
			"o", "ret = ('/app/object')").waitForFinished();
}

MockHudService::~MockHudService() {
}

