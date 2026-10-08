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

#include <BamfWindowStack.h>
#include <common/DBusTypes.h>
#include <common/Localisation.h>

#include <QFile>
#include <QFileInfo>

using namespace hud::common;

QString applicationIdFromDesktopFile(const QString &desktopFile,
		unsigned int windowId) {
	static const QString SUFFIX(".desktop");

	QString id(QFileInfo(desktopFile).fileName());
	if (id.endsWith(SUFFIX)) {
		id.chop(SUFFIX.size());
	}
	return id.isEmpty() ? QString::number(windowId) : id;
}

BamfWindow::BamfWindow(const QString &path, const QDBusConnection &connection) :
		m_window(DBusTypes::BAMF_DBUS_NAME, path, connection), m_view(
				DBusTypes::BAMF_DBUS_NAME, path, connection), m_error(false), m_windowId(
				0) {

	QDBusPendingReply<unsigned int> windowIdReply(m_window.GetXid());
	windowIdReply.waitForFinished();
	if (windowIdReply.isError()) {
		qWarning() << _("Could not get window ID for") << path
				<< windowIdReply.error();
		m_error = true;
		return;
	} else {
		m_windowId = windowIdReply;
	}

	Resolved resolved(resolveApplicationId(m_applicationId));
	if (resolved == Resolved::PARENTS_ERROR) {
		m_error = true;
		return;
	}

	// bamf can re-match a window to another application between our Parents()
	// and DesktopFile() calls, and the application we asked about is gone.
	// Keep the window with the window id as application id, as for an
	// application without a desktop file; BamfWindowStack::WindowAdded
	// corrects it when bamf announces the window under its application.
	if (resolved != Resolved::ID) {
		m_applicationId = QString::number(m_windowId);
	}
}

BamfWindow::Resolved BamfWindow::resolveApplicationId(QString &id) {
	QDBusPendingReply<QStringList> parentsReply(m_view.Parents());
	parentsReply.waitForFinished();
	if (parentsReply.isError()) {
		qWarning() << _("Error getting parents for") << path()
				<< parentsReply.error();
		return Resolved::PARENTS_ERROR;
	}

	QStringList parents(parentsReply);
	if (parents.empty()) {
		return Resolved::NO_PARENT;
	}

	OrgAyatanaBamfApplicationInterface application(DBusTypes::BAMF_DBUS_NAME,
			parents.first(), m_window.connection());
	QDBusPendingReply<QString> desktopFileReply(application.DesktopFile());
	desktopFileReply.waitForFinished();
	if (desktopFileReply.isError()) {
		qWarning() << _("Could not get desktop file for") << path()
				<< desktopFileReply.error();
		return Resolved::DESKTOP_FILE_ERROR;
	}

	id = applicationIdFromDesktopFile(desktopFileReply, m_windowId);
	return Resolved::ID;
}

BamfWindow::~BamfWindow() {
}

unsigned int BamfWindow::windowId() {
	return m_windowId;
}

QString BamfWindow::service() const
{
	return m_window.service();
}

QString BamfWindow::path() const
{
	return m_window.path();
}

const QString & BamfWindow::applicationId() {
	return m_applicationId;
}

void BamfWindow::setApplicationId(const QString &applicationId) {
	m_applicationId = applicationId;
}

bool BamfWindow::isError() const {
	return m_error;
}

const QString BamfWindow::xProp(const QString &property) {
	QDBusPendingReply<QString> propertyReply(m_window.Xprop(property));
	propertyReply.waitForFinished();
	if (propertyReply.isError()) {
		qWarning() << "Could not get window property" << property
				<< m_window.path();
		return QString();
	} else {
		return propertyReply;
	}
}

BamfWindowStack::WindowPtr BamfWindowStack::addWindow(const QString& path) {
	WindowPtr window(new BamfWindow(path, m_connection));
	if (!window->isError()) {
		m_windows[path] = window;
		m_windowsById[window->windowId()] = window;
	}
	return window;
}

BamfWindowStack::WindowPtr BamfWindowStack::removeWindow(const QString& path) {
	WindowPtr window(m_windows.take(path));
	if (!window.isNull()) {
		m_windowsById.remove(window->windowId());
	}
	return window;
}

BamfWindowStack::BamfWindowStack(const QDBusConnection &connection,
		QObject *parent) :
		AbstractWindowStack(connection, parent), m_matcher(
				DBusTypes::BAMF_DBUS_NAME, DBusTypes::BAMF_MATCHER_DBUS_PATH,
				connection) {

	QDBusConnectionInterface* interface = connection.interface();
	if (!interface->isServiceRegistered(DBusTypes::BAMF_DBUS_NAME)) {
		QDBusReply<void> reply(
				interface->startService(DBusTypes::BAMF_DBUS_NAME));
	}

	registerOnBus();

	connect(&m_matcher,
	SIGNAL(ActiveWindowChanged(const QString&, const QString&)), this,
	SLOT(ActiveWindowChanged(const QString&, const QString&)));

	connect(&m_matcher,
	SIGNAL(ViewClosed(const QString&, const QString&)), this,
	SLOT(ViewClosed(const QString&, const QString&)));

	connect(&m_matcher,
	SIGNAL(ViewOpened(const QString&, const QString&)), this,
	SLOT(ViewOpened(const QString&, const QString&)));

	// bamf moves a window to another application (LibreOffice changes its
	// window class after mapping) without a ViewOpened or ViewClosed for the
	// window; the application it moves to sends WindowAdded. Connected before
	// WindowPaths(), so no move after that reply is missed
	if (!m_connection.connect(DBusTypes::BAMF_DBUS_NAME, QString(),
			"org.ayatana.bamf.application", "WindowAdded", this,
			SLOT(WindowAdded(const QString&)))) {
		qWarning()
				<< _("Could not connect to bamf's WindowAdded; windows moved to another application keep their first id");
	}

	QDBusPendingReply<QStringList> windowPathsReply(m_matcher.WindowPaths());
	windowPathsReply.waitForFinished();

	if (windowPathsReply.isError()) {
		qWarning() << _("Could not get window paths")
				<< windowPathsReply.error();
	} else {
		QStringList windowPaths(windowPathsReply);

		for (const QString &path : windowPaths) {
			addWindow(path);
		}
	}

	// hud-service takes the focused window from GetWindowStack, which marks
	// bamf's active window
	QDBusPendingReply<QString> activeWindowReply(m_matcher.ActiveWindow());
	activeWindowReply.waitForFinished();
	if (!activeWindowReply.isError() && m_windows.value(activeWindowReply)) {
		m_activeWindowPath = activeWindowReply;
	}
}

