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

#include <service/Factory.h>
#include <service/WindowImpl.h>
#include <unit/service/Mocks.h>

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

class TestWindow: public Test {
protected:
	TestWindow() :
			mock(dbus) {
		factory.setSessionBus(dbus.sessionConnection());

		usageTracker.reset(new NiceMock<MockUsageTracker>());
		ON_CALL(factory, singletonUsageTracker()).WillByDefault(
				Return(usageTracker));

		allWindowsContext.reset(new NiceMock<MockWindowContext>());
		ON_CALL(*allWindowsContext, activeCollector()).WillByDefault(
				Return(Collector::Ptr()));

		allWindowsCollector.reset(new NiceMock<MockCollector>());
		ON_CALL(*allWindowsCollector, isValid()).WillByDefault(Return(true));

		dbusmenuWindowCollector.reset(new NiceMock<MockCollector>());
		ON_CALL(*dbusmenuWindowCollector, isValid()).WillByDefault(Return(true));

		gmenuWindowCollector.reset(new NiceMock<MockCollector>());
		ON_CALL(*gmenuWindowCollector, isValid()).WillByDefault(Return(true));

		windowCollector.reset(new NiceMock<MockCollector>());
		ON_CALL(*windowCollector, isValid()).WillByDefault(Return(true));
	}

	virtual ~TestWindow() {
	}

	Window::Ptr createWindow() {
		EXPECT_CALL(factory, newDBusMenuWindowCollector(1234)).Times(
				1).WillOnce(Return(dbusmenuWindowCollector));
		EXPECT_CALL(factory, newGMenuWindowCollector(1234, QString("application-id"))).Times(
				1).WillOnce(Return(gmenuWindowCollector));

		return Window::Ptr(
				new WindowImpl(1234, "application-id", allWindowsContext,
						factory));
	}

	DBusTestRunner dbus;

	DBusMock mock;

	NiceMock<MockFactory> factory;

	QSharedPointer<MockUsageTracker> usageTracker;

	QSharedPointer<MockWindowContext> allWindowsContext;

	shared_ptr<MockCollector> allWindowsCollector;

	shared_ptr<MockCollector> dbusmenuWindowCollector;

	shared_ptr<MockCollector> gmenuWindowCollector;

	shared_ptr<MockCollector> windowCollector;
};

TEST_F(TestWindow, TrysDBusMenuAndGMenu) {
	createWindow();
}

TEST_F(TestWindow, ActivateOnlyWithDBusMenu) {
	Window::Ptr window(createWindow());

	ON_CALL(*gmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*allWindowsCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*windowCollector, isValid()).WillByDefault(Return(false));

	QMenu dbusmenuWindowCollectorMenu;
	CollectorToken::Ptr dbusmenuWindowCollectorToken(
			new CollectorToken(dbusmenuWindowCollector,
					&dbusmenuWindowCollectorMenu));
	EXPECT_CALL(*dbusmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken));

	WindowToken::Ptr token(window->activate());
	EXPECT_EQ(QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken,
			token->tokens());
}

TEST_F(TestWindow, ActivateOnlyWithGMenu) {
	Window::Ptr window(createWindow());

	ON_CALL(*dbusmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*allWindowsCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*windowCollector, isValid()).WillByDefault(Return(false));

	QMenu gmenuWindowCollectorMenu;
	CollectorToken::Ptr gmenuWindowCollectorToken(
			new CollectorToken(gmenuWindowCollector,
					&gmenuWindowCollectorMenu));
	EXPECT_CALL(*gmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << gmenuWindowCollectorToken));

	WindowToken::Ptr token(window->activate());
	EXPECT_EQ(QList<CollectorToken::Ptr>() << gmenuWindowCollectorToken,
			token->tokens());
}

TEST_F(TestWindow, ActivateOnlyWithValidAllWindowsContext) {
	Window::Ptr window(createWindow());

	ON_CALL(*dbusmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*gmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*windowCollector, isValid()).WillByDefault(Return(false));

	QMenu allWindowsCollectorMenu;
	CollectorToken::Ptr allWindowsCollectorToken(
			new CollectorToken(allWindowsCollector, &allWindowsCollectorMenu));
	EXPECT_CALL(*allWindowsCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << allWindowsCollectorToken));

	ON_CALL(*allWindowsContext, activeCollector()).WillByDefault(
			Return(allWindowsCollector));

	WindowToken::Ptr token(window->activate());
	EXPECT_EQ(QList<CollectorToken::Ptr>() << allWindowsCollectorToken,
			token->tokens());
}

