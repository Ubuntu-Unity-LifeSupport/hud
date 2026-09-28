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
#include <service/HudServiceImpl.h>
#include <unit/service/Mocks.h>

#include <QDebug>
#include <libqtdbustest/DBusTestRunner.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace QtDBusTest;
using namespace hud::common;
using namespace hud::service;
using namespace hud::service::test;

namespace {

class TestHudService: public Test {
protected:
	TestHudService() {
		factory.setSessionBus(dbus.sessionConnection());

		applicationList.reset(new NiceMock<MockApplicationList>());
	}

	virtual ~TestHudService() {
	}

	DBusTestRunner dbus;

	NiceMock<MockFactory> factory;

	QSharedPointer<MockApplicationList> applicationList;
};

TEST_F(TestHudService, OpenCloseQuery) {
	HudServiceImpl hudService(factory, applicationList,
			dbus.sessionConnection());

	QDBusObjectPath queryPath("/path/query0");
	QString resultsModel("com.canonical.hud.results0");
	QString appstackModel("com.canonical.hud.appstack0");
	QSharedPointer<MockQuery> query(new NiceMock<MockQuery>());
	ON_CALL(*query, path()).WillByDefault(ReturnRef(queryPath));
	ON_CALL(*query, resultsModel()).WillByDefault(Return(resultsModel));
	ON_CALL(*query, appstackModel()).WillByDefault(Return(appstackModel));

	EXPECT_CALL(factory, newQuery(QString("query text"), QString("local"), Query::EmptyBehaviour::SHOW_SUGGESTIONS)).Times(
			1).WillOnce(Return(query));

	QString resultsName;
	QString appstackName;
	int modelRevision;

	EXPECT_EQ(queryPath,
			hudService.CreateQuery("query text", resultsName, appstackName,
					modelRevision));
	EXPECT_EQ(resultsModel, resultsName);
	EXPECT_EQ(appstackModel, appstackName);
	EXPECT_EQ(0, modelRevision);

	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath, hudService.openQueries());
	hudService.closeQuery(queryPath);
	EXPECT_EQ(QList<QDBusObjectPath>(), hudService.openQueries());
}

TEST_F(TestHudService, CloseUnknownQuery) {
	HudServiceImpl hudService(factory, applicationList,
			dbus.sessionConnection());

	QDBusObjectPath queryPath("/path/query0");

	EXPECT_EQ(QList<QDBusObjectPath>(), hudService.openQueries());
	hudService.closeQuery(queryPath);
	EXPECT_EQ(QList<QDBusObjectPath>(), hudService.openQueries());
}

TEST_F(TestHudService, CreateMultipleQueries) {
	HudServiceImpl hudService(factory, applicationList,
			dbus.sessionConnection());

	QDBusObjectPath queryPath0("/path/query0");
	QString resultsModel0("com.canonical.hud.results0");
	QString appstackModel0("com.canonical.hud.appstack0");
	QSharedPointer<MockQuery> query0(new NiceMock<MockQuery>());
	ON_CALL(*query0, path()).WillByDefault(ReturnRef(queryPath0));
	ON_CALL(*query0, resultsModel()).WillByDefault(Return(resultsModel0));
	ON_CALL(*query0, appstackModel()).WillByDefault(Return(appstackModel0));

	QDBusObjectPath queryPath1("/path/query1");
	QString resultsModel1("com.canonical.hud.results1");
	QString appstackModel1("com.canonical.hud.appstack1");
	QSharedPointer<MockQuery> query1(new NiceMock<MockQuery>());
	ON_CALL(*query1, path()).WillByDefault(ReturnRef(queryPath1));
	ON_CALL(*query1, resultsModel()).WillByDefault(Return(resultsModel1));
	ON_CALL(*query1, appstackModel()).WillByDefault(Return(appstackModel1));

	EXPECT_CALL(factory, newQuery(QString("query0"), QString("local"), Query::EmptyBehaviour::SHOW_SUGGESTIONS)).Times(
			1).WillOnce(Return(query0));
	EXPECT_CALL(factory, newQuery(QString("query1"), QString("local"), Query::EmptyBehaviour::SHOW_SUGGESTIONS)).Times(
			1).WillOnce(Return(query1));

	int modelRevision;
	QString resultsName;
	QString appstackName;

	EXPECT_EQ(queryPath0,
			hudService.CreateQuery("query0", resultsName, appstackName,
					modelRevision));
	EXPECT_EQ(resultsModel0, resultsName);
	EXPECT_EQ(appstackModel0, appstackName);
	EXPECT_EQ(0, modelRevision);
	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath0, hudService.openQueries());

	EXPECT_EQ(queryPath1,
			hudService.CreateQuery("query1", resultsName, appstackName,
					modelRevision));
	EXPECT_EQ(resultsModel1, resultsName);
	EXPECT_EQ(appstackModel1, appstackName);
	EXPECT_EQ(0, modelRevision);
	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath0 << queryPath1,
			hudService.openQueries());

	hudService.closeQuery(queryPath0);
	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath1, hudService.openQueries());

	hudService.closeQuery(queryPath1);
	EXPECT_EQ(QList<QDBusObjectPath>(), hudService.openQueries());
}

