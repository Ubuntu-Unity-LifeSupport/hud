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

#ifndef HUD_SERVICE_HUDSERVICEIMPL_H_
#define HUD_SERVICE_HUDSERVICEIMPL_H_

#include <common/NameObject.h>
#include <common/Suggestion.h>
#include <service/HudService.h>
#include <service/ApplicationList.h>

#include <QDBusContext>
#include <QDBusVariant>
#include <QMap>
#include <QScopedPointer>
#include <QTimer>

class HudAdaptor;

namespace hud {
namespace service {

class Factory;

class Q_DECL_EXPORT HudServiceImpl: public HudService, protected QDBusContext {
Q_OBJECT

public:
	typedef QSharedPointer<HudService> Ptr;

	HudServiceImpl(Factory &factory, ApplicationList::Ptr applicationList,
			const QDBusConnection &connection, QObject *parent = 0);

	virtual ~HudServiceImpl();

	Q_PROPERTY(QList<hud::common::NameObject> Applications READ applications)
	QList<hud::common::NameObject> applications() const;

	Q_PROPERTY(QList<QDBusObjectPath> OpenQueries READ openQueries)
	QList<QDBusObjectPath> openQueries() const;

	Query::Ptr closeQuery(const QDBusObjectPath &path) override;

public Q_SLOTS:
	QDBusObjectPath RegisterApplication(const QString &id);

	QDBusObjectPath CreateQuery(const QString &query, QString &resultsName,
			QString &appstackName, int &modelRevision);

	/*
	 * Legacy interface below here
	 */

	QString StartQuery(const QString &query, int entries,
			QList<hud::common::Suggestion> &suggestions,
			QDBusVariant &querykey);

	void ExecuteQuery(const QDBusVariant &key, uint timestamp);

	void CloseQuery(const QDBusVariant &querykey);

protected Q_SLOTS:
	void legacyTimeout();

protected:
	Query::Ptr createQuery(const QString &query, const QString &service,
			Query::EmptyBehaviour emptyBehaviour);

	QScopedPointer<HudAdaptor> m_adaptor;

	QDBusConnection m_connection;

	Factory &m_factory;

	QMap<QDBusObjectPath, Query::Ptr> m_queries;

	QMap<QString, QPair<Query::Ptr, QSharedPointer<QTimer>>> m_legacyQueries;

	QSharedPointer<ApplicationList> m_applicationList;

	QString messageSender();
};

}
}

#endif /* HUD_SERVICE_HUDSERVICEIMPL_H_ */
