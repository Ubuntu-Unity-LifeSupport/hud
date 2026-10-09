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

#include <common/ApplicationInterface.h>
#include <service/Factory.h>
#include <service/ApplicationImpl.h>
#include <unit/service/Mocks.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbusmock/DBusMock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace QtDBusTest;
using namespace QtDBusMock;
using namespace hud::common;
using namespace hud::service;
using namespace hud::service::test;

namespace {

class TestApplication: public Test {
protected:
	TestApplication() :
			mock(dbus) {
		factory.setSessionBus(dbus.sessionConnection());

		allWindowsContext.reset(new NiceMock<MockWindowContext>());

		EXPECT_CALL(factory, newWindowContext()).WillOnce(
				Return(allWindowsContext));
	}

	virtual ~TestApplication() {
	}

	static void addAction(QList<hud::common::Action> &actions,
			unsigned int windowId, const QString &context,
			const QString &prefix, const QDBusObjectPath &object) {
		hud::common::Action action;
		action.m_windowId = windowId;
		action.m_context = context;
		action.m_prefix = prefix;
		action.m_object = object;
		actions << action;
	}

	static void addMenu(QList<hud::common::Description> &descriptions,
			unsigned int windowId, const QString &context,
			const QDBusObjectPath &object) {
		hud::common::Description description;
		description.m_windowId = windowId;
		description.m_context = context;
		description.m_object = object;
		descriptions << description;
	}

	DBusTestRunner dbus;

	DBusMock mock;

	NiceMock<MockFactory> factory;

	QSharedPointer<MockWindowContext> allWindowsContext;
};

TEST_F(TestApplication, DBusInterfaceIsExported) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	ComCanonicalHudApplicationInterface applicationInterface(
			dbus.sessionConnection().baseService(),
			DBusTypes::applicationPath("application-id"),
			dbus.sessionConnection());

	ASSERT_TRUE(applicationInterface.isValid());

	//FIXME desktop path will return something when it's actually implemented
	EXPECT_EQ(QString(), applicationInterface.desktopPath());
}

// window-stack-bridge gives reverse-DNS applications their full desktop id:
// it is a valid object path, and the application finds its desktop file and
// icon by it.
TEST_F(TestApplication, ReverseDnsIdPathAndIcon) {
	EXPECT_EQ("/com/canonical/hud/applications/org_2eexample_2eFoo",
			DBusTypes::applicationPath("org.example.Foo"));

	QTemporaryDir dataDir;
	ASSERT_TRUE(dataDir.isValid());
	ASSERT_TRUE(QDir(dataDir.path()).mkpath("applications"));
	QFile desktopFile(
			QDir(dataDir.path()).filePath("applications/org.example.Foo.desktop"));
	ASSERT_TRUE(desktopFile.open(QIODevice::WriteOnly));
	desktopFile.write("[Desktop Entry]\nType=Application\nName=Foo\nIcon=foo-icon\n");
	desktopFile.close();

	const QByteArray xdgDataDirs(qgetenv("XDG_DATA_DIRS"));
	qputenv("XDG_DATA_DIRS", dataDir.path().toUtf8());

	ApplicationImpl application("org.example.Foo", factory,
			dbus.sessionConnection());
	ComCanonicalHudApplicationInterface applicationInterface(
			dbus.sessionConnection().baseService(),
			DBusTypes::applicationPath("org.example.Foo"),
			dbus.sessionConnection());
	EXPECT_TRUE(applicationInterface.isValid());
	EXPECT_EQ(desktopFile.fileName(), application.desktopPath());
	EXPECT_EQ("foo-icon", application.icon());

	qputenv("XDG_DATA_DIRS", xdgDataDirs);
}

// Sets an environment variable for one test and restores it afterwards,
// unset as unset
class ScopedEnv {
public:
	ScopedEnv(const char *name, const QByteArray &value) :
			m_name(name), m_wasSet(qEnvironmentVariableIsSet(name)), m_old(
					qgetenv(name)) {
		qputenv(name, value);
	}

