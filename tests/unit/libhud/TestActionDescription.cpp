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
#include <libhud/hud.h>
#include <common/shared-values.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <QTestEventLoop>
#include <gtest/gtest.h>

using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::common;
using namespace hud::testutils;

namespace {

class TestActionDescription: public Test {
protected:
	TestActionDescription() :
			mock(dbus), hud(dbus, mock) {
		dbus.startServices();
		hud.loadMethods();
	}

	virtual ~TestActionDescription() {
	}

	DBusTestRunner dbus;

	DBusMock mock;

	MockHudService hud;
};

TEST_F(TestActionDescription, WithAttributeValue) {
	HudActionDescription *description = hud_action_description_new(
			"hud.simple-action", g_variant_new_string("Foo"));
	hud_action_description_set_attribute_value(description,
	G_MENU_ATTRIBUTE_LABEL, g_variant_new_string("Simple Action"));

	EXPECT_STREQ("hud.simple-action",
			hud_action_description_get_action_name(description));

	EXPECT_STREQ("Foo",
			g_variant_get_string(
					hud_action_description_get_action_target(description), 0));

	hud_action_description_unref(description);
}

TEST_F(TestActionDescription, WithAttribute) {
	HudActionDescription *description = hud_action_description_new(
			"hud.simple-action", g_variant_new_string("Bar"));
	hud_action_description_set_attribute(description, G_MENU_ATTRIBUTE_LABEL,
			"s", "Simple Action");

	EXPECT_STREQ("hud.simple-action",
			hud_action_description_get_action_name(description));
	EXPECT_STREQ("Bar",
			g_variant_get_string(
					hud_action_description_get_action_target(description), 0));

	hud_action_description_unref(description);
}

} // namespace
