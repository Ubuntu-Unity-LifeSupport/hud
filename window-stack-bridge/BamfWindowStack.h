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

#ifndef BAMFWINDOWSTACK_H_
#define BAMFWINDOWSTACK_H_

#include <window-stack-bridge/AbstractWindowStack.h>
#include <window-stack-bridge/BamfInterface.h>
#include <window-stack-bridge/BamfViewInterface.h>

#include <QMap>
#include <QSharedPointer>

class Q_DECL_EXPORT BamfWindow {
public:
	explicit BamfWindow(const QString &path, const QDBusConnection &connection);

	virtual ~BamfWindow();

	unsigned int windowId();

	QString service() const;

	QString path() const;

	const QString & applicationId();

	const QString xProp(const QString &property);

	bool isError() const;

protected:
	OrgAyatanaBamfWindowInterface m_window;

	OrgAyatanaBamfViewInterface m_view;

	bool m_error;

	unsigned int m_windowId;

	QString m_applicationId;
};

class Q_DECL_EXPORT BamfWindowStack: public AbstractWindowStack {
Q_OBJECT

public:
	typedef QSharedPointer<BamfWindow> WindowPtr;

	BamfWindowStack(const QDBusConnection &connection, QObject *parent = 0);

	virtual ~BamfWindowStack();

public Q_SLOTS:
	QString GetAppIdFromPid(uint pid) override;

	hud::common::WindowInfoList GetWindowStack() override;

	QStringList GetWindowProperties(uint windowId, const QString &appId,
			const QStringList &names) override;

	QStringList GetWindowBusAddress(uint windowId) override;

protected Q_SLOTS:
	void ActiveWindowChanged(const QString &oldWindow,
			const QString &newWindow);

	void ViewClosed(const QString &path, const QString &type);

	void ViewOpened(const QString &path, const QString &type);

	WindowPtr addWindow(const QString& path);

	WindowPtr removeWindow(const QString& path);

protected:
	OrgAyatanaBamfMatcherInterface m_matcher;

	QMap<QString, WindowPtr> m_windows;

	QMap<unsigned int, WindowPtr> m_windowsById;
};

#endif /* BAMFWINDOWSTACK_H_ */
