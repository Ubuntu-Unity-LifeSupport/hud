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

#ifndef HUD_SERVICE_QUERY_H_
#define HUD_SERVICE_QUERY_H_

#include <service/Result.h>

#include <QDBusContext>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QScopedPointer>
#include <QSharedPointer>

class QueryAdaptor;

namespace hud {
namespace service {

class HudService;

class Q_DECL_EXPORT Query: public QObject {
Q_OBJECT
public:
	typedef QSharedPointer<Query> Ptr;

	enum class EmptyBehaviour {
		SHOW_SUGGESTIONS,
		NO_SUGGESTIONS,
	};

	explicit Query(QObject *parent = 0);

	virtual ~Query();

	Q_PROPERTY(QString AppstackModel READ appstackModel)
	virtual QString appstackModel() const = 0;

	Q_PROPERTY(QString CurrentQuery READ currentQuery)
	virtual QString currentQuery() const = 0;

	Q_PROPERTY(QString ResultsModel READ resultsModel)
	virtual QString resultsModel() const = 0;

	Q_PROPERTY(QStringList ToolbarItems READ toolbarItems)
	virtual QStringList toolbarItems() const = 0;

	virtual const QDBusObjectPath & path() const = 0;

	virtual const QList<Result> & results() const = 0;

public Q_SLOTS:
	virtual int UpdateQuery(const QString &query) = 0;

	virtual void ExecuteCommand(const QDBusVariant &item, uint timestamp) = 0;
};

}
}

#endif /* HUD_SERVICE_QUERY_H_ */
