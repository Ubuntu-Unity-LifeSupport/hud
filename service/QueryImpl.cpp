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

#include <common/AppstackModel.h>
#include <common/DBusTypes.h>
#include <common/Localisation.h>
#include <common/ResultsModel.h>
#include <service/HudService.h>
#include <service/QueryImpl.h>
#include <service/QueryAdaptor.h>

#include <QStringList>

using namespace hud::common;
using namespace hud::service;

QueryImpl::QueryImpl(unsigned int id, const QString &query,
		const QString &sender, EmptyBehaviour emptyBehaviour,
		HudService &service, ApplicationList::Ptr applicationList,
		Voice::Ptr voice, const QDBusConnection &connection, QObject *parent) :
		Query(parent), m_adaptor(new QueryAdaptor(this)), m_connection(
				connection), m_path(DBusTypes::queryPath(id)), m_service(
				service), m_emptyBehaviour(emptyBehaviour), m_applicationList(
				applicationList), m_voice(voice), m_query(query), m_serviceWatcher(
				sender, m_connection,
				QDBusServiceWatcher::WatchForUnregistration) {

	connect(&m_serviceWatcher, SIGNAL(serviceUnregistered(const QString &)),
			this, SLOT(serviceUnregistered(const QString &)));

	connect(m_applicationList.data(), SIGNAL(focusedWindowChanged()), this,
			SLOT(refresh()));

	m_resultsModel.reset(new ResultsModel(id));
	m_appstackModel.reset(new AppstackModel(id));

	refresh();

	if (!m_connection.registerObject(m_path.path(), this)) {
		throw std::logic_error(
				_("Unable to register HUD query object on DBus"));
	}

	connect(m_voice.data(), SIGNAL(HeardSomething()), m_adaptor.data(),
			SIGNAL(VoiceQueryHeardSomething()));
	connect(m_voice.data(), SIGNAL(Listening()), m_adaptor.data(),
			SIGNAL(VoiceQueryListening()));
	connect(m_voice.data(), SIGNAL(Loading()), m_adaptor.data(),
			SIGNAL(VoiceQueryLoading()));
}

QueryImpl::~QueryImpl() {
	m_connection.unregisterObject(m_path.path());
}

const QDBusObjectPath & QueryImpl::path() const {
	return m_path;
}

const QList<Result> & QueryImpl::results() const {
	return m_results;
}

QString QueryImpl::appstackModel() const {
	return QString::fromStdString(m_appstackModel->name());
}

QString QueryImpl::currentQuery() const {
	return m_query;
}

QString QueryImpl::resultsModel() const {
	return QString::fromStdString(m_resultsModel->name());
}

QStringList QueryImpl::toolbarItems() const {
	if (!m_windowToken) {
		return QStringList();
	}
	return m_windowToken->toolbarItems();
}

void QueryImpl::ExecuteToolbar(const QString &item, uint timestamp) {
	Q_UNUSED(timestamp);
	if (!m_windowToken) {
		return;
	}
	return m_windowToken->executeToolbar(item);
}

void QueryImpl::notifyPropertyChanged(const QString& interface,
		const QString& propertyName) {
	QDBusMessage signal = QDBusMessage::createSignal(m_path.path(),
			"org.freedesktop.DBus.Properties", "PropertiesChanged");
	signal << interface;
	QVariantMap changedProps;
	changedProps.insert(propertyName, property(qPrintable(propertyName)));
	signal << changedProps;
	signal << QStringList();
	m_connection.send(signal);
}

void QueryImpl::CloseQuery() {
	m_service.closeQuery(m_path);
}

void QueryImpl::ExecuteCommand(const QDBusVariant &item, uint timestamp) {
	Q_UNUSED(timestamp);

	if (!item.variant().canConvert<qlonglong>()) {
		qWarning() << "Failed to execute command - invalid item key"
				<< item.variant();
		sendErrorReply(QDBusError::InvalidArgs,
				"Failed to execute command - invalid item key");
		return;
	}

	qulonglong commandId(item.variant().toULongLong());
	m_windowToken->execute(commandId);
}

/**
 * The inputs to this function are:
 * - ulongint Item, which is the numerical index of the action
 * - uint, timestamp
 *
 * The outputs are:
 * - string busName, bus name to connect to
 * - string prefix, the prefix the gmenu is using
 * - string baseAction, name of the action group
 * - path actionPath, where we can find the action group
 * - path modelPath, where we can find the menu
 * - int modelSection, not used
 */
QString QueryImpl::ExecuteParameterized(const QDBusVariant &item,
		uint timestamp, QString &prefix, QString &baseAction,
		QDBusObjectPath &actionPath, QDBusObjectPath &modelPath,
		int &modelSection) {
	Q_UNUSED(timestamp);

	if (!item.variant().canConvert<qlonglong>()) {
		qWarning() << "Failed to execute command - invalid item key"
				<< item.variant();
		sendErrorReply(QDBusError::InvalidArgs,
				"Failed to execute command - invalid item key");
		return QString();
	}

	qulonglong commandId(item.variant().toULongLong());
	modelSection = 1;
	return m_windowToken->executeParameterized(commandId, prefix, baseAction,
			actionPath, modelPath);
}

/**
 * This means that the user has clicked on an application
 * in the HUD user interface.
 */
int QueryImpl::UpdateApp(const QString &app) {
	qDebug() << "UpdateApp" << app;
	return 0;
}

int QueryImpl::UpdateQuery(const QString &query) {
	if (m_query == query) {
		return 0;
	}

	m_query = query;
	refresh();

	return 0;
}

void QueryImpl::updateToken(Window::Ptr window) {
	m_windowToken = window->activate();
	connect(m_windowToken.data(), SIGNAL(changed()), this, SLOT(refresh()));
}

void QueryImpl::refresh() {
	// First clear the old results
	m_results.clear();

	// Now check for an active window
	Window::Ptr window(m_applicationList->focusedWindow());
	if (window) {
		// Hold onto a token for the active window
		updateToken(window);

		m_windowToken->search(m_query, m_emptyBehaviour, m_results);

		notifyPropertyChanged("com.canonical.hud.query", "ToolbarItems");
	}

	// Convert to results list to Dee model
	m_resultsModel->beginChangeset();
	for (const Result &result : m_results) {
		m_resultsModel->addResult(result.id(), result.commandName(),
				result.commandHighlights(), result.description(),
				result.descriptionHighlights(), result.shortcut(),
				result.distance(), result.parameterized());
	}
	m_resultsModel->endChangeset();

	// Now check for an active application
	Application::Ptr application(m_applicationList->focusedApplication());
	m_appstackModel->beginChangeset();
	if (application) {
		m_appstackModel->addApplication(application->id(), application->icon(),
				AppstackModel::ITEM_TYPE_FOCUSED_APP);
	}
	//TODO Apps other than the foreground one
	m_appstackModel->endChangeset();
}

int QueryImpl::VoiceQuery(QString &query) {
	Window::Ptr window(m_applicationList->focusedWindow());

	if (window.isNull()) {
		qWarning() << "No focused window for voice query";
		return 0;
	}

	// Hold onto a token for the active window
	updateToken(window);

	// Get the list of commands from the current window token
	QList<QStringList> commandsList(m_windowToken->commands());

	// Listen for speech, and set result
	query = m_voice->listen(commandsList);

	// Update the query accordingly
	UpdateQuery(query);

	return 0;
}

void QueryImpl::serviceUnregistered(const QString &service) {
	Q_UNUSED(service);
	CloseQuery();
}
