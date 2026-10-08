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

	void createApplication(uint applicationId, bool desktopFile = true) {
		QVariantMap properties;

		QList<Method> methods;
		addMethod(methods, "DesktopFile", "", "s",
				desktopFile ?
						QString("ret = '/usr/share/applications/appid-%1.desktop'").arg(
								applicationId) :
						QString("ret = ''"));

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

	// bamf moves a window to another application: Parents() names the new one
	// (or nothing, with an empty list), and the new application sends
	// WindowAdded; no ViewOpened or ViewClosed for the window
	void setParents(uint windowId, const QString &parents) {
		windowMock(windowId).AddMethod("org.ayatana.bamf.view", "Parents", "",
				"as", QString("ret = [%1]").arg(parents)).waitForFinished();
	}

	void moveWindow(uint windowId, uint applicationId) {
		setParents(windowId, QString("'%1'").arg(applicationPath(applicationId)));
		windowAdded(applicationId, windowId);
	}

	void windowAdded(uint applicationId, uint windowId) {
		mock.mockInterface(DBusTypes::BAMF_DBUS_NAME,
				applicationPath(applicationId),
				OrgAyatanaBamfApplicationInterface::staticInterfaceName(),
				QDBusConnection::SessionBus).EmitSignal(
				"org.ayatana.bamf.application", "WindowAdded", "s",
				QVariantList() << windowPath(windowId)).waitForFinished();
	}

	// The bridge's WindowCreated, FocusedWindowChanged and WindowDestroyed, in
	// the order it sends them
	void recordSignals(BamfWindowStack &windowStack, QStringList &log) {
		QObject::connect(&windowStack, &BamfWindowStack::WindowCreated,
				[&log](uint id, const QString &app) {
					log << QString("created %1 %2").arg(id).arg(app);
				});
		QObject::connect(&windowStack, &BamfWindowStack::FocusedWindowChanged,
				[&log](uint id, const QString &app, uint stage) {
					log << QString("focused %1 %2 %3").arg(id).arg(app).arg(stage);
				});
		QObject::connect(&windowStack, &BamfWindowStack::WindowDestroyed,
				[&log](uint id, const QString &app) {
					log << QString("destroyed %1 %2").arg(id).arg(app);
				});
	}

	// Let queued D-Bus signals reach the bridge
	static void settle(int ms = 300) {
		QSignalSpy never(qApp, SIGNAL(destroyed()));
		never.wait(ms);
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

// bamf re-matches a window to another application, and the one Parents()
// named can be gone before DesktopFile() is asked: the window stays, with its
// id as application id.
TEST_F(TestBamfWindowStack, HandlesWindowWhoseApplicationIsGone) {
	createWindow(0, 7); // application 7 is never exported
	createMatcherMethods(1, 0);

	qDebug() << "EXPECTED ERROR BELOW";
	BamfWindowStack windowStack(dbus.sessionConnection());
	qDebug() << "EXPECTED ERROR ABOVE";

	QList<WindowInfo> windowInfos(windowStack.GetWindowStack());
	ASSERT_EQ(1, windowInfos.size());
	EXPECT_EQ(WindowInfo(0, "0", true, WindowInfo::MAIN), windowInfos.at(0));
}

TEST_F(TestBamfWindowStack, WindowCreatedWhenApplicationIsGone) {
	createApplication(0);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QSignalSpy windowCreatedSpy(&windowStack,
	SIGNAL(WindowCreated(uint, const QString &)));

	createWindow(5, 9); // application 9 is never exported
	qDebug() << "EXPECTED ERROR BELOW";
	windowOpened(windowPath(0), windowPath(5));
	windowCreatedSpy.wait();
	qDebug() << "EXPECTED ERROR ABOVE";
	ASSERT_EQ(1, windowCreatedSpy.size());
	EXPECT_EQ(QVariantList() << uint(5) << "5", windowCreatedSpy.at(0));
}

TEST_F(TestBamfWindowStack, WindowDestroyedWhenApplicationWasGone) {
	createApplication(0);
	createWindow(0, 0);
	createWindow(1, 7); // application 7 is never exported
	createMatcherMethods(2, 0);

	qDebug() << "EXPECTED ERROR BELOW";
	BamfWindowStack windowStack(dbus.sessionConnection());
	qDebug() << "EXPECTED ERROR ABOVE";
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	windowClosed(windowPath(1), windowPath(0));
	windowDestroyedSpy.wait();
	ASSERT_EQ(1, windowDestroyedSpy.size());
	EXPECT_EQ(QVariantList() << uint(1) << "1", windowDestroyedSpy.at(0));
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

// UNITY-20260929-001: bamf announces LibreOffice's window under a temporary
// application without a desktop file and then moves it to libreoffice-writer.
// The bridge follows the move: the window under its new id first, the focus
// if the bridge reported this window as focused, then the old id removed.
TEST_F(TestBamfWindowStack, FocusedWindowMovedToItsApplication) {
	createApplication(2, false); // the temporary application
	createApplication(1);
	createWindow(0, 2);
	createMatcherMethods(1, 0); // window 0 is bamf's active window

	BamfWindowStack windowStack(dbus.sessionConnection());
	ASSERT_EQ(WindowInfo(0, "0", true, WindowInfo::MAIN),
			windowStack.GetWindowStack().at(0));
	QStringList log;
	recordSignals(windowStack, log);
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	moveWindow(0, 1);
	windowDestroyedSpy.wait();
	EXPECT_EQ(QStringList() << "created 0 appid-1" << "focused 0 appid-1 0"
			<< "destroyed 0 0", log);
	EXPECT_EQ(WindowInfo(0, "appid-1", true, WindowInfo::MAIN),
			windowStack.GetWindowStack().at(0));
}

// A window known from the start that bamf reports active is focused for
// hud-service (it reads GetWindowStack): a move before any
// ActiveWindowChanged still gives the focus to the new id.
TEST_F(TestBamfWindowStack, StartupFocusedWindowMovedWithoutActiveWindowChanged) {
	createApplication(2, false);
	createApplication(1);
	createWindow(0, 2);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	moveWindow(0, 1);
	windowDestroyedSpy.wait();
	EXPECT_EQ(QStringList() << "created 0 appid-1" << "focused 0 appid-1 0"
			<< "destroyed 0 0", log);
}

TEST_F(TestBamfWindowStack, UnfocusedWindowMovedWithoutFocus) {
	createApplication(0);
	createApplication(2, false);
	createApplication(1);
	createWindow(0, 0);
	createWindow(1, 2);
	createMatcherMethods(2, 0); // window 0 is active

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	moveWindow(1, 1);
	windowDestroyedSpy.wait();
	EXPECT_EQ(QStringList() << "created 1 appid-1" << "destroyed 1 1", log);
}

TEST_F(TestBamfWindowStack, WindowAddedFromSameApplicationChangesNothing) {
	createApplication(0);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);

	windowAdded(0, 0);
	settle();
	EXPECT_TRUE(log.isEmpty()) << log.join(", ").toStdString();
}

TEST_F(TestBamfWindowStack, WindowAddedForUnknownWindowChangesNothing) {
	createApplication(0);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);

	windowAdded(0, 8); // window 8 was never opened
	settle();
	EXPECT_TRUE(log.isEmpty()) << log.join(", ").toStdString();
}

// No answer from bamf keeps the id: the new application's DesktopFile()
// fails, or the window has no parent while it moves or closes.
TEST_F(TestBamfWindowStack, WindowAddedWithDesktopFileErrorKeepsId) {
	createApplication(0);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);

	setParents(0, QString("'%1'").arg(applicationPath(7))); // never exported
	qDebug() << "EXPECTED ERROR BELOW";
	windowAdded(7, 0);
	settle();
	qDebug() << "EXPECTED ERROR ABOVE";
	EXPECT_TRUE(log.isEmpty()) << log.join(", ").toStdString();
	EXPECT_EQ(WindowInfo(0, "appid-0", true, WindowInfo::MAIN),
			windowStack.GetWindowStack().at(0));
}

