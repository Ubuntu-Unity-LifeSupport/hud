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

#include <service/Factory.h>
#include <service/HudServiceImpl.h>
#include <service/HudAdaptor.h>
#include <common/Localisation.h>
#include <common/DBusTypes.h>

using namespace hud::common;
using namespace hud::service;

HudServiceImpl::HudServiceImpl(Factory &factory, ApplicationList::Ptr applicationList,
		const QDBusConnection &connection, QObject *parent) :
		HudService(parent), m_adaptor(new HudAdaptor(this)), m_connection(
				connection), m_factory(factory), m_applicationList(
				applicationList) {
	if (!m_connection.registerObject(DBusTypes::HUD_SERVICE_DBUS_PATH, this)) {
		throw std::logic_error(_("Unable to register HUD object on DBus"));
	}
	if (!m_connection.registerService(DBusTypes::HUD_SERVICE_DBUS_NAME)) {
		throw std::logic_error(_("Unable to register HUD service on DBus"));
	}
}

HudServiceImpl::~HudServiceImpl() {
	m_connection.unregisterObject(DBusTypes::HUD_SERVICE_DBUS_PATH);
}

QDBusObjectPath HudServiceImpl::RegisterApplication(const QString &id) {
	Application::Ptr application(m_applicationList->ensureApplication(id));

	if(application.isNull()) {
		sendErrorReply(QDBusError::InvalidArgs, "Invalid application ID");
		return QDBusObjectPath();
	}

	return application->path();
}

QList<NameObject> HudServiceImpl::applications() const {
	return m_applicationList->applications();
}

QList<QDBusObjectPath> HudServiceImpl::openQueries() const {
	return m_queries.keys();
}

Query::Ptr HudServiceImpl::createQuery(const QString &query,
		const QString &sender, Query::EmptyBehaviour emptyBehaviour) {
	Query::Ptr hudQuery(m_factory.newQuery(query, sender, emptyBehaviour));
	m_queries[hudQuery->path()] = hudQuery;

	return hudQuery;
}

QDBusObjectPath HudServiceImpl::CreateQuery(const QString &query,
		QString &resultsName, QString &appstackName, int &modelRevision) {
	QString sender(messageSender());

	Query::Ptr hudQuery(
			createQuery(query, sender,
					Query::EmptyBehaviour::SHOW_SUGGESTIONS));

	resultsName = hudQuery->resultsModel();
	appstackName = hudQuery->appstackModel();
	modelRevision = 0;

	return hudQuery->path();
}

Query::Ptr HudServiceImpl::closeQuery(const QDBusObjectPath &path) {
	return m_queries.take(path);
}

QString HudServiceImpl::messageSender() {
	QString sender("local");
	if (calledFromDBus()) {
		sender = message().service();
	}
	return sender;
}

/*
 * Legacy interface below here
 */

QString HudServiceImpl::StartQuery(const QString &queryString, int entries,
		QList<Suggestion> &suggestions, QDBusVariant &querykey) {
	QString sender(messageSender());

	QPair<Query::Ptr, QSharedPointer<QTimer>> entry(m_legacyQueries[sender]);
	Query::Ptr query(entry.first);
	QSharedPointer<QTimer> legacyTimeout(entry.second);
	if (query.isNull()) {
		query = createQuery(queryString, sender,
				Query::EmptyBehaviour::NO_SUGGESTIONS);

		legacyTimeout.reset(new QTimer());
		legacyTimeout->setInterval(2000);
		legacyTimeout->setSingleShot(true);
		connect(legacyTimeout.data(), SIGNAL(timeout()), this,
				SLOT(legacyTimeout()));

		m_legacyQueries[sender] = qMakePair(query, legacyTimeout);
	} else {
		query->UpdateQuery(queryString);
		legacyTimeout->stop();
		legacyTimeout->setProperty("sender", QVariant());
	}

	// The legacy API only allows you to search the current application
	Application::Ptr application(m_applicationList->focusedApplication());
	QString icon;
	if (!application.isNull()) {
		icon = application->icon();
	}

	int count(0);
	for (const Result &result : query->results()) {
		if (count >= entries) {
			break;
		}

		suggestions
				<< Suggestion(result.id(), result.commandName(),
						result.commandHighlights(), result.description(),
						result.descriptionHighlights(), icon);
		++count;
	}

	querykey.setVariant(query->path().path());

	return queryString;
}

void HudServiceImpl::ExecuteQuery(const QDBusVariant &itemKey, uint timestamp) {
	Q_UNUSED(timestamp);
	QString sender(messageSender());

	QPair<Query::Ptr, QSharedPointer<QTimer>> entry(m_legacyQueries.take(sender));
	Query::Ptr query(entry.first);
	QSharedPointer<QTimer> legacyTimeout(entry.second);
	if (!query.isNull()) {
		legacyTimeout->stop();
		legacyTimeout->setProperty("sender", QVariant());

		query->ExecuteCommand(itemKey, timestamp);
		closeQuery(query->path());
	}
}

void HudServiceImpl::CloseQuery(const QDBusVariant &querykey) {
	Q_UNUSED(querykey);
	QString sender(messageSender());

	// We don't actually close legacy queries, or we'd be constructing
	// and destructing them during the search, due to the way that
	// Unity7 uses the API.
	QPair<Query::Ptr, QSharedPointer<QTimer>> entry(m_legacyQueries[sender]);
	Query::Ptr query(entry.first);
	QSharedPointer<QTimer> legacyTimeout(entry.second);
	if (!query.isNull()) {
		query->UpdateQuery(QString());
		legacyTimeout->start();
		legacyTimeout->setProperty("sender", sender);
	}
}

void HudServiceImpl::legacyTimeout() {
	QObject *timer(sender());
	if (!timer) {
		return;
	}

	QVariant from(timer->property("sender"));
	if (from.isNull()) {
		return;
	}

	QString sender(from.toString());
	QPair<Query::Ptr, QSharedPointer<QTimer>> entry(m_legacyQueries.take(sender));
	Query::Ptr query(entry.first);
	if (query) {
		closeQuery(query->path());
	}

}