TEST_F(TestWindow, ActivateOnlyWithValidWindowContext) {
	Window::Ptr window(createWindow());

	ON_CALL(*dbusmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*gmenuWindowCollector, isValid()).WillByDefault(Return(false));
	ON_CALL(*allWindowsCollector, isValid()).WillByDefault(Return(false));

	WindowContext::MenuDefinition definition("bus.name");
	definition.actionPath = QDBusObjectPath("/action/path");
	definition.actionPrefix = "hud";
	definition.menuPath = QDBusObjectPath("/menu/path");

	QMap<QString, QDBusObjectPath> actions;
	actions[definition.actionPrefix] = definition.actionPath;

	EXPECT_CALL(factory, newGMenuCollector(definition.name, actions, definition.menuPath)).Times(
			1).WillOnce(Return(windowCollector));

	window->addMenu("context_1", definition);
	window->setContext("context_1");

	QMenu windowCollectorMenu;
	CollectorToken::Ptr windowCollectorToken(
			new CollectorToken(windowCollector, &windowCollectorMenu));
	EXPECT_CALL(*windowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << windowCollectorToken));

	WindowToken::Ptr token(window->activate());
	EXPECT_EQ(QList<CollectorToken::Ptr>() << windowCollectorToken,
			token->tokens());
}

TEST_F(TestWindow, ActivateWithMultipleCollectorsThenChange) {
	Window::Ptr window(createWindow());

	ON_CALL(*windowCollector, isValid()).WillByDefault(Return(false));

	QMenu gmenuWindowCollectorMenu;
	CollectorToken::Ptr gmenuWindowCollectorToken(
			new CollectorToken(gmenuWindowCollector,
					&gmenuWindowCollectorMenu));
	EXPECT_CALL(*gmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << gmenuWindowCollectorToken));

	QMenu dbusmenuWindowCollectorMenu;
	CollectorToken::Ptr dbusmenuWindowCollectorToken(
			new CollectorToken(dbusmenuWindowCollector,
					&dbusmenuWindowCollectorMenu));
	EXPECT_CALL(*dbusmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken));

	QMenu allWindowsCollectorMenu;
	CollectorToken::Ptr allWindowsCollectorToken(
			new CollectorToken(allWindowsCollector, &allWindowsCollectorMenu));
	EXPECT_CALL(*allWindowsCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << allWindowsCollectorToken));
	ON_CALL(*allWindowsContext, activeCollector()).WillByDefault(
			Return(allWindowsCollector));

	WindowToken::Ptr token(window->activate());
	EXPECT_EQ(
			QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken
					<< gmenuWindowCollectorToken << allWindowsCollectorToken,
			token->tokens());

	// The gmenu window collector is going to give a different token now
	QMenu gmenuWindowCollectorMenuChanged;
	CollectorToken::Ptr gmenuWindowCollectorTokenChanged(
			new CollectorToken(gmenuWindowCollector,
					&gmenuWindowCollectorMenuChanged));
	EXPECT_CALL(*gmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << gmenuWindowCollectorTokenChanged));

	// Re-prime the other collectors
	EXPECT_CALL(*dbusmenuWindowCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken));
	EXPECT_CALL(*allWindowsCollector, activate()).Times(1).WillOnce(
			Return(QList<CollectorToken::Ptr>() << allWindowsCollectorToken));

	WindowToken::Ptr tokenChanged(window->activate());
	EXPECT_NE(token, tokenChanged);

	EXPECT_EQ(
			QList<CollectorToken::Ptr>() << dbusmenuWindowCollectorToken
					<< gmenuWindowCollectorTokenChanged
					<< allWindowsCollectorToken, tokenChanged->tokens());
}

TEST_F(TestWindow, Context) {
	WindowContextImpl context(factory);

	WindowContext::MenuDefinition definition("bus.name");
	definition.actionPath = QDBusObjectPath("/action/path");
	definition.actionPrefix = "hud";
	definition.menuPath = QDBusObjectPath("/menu/path");

	QMap<QString, QDBusObjectPath> actions;
	actions[definition.actionPrefix] = definition.actionPath;

	EXPECT_CALL(factory, newGMenuCollector(definition.name, actions, definition.menuPath)).Times(
			1).WillOnce(Return(windowCollector));

	context.addMenu("context_1", definition);

	EXPECT_FALSE(context.activeCollector());

	context.setContext("context_1");

	EXPECT_EQ(windowCollector, context.activeCollector());
}

} // namespace