TEST_F(TestBamfWindowStack, WindowAddedWithoutParentKeepsId) {
	createApplication(0);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);

	setParents(0, "");
	windowAdded(0, 0);
	settle();
	EXPECT_TRUE(log.isEmpty()) << log.join(", ").toStdString();
	EXPECT_EQ(WindowInfo(0, "appid-0", true, WindowInfo::MAIN),
			windowStack.GetWindowStack().at(0));
}

TEST_F(TestBamfWindowStack, MovedWindowKeepsNewIdInLaterSignals) {
	createApplication(0);
	createApplication(2, false);
	createApplication(1);
	createWindow(0, 0);
	createWindow(1, 2);
	createMatcherMethods(2, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));
	QSignalSpy windowChangedSpy(&windowStack,
	SIGNAL(FocusedWindowChanged(uint, const QString &, uint)));

	moveWindow(1, 1);
	windowDestroyedSpy.wait();
	windowDestroyedSpy.clear();

	windowChanged(windowPath(0), windowPath(1));
	windowChangedSpy.wait();
	ASSERT_FALSE(windowChangedSpy.isEmpty());
	EXPECT_EQ(QVariantList() << uint(1) << "appid-1" << uint(0),
			windowChangedSpy.last());

	windowClosed(windowPath(1), windowPath(0));
	windowDestroyedSpy.wait();
	ASSERT_EQ(1, windowDestroyedSpy.size());
	EXPECT_EQ(QVariantList() << uint(1) << "appid-1", windowDestroyedSpy.at(0));
}

