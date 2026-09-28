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
#include <libqtdbustest/QProcessDBusService.h>
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

class TestParam: public Test {

protected:
	TestParam() :
			mock(dbus) {
		dbus.startServices();
	}

	virtual ~TestParam() {
	}

	void startDBusMenu(const QString &name, const QString &path,
			const QString &json) {
		menuService.reset(
				new QProcessDBusService(name, QDBusConnection::SessionBus,
				DBUSMENU_JSON_LOADER, QStringList() << name << path << json));
		menuService->start(dbus.sessionConnection());
	}

protected:

	DBusTestRunner dbus;

	DBusMock mock;

	QSharedPointer<QProcessDBusService> menuService;
};

TEST_F(TestParam, Create) {
	HudClientParam *param = hud_client_param_new("app.dbus.name", "prefix", "base_action",
			"/action/path", "/model/path", 1);

	EXPECT_TRUE(param);

	g_object_unref(param);
}

TEST_F(TestParam, GetActions) {
	startDBusMenu("app.dbus.name", "/menu", JSON_SOURCE_ONE);

	HudClientParam* param = hud_client_param_new("app.dbus.name", "prefix", "base_action",
			"/action/path", "/model/path", 1);

	GActionGroup *action_group = hud_client_param_get_actions(param);
	EXPECT_TRUE(action_group);

	g_object_unref(param);
}

}