BamfWindowStack::~BamfWindowStack() {
}

QString BamfWindowStack::GetAppIdFromPid(uint pid) {
	Q_UNUSED(pid);
	// FIXME Not implemented
	sendErrorReply(QDBusError::NotSupported,
			"GetAppIdFromPid method not implemented");
	return QString();
}

WindowInfoList BamfWindowStack::GetWindowStack() {
	WindowInfoList results;

	QDBusPendingReply<QStringList> stackReply(
			m_matcher.WindowStackForMonitor(-1));
	stackReply.waitForFinished();
	if (stackReply.isError()) {
		qWarning() << "Failed to get BAMF window stack" << stackReply.error();
		return results;
	}

	QStringList stack(stackReply);
	for (const QString &path : stack) {
		const auto window(m_windows.value(path));
		if (window) {
			results
					<< WindowInfo(window->windowId(), window->applicationId(),
							false);
		}
	}

	QDBusPendingReply<QString> activeWindowReply(m_matcher.ActiveWindow());
	activeWindowReply.waitForFinished();
	if (activeWindowReply.isError()) {
		qWarning() << "Failed to get BAMF active window"
				<< activeWindowReply.error();
		return results;
	}

	const auto window(m_windows.value(activeWindowReply));
	if (window) {
		const uint windowId(window->windowId());

		for (WindowInfo &windowInfo : results) {
			if (windowInfo.window_id == windowId) {
				windowInfo.focused = true;
				m_activeWindowPath = activeWindowReply;
			}
		}
	}

	return results;
}

QStringList BamfWindowStack::GetWindowProperties(uint windowId,
		const QString &appId, const QStringList &names) {
	Q_UNUSED(appId);
	QStringList result;
	const auto window = m_windowsById.value(windowId);

	if (window == nullptr) {
		sendErrorReply(QDBusError::InvalidArgs, "Unable to find windowId");
		return result;
	}

	for (const QString &name : names) {
		if (window) {
			result << window->xProp(name);
		} else {
			result << QString();
		}
	}
	return result;
}

QStringList BamfWindowStack::GetWindowBusAddress(uint windowId) {
	const auto window = m_windowsById.value(windowId);

	if (window == nullptr) {
		sendErrorReply(QDBusError::InvalidArgs, "Unable to find windowId");
		return {};
	}

	return {window->service(), window->path()};
}

void BamfWindowStack::ActiveWindowChanged(const QString &oldWindowPath,
		const QString &newWindowPath) {
	Q_UNUSED(oldWindowPath);
	if (!newWindowPath.isEmpty()) {
		const auto window(m_windows.value(newWindowPath));
		if (window) {
			m_activeWindowPath = newWindowPath;
			FocusedWindowChanged(window->windowId(), window->applicationId(),
					WindowInfo::MAIN);
		}
	}
}

void BamfWindowStack::ViewClosed(const QString &path, const QString &type) {
	if (type == "window") {
		if (path == m_activeWindowPath) {
			m_activeWindowPath.clear();
		}
		WindowPtr window(removeWindow(path));
		if (!window.isNull()) {
			WindowDestroyed(window->windowId(), window->applicationId());
		}
	}
}

void BamfWindowStack::WindowAdded(const QString &path) {
	// The window is now under the application that sent this. Ask bamf again
	// rather than trusting the sender: signals queued during a move arrive
	// after it. Keep the id when bamf gives no answer (no parent while the
	// window moves or closes, an error); a move always ends with a
	// WindowAdded from the final application
	const auto window(m_windows.value(path));
	if (!window) {
		return;
	}

	QString applicationId;
	if (window->resolveApplicationId(applicationId) != BamfWindow::Resolved::ID
			|| applicationId == window->applicationId()) {
		return;
	}

	// hud-service keys windows by application id: announce the window under
	// the new id first, give it the focus if hud-service has it focused, and
	// only then remove it from the old application. Removing first would
	// empty the focused application and leave hud-service with no focus
	const QString oldApplicationId(window->applicationId());
	window->setApplicationId(applicationId);
	WindowCreated(window->windowId(), applicationId);
	if (path == m_activeWindowPath) {
		FocusedWindowChanged(window->windowId(), applicationId,
				WindowInfo::MAIN);
	}
	WindowDestroyed(window->windowId(), oldApplicationId);
}

void BamfWindowStack::ViewOpened(const QString &path, const QString &type) {
	if (type == "window") {
		WindowPtr window(addWindow(path));
		if (!window->isError()) {
			WindowCreated(window->windowId(), window->applicationId());
		}
	}
}
