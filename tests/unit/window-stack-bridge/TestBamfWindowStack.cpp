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
#include <common/WindowStackInterface.h>
#include <window-stack-bridge/BamfWindowStack.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <QSignalSpy>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace hud::common;
using namespace QtDBusTest;
using namespace QtDBusMock;

namespace {

class TestBamfWindowStack: public Test {
protected:
	TestBamfWindowStack() :
			mock(dbus) {

		mock.registerCustomMock(DBusTypes::BAMF_DBUS_NAME,
				DBusTypes::BAMF_MATCHER_DBUS_PATH,
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);

		dbus.startServices();
	}

	virtual ~TestBamfWindowStack() {
	}

	OrgFreedesktopDBusMockInterface & bamfMatcherMock() {
		return mock.mockInterface(DBusTypes::BAMF_DBUS_NAME,
				DBusTypes::BAMF_MATCHER_DBUS_PATH,
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);
	}

	OrgFreedesktopDBusMockInterface & windowMock(uint id) {
		return mock.mockInterface(DBusTypes::BAMF_DBUS_NAME, windowPath(id),
				OrgAyatanaBamfWindowInterface::staticInterfaceName(),
				QDBusConnection::SessionBus);
	}

	static void addMethod(QList<Method> &methods, const QString &name,
			const QString &inSig, const QString &outSig, const QString &code) {
		Method method;
		method.setName(name);
		method.setInSig(inSig);
		method.setOutSig(outSig);
		method.setCode(code);
		methods << method;
	}

	static QString applicationPath(uint id) {
		return QString("/org/ayatana/bamf/application%1").arg(id);
	}

	static QString windowPath(uint id) {
		return QString("/org/ayatana/bamf/window%1").arg(id);
	}

	void createApplication(uint applicationId) {
		QVariantMap properties;

		QList<Method> methods;
		addMethod(methods, "DesktopFile", "", "s",
				QString("ret = '/usr/share/applications/appid-%1.desktop'").arg(
						applicationId));

		bamfMatcherMock().AddObject(applicationPath(applicationId),
				OrgAyatanaBamfApplicationInterface::staticInterfaceName(),
				properties, methods).waitForFinished();
	}

	void createWindow(uint windowId, uint applicationId, bool propertyMethod =
			true) {
		QVariantMap properties;

		QList<Method> methods;
		addMethod(methods, "GetXid", "", "u",
				QString("ret = %1").arg(windowId));
		if (propertyMethod) {
			addMethod(methods, "Xprop", "s", "s", "ret = 'foo'");
		}

		bamfMatcherMock().AddObject(windowPath(windowId),
				OrgAyatanaBamfWindowInterface::staticInterfaceName(),
				properties, methods).waitForFinished();

		QList<Method> viewMethods;
		addMethod(viewMethods, "Parents", "", "as",
				QString("ret = ['%1']").arg(applicationPath(applicationId)));

		windowMock(windowId).AddMethods("org.ayatana.bamf.view", viewMethods).waitForFinished();
	}

	void createMatcherMethods(uint windowCount, uint activeWindow) {
		bamfMatcherMock().AddMethod(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ActiveWindow", "", "s",
				QString("ret = '%1'").arg(windowPath(activeWindow))).waitForFinished();

		QString windowStack("ret = [");
		if (windowCount > 0) {
			windowStack.append(QString("'%1'").arg(windowPath(activeWindow)));
			for (uint i(0); i < windowCount; ++i) {
				if (i != activeWindow) {
					windowStack.append(",\'");
					windowStack.append(windowPath(i));
					windowStack.append('\'');
				}
			}
		}
		windowStack.append(']');

		bamfMatcherMock().AddMethod(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"WindowPaths", "", "as", windowStack).waitForFinished();
		bamfMatcherMock().AddMethod(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"WindowStackForMonitor", "i", "as", windowStack).waitForFinished();
	}

	void windowChanged(const QString &oldPath, const QString &newPath) {
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ActiveWindowChanged", "ss", QVariantList() << oldPath << "");
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ActiveWindowChanged", "ss", QVariantList() << "" << newPath);
	}

	void windowClosed(const QString &closedPath, const QString &newPath) {
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ActiveWindowChanged", "ss",
				QVariantList() << closedPath << newPath);
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ViewClosed", "ss", QVariantList() << closedPath << "window");
	}

	void windowOpened(const QString &oldPath, const QString &openedPath) {
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ViewOpened", "ss", QVariantList() << openedPath << "window");
		bamfMatcherMock().EmitSignal(
				OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
				"ActiveWindowChanged", "ss",
				QVariantList() << oldPath << openedPath);
	}

	DBusTestRunner dbus;

	DBusMock mock;
};

TEST_F(TestBamfWindowStack, ExportsDBusInterface) {
	bamfMatcherMock().AddMethod(
			OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
			"WindowPaths", "", "as", "ret = []").waitForFinished();

	BamfWindowStack windowStack(dbus.sessionConnection());

	ComCanonicalUnityWindowStackInterface windowStackInterface(
			DBusTypes::WINDOW_STACK_DBUS_NAME,
			DBusTypes::WINDOW_STACK_DBUS_PATH, dbus.sessionConnection());

	ASSERT_TRUE(windowStackInterface.isValid());
}

TEST_F(TestBamfWindowStack, HandlesEmptyWindowStack) {
	createMatcherMethods(0, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());

	QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
	EXPECT_TRUE(windowInfos.empty());
}

