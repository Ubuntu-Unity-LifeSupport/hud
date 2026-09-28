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

#ifndef HUD_TESTUTILS_MOCKHUDSERVICE_H_
#define HUD_TESTUTILS_MOCKHUDSERVICE_H_

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>

#include <QScopedPointer>

namespace hud {
namespace common {

class AppstackModel;
class ResultsModel;

}

namespace testutils {

class Q_DECL_EXPORT MockHudService {
public:
	static const QString QUERY_PATH;

	MockHudService(QtDBusTest::DBusTestRunner &dbus,
			QtDBusMock::DBusMock &mock);

	virtual ~MockHudService();

	void loadMethods();

	OrgFreedesktopDBusMockInterface & hudInterface();

	OrgFreedesktopDBusMockInterface & applicationInterface();

	OrgFreedesktopDBusMockInterface & queryInterface();

protected:
	QtDBusTest::DBusTestRunner &m_dbus;

	QtDBusMock::DBusMock &m_mock;

	QScopedPointer<hud::common::AppstackModel> m_appstack;

	QScopedPointer<hud::common::ResultsModel> m_results;
};

}
}
#endif /* HUD_TESTUTILS_MOCKHUDSERVICE_H_ */
