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

#include <service/ItemStore.h>
#include <service/HardCodedSearchSettings.h>
#include <tests/unit/service/Mocks.h>

#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace hud::service;
using namespace hud::service::test;

namespace {

class TestItemStore: public Test {
protected:
	TestItemStore() {
		usageTracker.reset(new NiceMock<MockUsageTracker>());

		searchSettings.reset(new HardCodedSearchSettings());

		store.reset(new ItemStore("app-id", usageTracker, searchSettings));
	}

	/* Test a set of strings */
	string search(const QString &query) {
		QList<Result> results;
		store->search(query, Query::EmptyBehaviour::SHOW_SUGGESTIONS, results);

		QString result;

		if (!results.isEmpty()) {
			result = results.at(0).commandName();
		}

		return result.toStdString();
	}

	ItemStore::Ptr store;

	QSharedPointer<MockUsageTracker> usageTracker;

	QSharedPointer<HardCodedSearchSettings> searchSettings;
};

/* Ensure the base calculation works */
TEST_F(TestItemStore, DistanceSubfunction) {
	QMenu root;

	QMenu file("File");
	file.addAction("Open");
	file.addAction("New");
	file.addAction("Print");
	file.addAction("Print Preview");
	root.addMenu(&file);

	store->indexMenu(&root);

	EXPECT_EQ("Print Preview", search("Print Pre"));
}

/* Ensure that we can handle some misspelling */
TEST_F(TestItemStore, DistanceMisspelll) {
	QMenu root;

	QMenu file("File");
	file.addAction("Open");
	file.addAction("New");
	file.addAction("Print");
	file.addAction("Print Preview");
	root.addMenu(&file);

	store->indexMenu(&root);

	EXPECT_EQ("Print Preview", search("Prnt Pr"));
	EXPECT_EQ("Print Preview", search("Print Preiw"));
	EXPECT_EQ("Print Preview", search("Prnt Pr"));
}

/* Ensure that we can find print with short strings */
TEST_F(TestItemStore, DistancePrintIssues) {
	QMenu root;

	QMenu file("File");
	file.addAction("New");
	file.addAction("Open");
	file.addAction("Print...");
	root.addMenu(&file);

	QMenu edit("Edit");
	edit.addAction("Undo");
	root.addMenu(&edit);

	QMenu help("Help");
	help.addAction("About");
	help.addAction("Empty");
	root.addMenu(&help);

	store->indexMenu(&root);

	EXPECT_EQ("Print...", search("Pr"));
	EXPECT_EQ("Print...", search("Print"));
	EXPECT_EQ("Print...", search("Print..."));
}

/* Not finished word yet */
TEST_F(TestItemStore, UnfinishedWord) {
	QMenu root;
	root.addAction("Open Terminal");
	root.addAction("Open Tab");
	store->indexMenu(&root);

	EXPECT_EQ("Open Terminal", search("open ter"));
	EXPECT_EQ("Open Terminal", search("open term"));
	EXPECT_EQ("Open Terminal", search("open termi"));
	EXPECT_EQ("Open Terminal", search("open termin"));
	EXPECT_EQ("Open Terminal", search("open termina"));
	EXPECT_EQ("Open Terminal", search("open terminal"));
}

/* Not finished word yet */
TEST_F(TestItemStore, UnfinishedWord2) {
	QMenu root;
	root.addAction("Change Topic");
	store->indexMenu(&root);

	EXPECT_EQ("Change Topic", search("cha"));
}

/* A variety of strings that should have predictable results */
TEST_F(TestItemStore, DistanceVariety) {
	QMenu root;

	QMenu date("Date");
	date.addAction("House Cleaning");
	root.addMenu(&date);

	QMenu file("File");
	file.addAction("Close Window");
	root.addMenu(&file);

	QMenu edit("Edit");
	edit.addAction("Keyboard Shortcuts...");
	root.addMenu(&edit);

	QMenu vpn("VPN Configuration");
	vpn.addAction("Configure VPN...");
	QMenu network("Network");
	network.addMenu(&vpn);
	root.addMenu(&network);

	store->indexMenu(&root);

	EXPECT_EQ("House Cleaning", search("House"));
	EXPECT_EQ("House Cleaning", search("House C"));
	EXPECT_EQ("House Cleaning", search("House Cle"));
	EXPECT_EQ("House Cleaning", search("House Clean"));
	EXPECT_EQ("House Cleaning", search("Clean House"));
}

/* A variety of strings that should have predictable results */
TEST_F(TestItemStore, DistanceFrenchPref) {
	QMenu root;

	QMenu file("Fichier");
	file.addAction("aperçu avant impression");
	root.addMenu(&file);

	QMenu connection("Connexion au réseau...");
	root.addMenu(&connection);

	QMenu edit("Edition");
	edit.addAction("préférences");
	root.addMenu(&edit);

	store->indexMenu(&root);

	EXPECT_EQ("préférences", search("préférences"));
	EXPECT_EQ("préférences", search("pré"));
	EXPECT_EQ("préférences", search("préf"));
	EXPECT_EQ("préférences", search("préfé"));
	EXPECT_EQ("préférences", search("pref"));
}

/* Check to make sure the returned hits are not dups and the
 proper number */
TEST_F(TestItemStore, DistanceDups) {
	QMenu root;
	root.addAction("Inflated");
	root.addAction("Confluated");
	root.addAction("Sublimated");
	root.addAction("Sadated");
	root.addAction("Situated");
	root.addAction("Infatuated");

	store->indexMenu(&root);

	EXPECT_EQ("Inflated", search("ted inf"));
}

/* Check to make sure 'Save' matches better than 'Save As...' for "save" */
TEST_F(TestItemStore, DistanceExtraTerms) {
	QMenu root;

	QMenu file("File");
	file.addAction("Banana");
	file.addAction("Save All");
	file.addAction("Save");
	file.addAction("Save As...");
	file.addAction("Apple");
	root.addMenu(&file);

	store->indexMenu(&root);

	EXPECT_EQ("Save", search("save"));
}

TEST_F(TestItemStore, BlankSearchFrequentlyUsedItems) {
	QMenu root;

	QMenu file("&File");
	file.addAction("&One");
	file.addAction("&Two");
	file.addAction("T&hree");
	file.addAction("Fou&r");
	root.addMenu(&file);

	store->indexMenu(&root);

	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||One"))).WillByDefault(
			Return(2));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Two"))).WillByDefault(
			Return(0));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Three"))).WillByDefault(
			Return(4));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Four"))).WillByDefault(
			Return(3));

	QList<Result> results;
	store->search("", Query::EmptyBehaviour::SHOW_SUGGESTIONS, results);
	ASSERT_EQ(4, results.size());
	EXPECT_EQ(QString("Three"), results.at(0).commandName());
	EXPECT_EQ(QString("Four"), results.at(1).commandName());
	EXPECT_EQ(QString("One"), results.at(2).commandName());
	EXPECT_EQ(QString("Two"), results.at(3).commandName());
}

