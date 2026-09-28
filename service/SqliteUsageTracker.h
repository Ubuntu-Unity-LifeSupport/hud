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

#ifndef HUD_SERVICE_SQLITEUSAGETRACKER_H_
#define HUD_SERVICE_SQLITEUSAGETRACKER_H_

#include <service/UsageTracker.h>

#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>

namespace hud {
namespace service {

class SqliteUsageTracker: public UsageTracker {
Q_OBJECT

public:
	SqliteUsageTracker();

	virtual ~SqliteUsageTracker();

	void markUsage(const QString &applicationId, const QString &entry) override;

	unsigned int usage(const QString &applicationId, const QString &entry) const
			override;

protected Q_SLOTS:
	void loadFromDatabase();

protected:
	typedef QPair<QString, QString> UsagePair;

	QMap<UsagePair, unsigned int> m_usage;

	QTimer m_timer;

	QSqlDatabase m_db;

	QScopedPointer<QSqlQuery> m_insert;

	QScopedPointer<QSqlQuery> m_query;

	QScopedPointer<QSqlQuery> m_delete;
};

}
}

#endif /* HUD_SERVICE_SQLITEUSAGETRACKER_H_ */