	ScopedEnv(const char *name) :
			m_name(name), m_wasSet(qEnvironmentVariableIsSet(name)), m_old(
					qgetenv(name)) {
		qunsetenv(name);
	}

	~ScopedEnv() {
		if (m_wasSet) {
			qputenv(m_name, m_old);
		} else {
			qunsetenv(m_name);
		}
	}

private:
	const char *m_name;
	bool m_wasSet;
	QByteArray m_old;
};

static void writeDesktopFile(const QString &dataDir, const QString &id,
		const QString &icon) {
	ASSERT_TRUE(QDir(dataDir).mkpath("applications"));
	QFile file(QDir(dataDir).filePath("applications/" + id + ".desktop"));
	ASSERT_TRUE(file.open(QIODevice::WriteOnly));
	file.write(QString("[Desktop Entry]\nType=Application\nName=%1\nIcon=%2\n").arg(
			id, icon).toUtf8());
}

// A desktop file only in the user's data directory ($XDG_DATA_HOME, e.g.
// ~/.local/share/applications) is found, with its icon.
TEST_F(TestApplication, DesktopFileInDataHome) {
	QTemporaryDir home, dirs;
	ASSERT_TRUE(home.isValid() && dirs.isValid());
	writeDesktopFile(home.path(), "org.example.UserOnly", "user-icon");
	ScopedEnv dataHome("XDG_DATA_HOME", home.path().toUtf8());
	ScopedEnv dataDirs("XDG_DATA_DIRS", dirs.path().toUtf8());

	ApplicationImpl application("org.example.UserOnly", factory,
			dbus.sessionConnection());
	EXPECT_EQ(QDir(home.path()).filePath("applications/org.example.UserOnly.desktop"),
			application.desktopPath());
	EXPECT_EQ("user-icon", application.icon());
}

// The XDG order: a user's desktop file overrides a system one with the same id.
TEST_F(TestApplication, DataHomeOverridesDataDirs) {
	QTemporaryDir home, dirs;
	ASSERT_TRUE(home.isValid() && dirs.isValid());
	writeDesktopFile(home.path(), "org.example.Both", "user-icon");
	writeDesktopFile(dirs.path(), "org.example.Both", "system-icon");
	ScopedEnv dataHome("XDG_DATA_HOME", home.path().toUtf8());
	ScopedEnv dataDirs("XDG_DATA_DIRS", dirs.path().toUtf8());

	ApplicationImpl application("org.example.Both", factory,
			dbus.sessionConnection());
	EXPECT_EQ("user-icon", application.icon());
}

// With XDG_DATA_DIRS unset nothing is looked up relative to the working
// directory (the old code split "" into one empty directory, ".").
TEST_F(TestApplication, NoCwdLookupWithoutDataDirs) {
	QTemporaryDir home, cwd;
	ASSERT_TRUE(home.isValid() && cwd.isValid());
	writeDesktopFile(cwd.path(), "org.example.OnlyInCwd", "cwd-icon");
	ScopedEnv dataHome("XDG_DATA_HOME", home.path().toUtf8());
	ScopedEnv dataDirs("XDG_DATA_DIRS");
	const QString oldCwd(QDir::currentPath());
	ASSERT_TRUE(QDir::setCurrent(cwd.path()));

	ApplicationImpl application("org.example.OnlyInCwd", factory,
			dbus.sessionConnection());
	const QString path(application.desktopPath());
	QDir::setCurrent(oldCwd);
	EXPECT_EQ(QString(), path);
}

TEST_F(TestApplication, AddsWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());
	EXPECT_TRUE(application.isEmpty());

	QSharedPointer<MockWindow> window(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(4567, QString("application-id"), _)).WillOnce(
			Return(window));
	application.addWindow(4567);
	EXPECT_FALSE(application.isEmpty());
}

TEST_F(TestApplication, HandlesDeleteUnknownWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());
	EXPECT_TRUE(application.isEmpty());

	QSharedPointer<MockWindow> window(new NiceMock<MockWindow>());
	application.removeWindow(4567);
	EXPECT_TRUE(application.isEmpty());
}

