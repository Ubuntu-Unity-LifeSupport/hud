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
#include <service/QueryImpl.h>
#include <unit/service/Mocks.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbusmock/DBusMock.h>
#include <QDBusMessage>
#include <QSignalSpy>
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

class TestQuery: public QObject, public Test {
Q_OBJECT

protected:
	TestQuery() :
			mock(dbus) {

		windowToken.reset(new NiceMock<MockWindowToken>());
		ON_CALL(*windowToken, toolbarItems()).WillByDefault(
				Return(QStringList()));
		ON_CALL(*windowToken, commands()).WillByDefault(
				Return(
						QList<QStringList>()
								<< (QStringList() << "command1" << "command2")));

		window.reset(new NiceMock<MockWindow>());
		ON_CALL(*window, activate()).WillByDefault(Return(windowToken));

		application.reset(new NiceMock<MockApplication>());
		appId = "app-id";
		appIcon = "app-icon";
		ON_CALL(*application, id()).WillByDefault(ReturnRef(appId));
		ON_CALL(*application, icon()).WillByDefault(ReturnRef(appIcon));

		applicationList.reset(new NiceMock<MockApplicationList>());
		ON_CALL(*applicationList, focusedApplication()).WillByDefault(
				Return(application));
		ON_CALL(*applicationList, focusedWindow()).WillByDefault(
				Return(window));

		voice.reset(new NiceMock<MockVoice>());

		hudService.reset(new NiceMock<MockHudService>);
	}

	virtual ~TestQuery() {
	}

Q_SIGNALS:
	void queryClosed(const QDBusObjectPath &path);

protected:
	DBusTestRunner dbus;

	DBusMock mock;

	QString appId;

	QString appIcon;

	QSharedPointer<MockHudService> hudService;

	QSharedPointer<MockVoice> voice;

	QSharedPointer<MockApplicationList> applicationList;

	QSharedPointer<MockApplication> application;

	QSharedPointer<MockWindow> window;

	QSharedPointer<MockWindowToken> windowToken;
};

TEST_F(TestQuery, Create) {
	QList<Result> expectedResults;
	expectedResults
			<< Result(0, "command name",
					Result::HighlightList() << Result::Highlight(0, 1),
					"description field",
					Result::HighlightList() << Result::Highlight(2, 3),
					"shortcut field", 100, false);

	QString queryString("query");

	EXPECT_CALL(*window, activate()).WillOnce(Return(windowToken));

	EXPECT_CALL(*windowToken, search(queryString, Query::EmptyBehaviour::SHOW_SUGGESTIONS, _)).WillOnce(
			Invoke(
					[&expectedResults](const QString &, Query::EmptyBehaviour, QList<Result> &results) {
				results.append(expectedResults);
			}));

	QueryImpl query(0, queryString, "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	const QList<Result> results(query.results());
	ASSERT_EQ(expectedResults.size(), results.size());
	ASSERT_EQ(expectedResults.at(0).id(), results.at(0).id());
	ASSERT_EQ(expectedResults.at(0).commandName(), results.at(0).commandName());
	ASSERT_EQ(expectedResults.at(0).description(), results.at(0).description());
}

TEST_F(TestQuery, ExecuteCommand) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*windowToken, execute(123));
	query.ExecuteCommand(QDBusVariant(123), 12345);
}

/*
 * UNITY-20261009-003. Unity 7's ExecuteQuery reaches ExecuteCommand directly,
 * through HudServiceImpl, not over D-Bus on the query object: an error reply
 * there has no message to answer.
 */
TEST_F(TestQuery, ExecuteCommandWithStructureKeyOutsideDBus) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*windowToken, execute(_)).Times(0);
	query.ExecuteCommand(
			QDBusVariant(QStringList() << "not" << "a-number"), 12345);
}

TEST_F(TestQuery, ExecuteCommandWithNonNumericStringKey) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	// Qt 5 converts any string to qlonglong, "not-a-number" to 0
	EXPECT_CALL(*windowToken, execute(_)).Times(0);
	query.ExecuteCommand(QDBusVariant(QString("not-a-number")), 12345);
}

TEST_F(TestQuery, ExecuteCommandWithNumericStringKey) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*windowToken, execute(7));
	query.ExecuteCommand(QDBusVariant(QString("7")), 12345);
}

