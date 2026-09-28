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

#include <service/SqliteUsageTracker.h>

#include <QDir>
#include <QVariant>
#include <QGSettings/qgsettings.h>

using namespace hud::service;

/*
 * One day in milliseconds
 */
static const int ONE_DAY = 86400000;

SqliteUsageTracker::SqliteUsageTracker() {
	// once each day, clear out the old database entries
	m_timer.setTimerType(Qt::VeryCoarseTimer);
	m_timer.setInterval(ONE_DAY);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(loadFromDatabase()));
	m_timer.start();

	// Should we store user history
	bool storeHistory(false);
	if (qEnvironmentVariableIsEmpty("HUD_STORE_USAGE_DATA")) {
		QGSettings settings("com.canonical.indicator.appmenu.hud",
				"/com/canonical/indicator/appmenu/hud/");
		storeHistory = settings.get("storeUsageData").toBool();
	} else {
		storeHistory = qgetenv("HUD_STORE_USAGE_DATA") == "TRUE";
	}

	if (QSqlDatabase::contains("usage-tracker")) {
		m_db = QSqlDatabase::database("usage-tracker");
	} else {
		m_db = QSqlDatabase::addDatabase("QSQLITE", "usage-tracker");
	}

	if (storeHistory) {
		QDir cacheDirectory;
		if (qEnvironmentVariableIsSet("HUD_CACHE_DIR")) {
			cacheDirectory = qgetenv("HUD_CACHE_DIR");
		} else {
			cacheDirectory = QDir::home().filePath(".cache");
		}
		cacheDirectory.mkpath("indicator-appmenu");
		QDir appmenuIndicatorDirectory(
				cacheDirectory.filePath("indicator-appmenu"));
		m_db.setDatabaseName(
				appmenuIndicatorDirectory.filePath("hud-usage-log.sqlite"));
	} else {
		m_db.setDatabaseName(":memory:");
	}

	m_db.open();

	// it's important to construct these against the newly open database
	m_insert.reset(new QSqlQuery(m_db));
	m_query.reset(new QSqlQuery(m_db));
	m_delete.reset(new QSqlQuery(m_db));

	// Create the database schema if it doesn't exist
	QSqlQuery create(m_db);
	create.prepare(
			"create table if not exists usage (application text, entry text, timestamp datetime)");
	create.exec();
	create.prepare(
			"create index if not exists application_index on usage (application, entry)");
	create.exec();

	// Prepare our SQL statements
	m_insert->prepare(
			"insert into usage (application, entry, timestamp) values (?, ?, date('now', 'utc'))");
	m_query->prepare(
			"select application, entry, count(*) from usage where timestamp > date('now', 'utc', '-30 days') group by application, entry");
	m_delete->prepare(
			"delete from usage where timestamp < date('now', 'utc', '-30 days')");

	loadFromDatabase();
}

SqliteUsageTracker::~SqliteUsageTracker() {
	m_db.close();
}

void SqliteUsageTracker::loadFromDatabase() {
	// Clear our in-memory cache
	m_usage.clear();

	// Delete entries older than 30 days
	m_delete->exec();

	m_query->exec();
	while (m_query->next()) {
		UsagePair pair(m_query->value(0).toString(),
				m_query->value(1).toString());
		m_usage[pair] = m_query->value(2).toInt();
	}
}

void SqliteUsageTracker::markUsage(const QString &applicationId,
		const QString &entry) {
	UsagePair pair(applicationId, entry);

	if (m_usage.contains(pair)) {
		// increment if we have an existing entry
		++(*m_usage.find(pair));
	} else {
		// just one usage otherwise
		m_usage[pair] = 1;
	}

	// write out the data to sqlite
	m_insert->bindValue(0, applicationId);
	m_insert->bindValue(1, entry);
	m_insert->exec();
}

unsigned int SqliteUsageTracker::usage(const QString &applicationId,
		const QString &entry) const {
	return m_usage[UsagePair(applicationId, entry)];
}
