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
#include <libqtdbusmock/DBusMock.h>
#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "minimal");
	qputenv("HUD_IGNORE_SEARCH_SETTINGS", "1");
	qputenv("HUD_STORE_USAGE_DATA", "FALSE");

//	qputenv("LANG", "C.UTF-8");
	unsetenv("LC_ALL");

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain(GETTEXT_PACKAGE);

	QApplication application(argc, argv);
	hud::common::DBusTypes::registerMetaTypes();
	QtDBusMock::DBusMock::registerMetaTypes();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
