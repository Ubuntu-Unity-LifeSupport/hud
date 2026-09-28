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

#include <common/DBusTypes.h>
#include <service/ApplicationListImpl.h>
#include <service/Factory.h>

#include <QDebug>

using namespace hud::common;
using namespace hud::service;

static QList<QString> IGNORED_APPLICATION_IDS = { "", "compiz", "hud-gui",
		"unknown" };

ApplicationListImpl::ApplicationListImpl(Factory &factory,
		QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack,
		QSharedPointer<QDBusServiceWatcher> windowStackWatcher) :
		m_windowStack(windowStack), m_windowStackWatcher(windowStackWatcher), m_factory(
				factory), m_focusedWindowId(0) {



	QDBusPendingReply<QList<WindowInfo>> windowsReply(
			m_windowStack->GetWindowStack());

	if (windowsReply.isError()) {
		qWarning() << windowsReply.error();
		return;
	}

	QList<WindowInfo> windows(windowsReply);
	for (const WindowInfo &window : windows) {
		ensureApplicationWithWindow(window.window_id, window.app_id);
		if (window.focused) {
			//FIXME Stage is wrong
			FocusedWindowChanged(window.window_id, window.app_id, 0);
		}
	}

	connect(m_windowStack.data(),
	SIGNAL(FocusedWindowChanged(uint, const QString &, uint)), this,
	SLOT(FocusedWindowChanged(uint, const QString &, uint)));

	connect(m_windowStack.data(),
	SIGNAL(WindowCreated(uint, const QString &)), this,
	SLOT(WindowCreated(uint, const QString &)));

	connect(m_windowStack.data(),
	SIGNAL(WindowDestroyed(uint, const QString &)), this,
	SLOT(WindowDestroyed(uint, const QString &)));
}

ApplicationListImpl::~ApplicationListImpl() {
}

void ApplicationListImpl::serviceUnregistered(const QString &service) {
	Q_UNUSED(service);

	m_focusedApplication.reset();
	m_applications.clear();
	m_focusedWindowId = 0;
}

bool ApplicationListImpl::isIgnoredApplication(const QString &applicationId) {
	if (IGNORED_APPLICATION_IDS.contains(applicationId)) {
		return true;
	}

	return false;
}

Application::Ptr ApplicationListImpl::ensureApplication(
		const QString &applicationId) {
	if (isIgnoredApplication(applicationId)) {
		return Application::Ptr();
	}

	Application::Ptr application(m_applications[applicationId]);
	if (application.isNull()) {
		application = m_factory.newApplication(applicationId);
		m_applications[applicationId] = application;
	}

	return application;
}

void ApplicationListImpl::ensureApplicationWithWindow(uint windowId,
		const QString& applicationId) {
	Application::Ptr application(ensureApplication(applicationId));

	if (!application.isNull()) {
		application->addWindow(windowId);
	}
}

void ApplicationListImpl::removeWindow(uint windowId,
		const QString& applicationId) {
	if (isIgnoredApplication(applicationId)) {
		return;
	}

	Application::Ptr application(m_applications[applicationId]);

	if (application.isNull()) {
		qWarning() << "Attempt to remove window" << windowId
				<< "from non-existent application" << applicationId;
		return;
	}

	application->removeWindow(windowId);

	// If the application has no windows left, then the best
	// we can do is assume it has been closed.
	if (application->isEmpty()) {
		m_applications.remove(applicationId);

		// If this was the focused application
		if (application == m_focusedApplication) {
			setFocusedWindow(Application::Ptr(), 0);
		}
	}
}

void ApplicationListImpl::setFocusedWindow(Application::Ptr application,
		uint windowId) {
	m_focusedApplication = application;
	m_focusedWindowId = windowId;

	focusedWindowChanged();
}

void ApplicationListImpl::FocusedWindowChanged(uint windowId,
		const QString &applicationId, uint stage) {
	Q_UNUSED(stage);
	if (isIgnoredApplication(applicationId)) {
		return;
	}

	setFocusedWindow(m_applications[applicationId], windowId);
}

void ApplicationListImpl::WindowCreated(uint windowId,
		const QString &applicationId) {
	ensureApplicationWithWindow(windowId, applicationId);
}

void ApplicationListImpl::WindowDestroyed(uint windowId,
		const QString &applicationId) {
	removeWindow(windowId, applicationId);
}

QList<NameObject> ApplicationListImpl::applications() const {
	QList<NameObject> results;
	for (auto i(m_applications.cbegin()); i != m_applications.cend(); ++i) {
		Application::Ptr application(i.value());
		if (application) {
			results << NameObject(i.key(), application->path());
		}
	}
	return results;
}

Application::Ptr ApplicationListImpl::focusedApplication() const {
	return m_focusedApplication;
}

Window::Ptr ApplicationListImpl::focusedWindow() const {
	Window::Ptr window;
	Application::Ptr application(focusedApplication());
	if (!application.isNull()) {
		window = application->window(m_focusedWindowId);
	}
	return window;
}