TEST_F(TestBamfWindowStack, OverDBus) {
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createMatcherMethods(2, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());

	ComCanonicalUnityWindowStackInterface windowStackInterface(
			DBusTypes::WINDOW_STACK_DBUS_NAME,
			DBusTypes::WINDOW_STACK_DBUS_PATH, dbus.sessionConnection());

	QDBusPendingReply<WindowInfoList> reply(
			windowStackInterface.GetWindowStack());
	QDBusPendingCallWatcher watcher(reply);
	QSignalSpy spy(&watcher, SIGNAL(finished(QDBusPendingCallWatcher *)));
	spy.wait();
	EXPECT_FALSE(spy.isEmpty());

	QList<WindowInfo> windowInfos(reply);
	ASSERT_EQ(2, windowInfos.size());
	EXPECT_EQ(WindowInfo(0, "appid-0", true, WindowInfo::MAIN),
			windowInfos.at(0));
	EXPECT_EQ(WindowInfo(1, "appid-0", false, WindowInfo::MAIN),
			windowInfos.at(1));
}

TEST_F(TestBamfWindowStack, HandlesTwoWindows) {
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createMatcherMethods(2, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());

	QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
	ASSERT_EQ(2, windowInfos.size());
	EXPECT_EQ(WindowInfo(0, "appid-0", true, WindowInfo::MAIN),
			windowInfos.at(0));
	EXPECT_EQ(WindowInfo(1, "appid-0", false, WindowInfo::MAIN),
			windowInfos.at(1));
}

TEST_F(TestBamfWindowStack, HandlesMissingWindow) {
	createMatcherMethods(1, 0);

	qDebug() << "EXPECTED ERROR BELOW";
	BamfWindowStack windowStack(dbus.sessionConnection());
	qDebug() << "EXPECTED ERROR ABOVE";

	QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
	ASSERT_EQ(0, windowInfos.size());
}

TEST_F(TestBamfWindowStack, GetWindowPropertiesForBrokenWindow) {
	createWindow(0, 0, false);
	createApplication(0);

	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());

	qDebug() << "EXPECTED ERROR BELOW";
	QStringList properties(
			windowStack.GetWindowProperties(0, "unknown",
					QStringList() << "some-random-property"));
	qDebug() << "EXPECTED ERROR ABOVE";

	ASSERT_EQ(QStringList() << "", properties);
}

TEST_F(TestBamfWindowStack, HandlesTwoApplications) {
	// app 0
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createWindow(2, 0);

	// app 1
	createApplication(1);
	createWindow(3, 1);
	createWindow(4, 1);

	createMatcherMethods(5, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());

	QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
	ASSERT_EQ(5, windowInfos.size());
	EXPECT_EQ(WindowInfo(0, "appid-0", true, WindowInfo::MAIN),
			windowInfos.at(0));
	EXPECT_EQ(WindowInfo(1, "appid-0", false, WindowInfo::MAIN),
			windowInfos.at(1));
	EXPECT_EQ(WindowInfo(2, "appid-0", false, WindowInfo::MAIN),
			windowInfos.at(2));
	EXPECT_EQ(WindowInfo(3, "appid-1", false, WindowInfo::MAIN),
			windowInfos.at(3));
	EXPECT_EQ(WindowInfo(4, "appid-1", false, WindowInfo::MAIN),
			windowInfos.at(4));
}

TEST_F(TestBamfWindowStack, FocusedWindowChanged) {
	// app 0
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createWindow(2, 0);

	// app 1
	createApplication(1);
	createWindow(3, 1);
	createWindow(4, 1);

	createMatcherMethods(5, 3);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QSignalSpy windowChangedSpy(&windowStack,
	SIGNAL(FocusedWindowChanged(uint, const QString &, uint)));

	windowChanged(windowPath(0), windowPath(3));
	windowChangedSpy.wait();
	ASSERT_EQ(1, windowChangedSpy.size());
	EXPECT_EQ(QVariantList() << uint(3) << "appid-1" << uint(0),
			windowChangedSpy.at(0));

	{
		QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
		ASSERT_EQ(5, windowInfos.size());
		EXPECT_EQ(WindowInfo(3, "appid-1", true, WindowInfo::MAIN),
				windowInfos.at(0));
		EXPECT_EQ(WindowInfo(0, "appid-0", false, WindowInfo::MAIN),
				windowInfos.at(1));
		EXPECT_EQ(WindowInfo(1, "appid-0", false, WindowInfo::MAIN),
				windowInfos.at(2));
		EXPECT_EQ(WindowInfo(2, "appid-0", false, WindowInfo::MAIN),
				windowInfos.at(3));
		EXPECT_EQ(WindowInfo(4, "appid-1", false, WindowInfo::MAIN),
				windowInfos.at(4));
	}
}

TEST_F(TestBamfWindowStack, WindowDestroyed) {
	// app 0
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createWindow(2, 0);

	// app 1
	createApplication(1);
	createWindow(3, 1);
	createWindow(4, 1);

	createMatcherMethods(5, 4);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	windowClosed(windowPath(4), windowPath(0));
	windowDestroyedSpy.wait();
	ASSERT_EQ(1, windowDestroyedSpy.size());
	EXPECT_EQ(QVariantList() << uint(4) << "appid-1", windowDestroyedSpy.at(0));
}

TEST_F(TestBamfWindowStack, WindowCreated) {
	// app 0
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 0);
	createWindow(2, 0);

	// app 1
	createApplication(1);
	createWindow(3, 1);
	createWindow(4, 1);


	createMatcherMethods(5, 4);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QSignalSpy windowCreatedSpy(&windowStack,
	SIGNAL(WindowCreated(uint, const QString &)));

	createWindow(5, 1);
	windowOpened(windowPath(4), windowPath(5));
	windowCreatedSpy.wait();
	ASSERT_EQ(1, windowCreatedSpy.size());
	EXPECT_EQ(QVariantList() << uint(5) << "appid-1", windowCreatedSpy.at(0));
}

} // namespace