TEST_F(TestHudService, LegacyQuery) {
	QSharedPointer<MockApplication> application(
			new NiceMock<MockApplication>());
	QString icon("app0-icon");
	ON_CALL(*application, icon()).WillByDefault(ReturnRef(icon));

	ON_CALL(*applicationList, focusedApplication()).WillByDefault(
			Return(application));

	HudServiceImpl hudService(factory, applicationList,
			dbus.sessionConnection());

	QDBusObjectPath queryPath("/path/query0");
	QList<Result> results;
	results
			<< Result(1, "command1",
					Result::HighlightList() << Result::Highlight(1, 4)
							<< Result::Highlight(5, 6), "descripton1",
					Result::HighlightList() << Result::Highlight(0, 3),
					"shortcut1", 10, false);
	results
			<< Result(2, "simon & garfunkel",
					Result::HighlightList() << Result::Highlight(4, 8),
					"descripton2", Result::HighlightList(), "shortcut2", 20,
					false);
	QSharedPointer<MockQuery> query(new NiceMock<MockQuery>());
	ON_CALL(*query, path()).WillByDefault(ReturnRef(queryPath));
	ON_CALL(*query, results()).WillByDefault(ReturnRef(results));

	EXPECT_CALL(factory, newQuery(QString("query text"), QString("local"), Query::EmptyBehaviour::NO_SUGGESTIONS)).Times(
			1).WillOnce(Return(query));

	QList<Suggestion> suggestions;
	QDBusVariant querykey;
	EXPECT_EQ("query text",
			hudService.StartQuery("query text", 3, suggestions, querykey));
	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath, hudService.openQueries());
	EXPECT_EQ(queryPath.path(), querykey.variant().toString());
	ASSERT_EQ(2, suggestions.size());
	{
		const Suggestion &suggestion(suggestions.at(0));
		EXPECT_EQ(1, suggestion.m_id);
		EXPECT_EQ(QString("c<b>omm</b>a<b>n</b>d1 (<b>des</b>cripton1)"),
				suggestion.m_description);
		EXPECT_EQ(QString("app0-icon"), suggestion.m_icon);
	}
	{
		const Suggestion &suggestion(suggestions.at(1));
		EXPECT_EQ(2, suggestion.m_id);
		EXPECT_EQ(QString("simo<b>n &amp; </b>garfunkel (descripton2)"),
				suggestion.m_description);
		EXPECT_EQ(QString("app0-icon"), suggestion.m_icon);
	}

	// We don't close legacy queries when the close method is called
	EXPECT_CALL(*query, UpdateQuery(QString())).Times(1);
	hudService.CloseQuery(querykey);
	EXPECT_EQ(QList<QDBusObjectPath>() << queryPath, hudService.openQueries());

	QDBusVariant itemKey(qulonglong(1));
	EXPECT_CALL(*query, ExecuteCommand(itemKey, 12345)).Times(1);
	hudService.ExecuteQuery(itemKey, 12345);
	EXPECT_TRUE(hudService.openQueries().isEmpty());
}

TEST_F(TestHudService, RegisterApplication) {
	QDBusObjectPath path("/foo");

	QSharedPointer<MockApplication> application(
			new NiceMock<MockApplication>());
	ON_CALL(*application, path()).WillByDefault(ReturnRef(path));

	EXPECT_CALL(*applicationList, ensureApplication(QString("app-id"))).WillOnce(
			Return(application));

	HudServiceImpl hudService(factory, applicationList,
			dbus.sessionConnection());

	EXPECT_EQ(path, hudService.RegisterApplication("app-id"));
}

} // namespace