TEST_F(TestQuery, ExecuteCommandWithoutFocusedWindow) {
	// The constructor already refreshes, so there is never a window token
	ON_CALL(*applicationList, focusedWindow()).WillByDefault(
			Return(Window::Ptr()));
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*windowToken, execute(_)).Times(0);
	query.ExecuteCommand(QDBusVariant(qulonglong(0)), 12345);
}

TEST_F(TestQuery, ExecuteCommandWithBadKeyOverDBus) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*windowToken, execute(_)).Times(0);
	QDBusMessage call(QDBusMessage::createMethodCall(
			dbus.sessionConnection().baseService(), query.path().path(),
			"com.canonical.hud.query", "ExecuteCommand"));
	call << QVariant::fromValue(
			QDBusVariant(QStringList() << "not" << "a-number")) << uint(12345);
	QDBusMessage reply(dbus.sessionConnection().call(call));
	EXPECT_EQ(QDBusMessage::ErrorMessage, reply.type());
	EXPECT_EQ(QString("org.freedesktop.DBus.Error.InvalidArgs"),
			reply.errorName());
}

TEST_F(TestQuery, ExecuteParameterizedWithBadKeys) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	QString prefix, baseAction;
	QDBusObjectPath actionPath, modelPath;
	int modelSection = 0;
	EXPECT_CALL(*windowToken, executeParameterized(_, _, _, _, _)).Times(0);
	query.ExecuteParameterized(
			QDBusVariant(QStringList() << "not" << "a-number"), 12345,
			prefix, baseAction, actionPath, modelPath, modelSection);
	query.ExecuteParameterized(QDBusVariant(QString("not-a-number")), 12345,
			prefix, baseAction, actionPath, modelPath, modelSection);
}

TEST_F(TestQuery, ExecuteParameterizedWithoutFocusedWindow) {
	ON_CALL(*applicationList, focusedWindow()).WillByDefault(
			Return(Window::Ptr()));
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	QString prefix, baseAction;
	QDBusObjectPath actionPath, modelPath;
	int modelSection = 0;
	EXPECT_CALL(*windowToken, executeParameterized(_, _, _, _, _)).Times(0);
	EXPECT_EQ(QString(), query.ExecuteParameterized(
			QDBusVariant(qulonglong(0)), 12345, prefix, baseAction,
			actionPath, modelPath, modelSection));
}

TEST_F(TestQuery, CloseWhenSenderDies) {
	// a random dbus service that we're going to tell the query to watch
	QScopedPointer<QProcessDBusService> keepAliveService(
			new QProcessDBusService("keep.alive", QDBusConnection::SessionBus,
			MODEL_SIMPLE, QStringList() << "keep.alive" << "/"));
	keepAliveService->start(dbus.sessionConnection());

	Query::Ptr query(
			new QueryImpl(0, "query", "keep.alive",
					Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
					applicationList, voice, dbus.sessionConnection()));

	EXPECT_CALL(*hudService, closeQuery(query->path())).WillOnce(
			Invoke([this, query](const QDBusObjectPath &path) {
				this->queryClosed(path);
				return query;
			}));

	QSignalSpy queryClosedSpy(this,
			SIGNAL(queryClosed(const QDBusObjectPath &)));

	// kill the service the query should be watching
	keepAliveService.reset();

	// wait for the query to request close
	queryClosedSpy.wait();

	// check that it tried to closed itself using the HudService interface
	ASSERT_EQ(1, queryClosedSpy.size());
	EXPECT_EQ(query->path(),
			queryClosedSpy.at(0).at(0).value<QDBusObjectPath>());
}

TEST_F(TestQuery, VoiceQuery) {
	QueryImpl query(0, "query", "keep.alive",
			Query::EmptyBehaviour::SHOW_SUGGESTIONS, *hudService,
			applicationList, voice, dbus.sessionConnection());

	EXPECT_CALL(*voice, listen(QList<QStringList>()
					<< (QStringList() << "command1" << "command2"))).WillOnce(
			Return("voice query"));

	// call VoiceQuery while MockWindow appears focused
	QString voiceQuery;
	query.VoiceQuery(voiceQuery);
	EXPECT_EQ("voice query", voiceQuery.toStdString());

	// return null when focusedWindow() is requested
	EXPECT_CALL(*applicationList, focusedWindow()).WillOnce(
			Return(Window::Ptr()));

	// call VoiceQuery again with no window focused
	voiceQuery = "";
	query.VoiceQuery(voiceQuery);
	EXPECT_EQ("", voiceQuery.toStdString());
}

} // namespace

#include "TestQuery.moc"