// The other side of the race: bamf moves the window between the bridge's
// Parents() and DesktopFile() calls, so the first application is gone when
// it is asked; the bridge announces the window number, and the WindowAdded of
// the move, handled afterwards, corrects it.
TEST_F(TestBamfWindowStack, WindowOpenedDuringMoveIsCorrected) {
	createApplication(0);
	createApplication(1);
	createWindow(0, 0);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));
	QSignalSpy windowChangedSpy(&windowStack,
	SIGNAL(FocusedWindowChanged(uint, const QString &, uint)));

	createWindow(5, 9); // application 9 is never exported: gone when asked
	qDebug() << "EXPECTED ERROR BELOW";
	windowOpened(windowPath(0), windowPath(5));
	windowChangedSpy.wait();
	qDebug() << "EXPECTED ERROR ABOVE";
	moveWindow(5, 1);
	windowDestroyedSpy.wait();
	EXPECT_EQ(QStringList() << "created 5 5" << "focused 5 5 0"
			<< "created 5 appid-1" << "focused 5 appid-1 0"
			<< "destroyed 5 5", log);
}

// ActiveWindowChanged to a window the bridge does not know reports nothing,
// so hud-service keeps the window it had focused, and so does the bridge.
TEST_F(TestBamfWindowStack, FocusKeptOverUnknownActiveWindow) {
	createApplication(2, false);
	createApplication(1);
	createWindow(0, 2);
	createMatcherMethods(1, 0);

	BamfWindowStack windowStack(dbus.sessionConnection());
	QStringList log;
	recordSignals(windowStack, log);
	QSignalSpy windowDestroyedSpy(&windowStack,
	SIGNAL(WindowDestroyed(uint, const QString &)));

	bamfMatcherMock().EmitSignal(
			OrgAyatanaBamfMatcherInterface::staticInterfaceName(),
			"ActiveWindowChanged", "ss",
			QVariantList() << windowPath(0) << windowPath(9)).waitForFinished();
	settle();
	moveWindow(0, 1);
	windowDestroyedSpy.wait();
	EXPECT_EQ(QStringList() << "created 0 appid-1" << "focused 0 appid-1 0"
			<< "destroyed 0 0", log);
}

} // namespace