TEST_F(TestItemStore, BlankSearchNoSuggestions) {
	QMenu root;

	QMenu file("&File");
	file.addAction("&One");
	file.addAction("&Two");
	file.addAction("T&hree");
	file.addAction("Fou&r");
	root.addMenu(&file);

	store->indexMenu(&root);

	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||One"))).WillByDefault(
			Return(2));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Two"))).WillByDefault(
			Return(0));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Three"))).WillByDefault(
			Return(4));
	ON_CALL(*usageTracker,
			usage(QString("app-id"), QString("File||Four"))).WillByDefault(
			Return(3));

	QList<Result> results;
	store->search("", Query::EmptyBehaviour::NO_SUGGESTIONS, results);
	ASSERT_TRUE(results.empty());
}

TEST_F(TestItemStore, ExecuteMarksHistory) {
	QMenu root;

	QMenu file("File");
	file.addAction("Save As...");
	file.addAction("Save");
	root.addMenu(&file);

	store->indexMenu(&root);

	EXPECT_CALL(*usageTracker,
			markUsage(QString("app-id"), QString("File||Save As...")));
	store->execute(0);
}

TEST_F(TestItemStore, ChangeSearchSettings) {
	QMenu root;

	QMenu file("&File");
	file.addAction("Apple");
	file.addAction("Banana");
	file.addAction("Can Cherry");
	root.addMenu(&file);

	store->indexMenu(&root);

	EXPECT_EQ("Banana", search("Ban"));

	searchSettings->setEndDropPenalty(100);

	EXPECT_EQ("Can Cherry", search("Ban"));
}

TEST_F(TestItemStore, DeletedActions) {
	QMenu root;

	QMenu file("&File");
	file.addAction("Apple");
	file.addAction("Banana");
	file.addAction("Can Cherry");
	root.addMenu(&file);

	store->indexMenu(&root);

	file.clear();
	root.clear();

	// It should not crash! :)
	EXPECT_EQ("", search("flibble"));

	// It should not crash! :)
	EXPECT_EQ("", search(""));
}

} // namespace
