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

#include <service/SqliteUsageTracker.h>

#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;
using namespace hud::service;

namespace {

class TestUsageTracker: public Test {
protected:
};

TEST_F(TestUsageTracker, BasicBehaviour) {
	SqliteUsageTracker usageTracker;
	usageTracker.markUsage("app-id-1", "entry1");
	usageTracker.markUsage("app-id-1", "entry1");
	usageTracker.markUsage("app-id-1", "entry2");
	usageTracker.markUsage("app-id-1", "entry2");
	usageTracker.markUsage("app-id-1", "entry2");
	usageTracker.markUsage("app-id-1", "entry3");

	usageTracker.markUsage("app-id-2", "entry1");
	usageTracker.markUsage("app-id-2", "entry2");
	usageTracker.markUsage("app-id-2", "entry2");

	EXPECT_EQ(2, usageTracker.usage("app-id-1", "entry1"));
	EXPECT_EQ(3, usageTracker.usage("app-id-1", "entry2"));
	EXPECT_EQ(1, usageTracker.usage("app-id-1", "entry3"));

	EXPECT_EQ(1, usageTracker.usage("app-id-2", "entry1"));
	EXPECT_EQ(2, usageTracker.usage("app-id-2", "entry2"));
}

TEST_F(TestUsageTracker, SavesThingsBetweenRuns) {
	QTemporaryDir temporaryDir;
	ASSERT_TRUE(temporaryDir.isValid());
	qputenv("HUD_STORE_USAGE_DATA", "TRUE");
	qputenv("HUD_CACHE_DIR", temporaryDir.path().toUtf8());

	{
		SqliteUsageTracker usageTracker;
		usageTracker.markUsage("app-id-1", "entry1");
		usageTracker.markUsage("app-id-1", "entry1");
		usageTracker.markUsage("app-id-1", "entry2");
		usageTracker.markUsage("app-id-1", "entry2");
		usageTracker.markUsage("app-id-1", "entry2");
		usageTracker.markUsage("app-id-1", "entry3");

		usageTracker.markUsage("app-id-2", "entry1");
		usageTracker.markUsage("app-id-2", "entry2");
		usageTracker.markUsage("app-id-2", "entry2");

		EXPECT_EQ(2, usageTracker.usage("app-id-1", "entry1"));
		EXPECT_EQ(3, usageTracker.usage("app-id-1", "entry2"));
		EXPECT_EQ(1, usageTracker.usage("app-id-1", "entry3"));

		EXPECT_EQ(1, usageTracker.usage("app-id-2", "entry1"));
		EXPECT_EQ(2, usageTracker.usage("app-id-2", "entry2"));
	}

	{
		SqliteUsageTracker usageTracker;

		EXPECT_EQ(2, usageTracker.usage("app-id-1", "entry1"));
		EXPECT_EQ(3, usageTracker.usage("app-id-1", "entry2"));
		EXPECT_EQ(1, usageTracker.usage("app-id-1", "entry3"));

		EXPECT_EQ(1, usageTracker.usage("app-id-2", "entry1"));
		EXPECT_EQ(2, usageTracker.usage("app-id-2", "entry2"));
	}

	qputenv("HUD_STORE_USAGE_DATA", "FALSE");
}

} // namespace