TEST_F(TestApplication, DeletesWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QSharedPointer<MockWindow> window0(new NiceMock<MockWindow>());
	QSharedPointer<MockWindow> window1(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(0, QString("application-id"), _)).WillOnce(
			Return(window0));
	application.addWindow(0);
	EXPECT_FALSE(application.isEmpty());

	EXPECT_CALL(factory, newWindow(1, QString("application-id"), _)).WillOnce(
			Return(window1));
	application.addWindow(1);
	EXPECT_FALSE(application.isEmpty());

	application.removeWindow(0);
	EXPECT_FALSE(application.isEmpty());

	application.removeWindow(1);
	EXPECT_TRUE(application.isEmpty());
}

TEST_F(TestApplication, AddSourcesToAllWindowsContext) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QList<hud::common::Action> actions;
	addAction(actions, 0, "context1", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 0, "context2", "prefix", QDBusObjectPath("/actions1"));

	QList<Description> descriptions;
	addMenu(descriptions, 0, "context1", QDBusObjectPath("/menu1"));
	addMenu(descriptions, 0, "context2", QDBusObjectPath("/menu2"));

	WindowContext::MenuDefinition menuDefinition1("local");
	menuDefinition1.actionPath = QDBusObjectPath("/actions1");
	menuDefinition1.actionPrefix = "prefix";
	menuDefinition1.menuPath = QDBusObjectPath("/menu1");

	WindowContext::MenuDefinition menuDefinition2("local");
	menuDefinition2.actionPath = QDBusObjectPath("/actions1");
	menuDefinition2.actionPrefix = "prefix";
	menuDefinition2.menuPath = QDBusObjectPath("/menu2");

	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context1"), menuDefinition1));
	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context2"), menuDefinition2));

	application.AddSources(actions, descriptions);
}

TEST_F(TestApplication, AddSourcesToAllWindowsContextAndWindow) {
	ApplicationImpl application("application-id", factory,
			dbus.sessionConnection());

	QSharedPointer<MockWindow> window1(new NiceMock<MockWindow>());

	EXPECT_CALL(factory, newWindow(1, QString("application-id"), _)).WillOnce(
			Return(window1));
	application.addWindow(1);

	QList<hud::common::Action> actions;
	addAction(actions, 0, "context1", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 0, "context2", "prefix", QDBusObjectPath("/actions1"));
	addAction(actions, 1, "context1", "prefix", QDBusObjectPath("/actions2"));

	QList<Description> descriptions;
	addMenu(descriptions, 0, "context1", QDBusObjectPath("/menu1"));
	addMenu(descriptions, 0, "context2", QDBusObjectPath("/menu2"));
	addMenu(descriptions, 1, "context1", QDBusObjectPath("/menu3"));

	WindowContext::MenuDefinition menuDefinition1("local");
	menuDefinition1.actionPath = QDBusObjectPath("/actions1");
	menuDefinition1.actionPrefix = "prefix";
	menuDefinition1.menuPath = QDBusObjectPath("/menu1");

	WindowContext::MenuDefinition menuDefinition2("local");
	menuDefinition2.actionPath = QDBusObjectPath("/actions1");
	menuDefinition2.actionPrefix = "prefix";
	menuDefinition2.menuPath = QDBusObjectPath("/menu2");

	WindowContext::MenuDefinition menuDefinition3("local");
	menuDefinition3.actionPath = QDBusObjectPath("/actions2");
	menuDefinition3.actionPrefix = "prefix";
	menuDefinition3.menuPath = QDBusObjectPath("/menu3");

	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context1"), menuDefinition1));
	EXPECT_CALL(*allWindowsContext,
			addMenu(QString("context2"), menuDefinition2));
	EXPECT_CALL(*window1, addMenu(QString("context1"), menuDefinition3));

	application.AddSources(actions, descriptions);
}

} // namespace
