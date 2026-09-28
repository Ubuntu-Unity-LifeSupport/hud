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

#ifndef ABSTRACTWINDOWSTACK_H_
#define ABSTRACTWINDOWSTACK_H_

#include <common/WindowInfo.h>

#include <QList>
#include <QString>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusContext>
#include <QScopedPointer>

class WindowStackAdaptor;

class Q_DECL_EXPORT AbstractWindowStack: public QObject, protected QDBusContext {
Q_OBJECT

public:
	explicit AbstractWindowStack(const QDBusConnection &connection,
			QObject *parent = 0);

	virtual ~AbstractWindowStack();

public:

	Q_DECL_EXPORT
	static void registerMetaTypes();

public Q_SLOTS:
	virtual QString GetAppIdFromPid(uint pid) = 0;

	virtual hud::common::WindowInfoList GetWindowStack() = 0;

	virtual QStringList GetWindowProperties(uint windowId, const QString &appId,
			const QStringList &names) = 0;

	virtual QStringList GetWindowBusAddress(uint windowId) = 0;

Q_SIGNALS:
	void FocusedWindowChanged(uint windowId, const QString &appId, uint stage);

	void WindowCreated(uint windowId, const QString &appId);

	void WindowDestroyed(uint windowId, const QString &appId);

protected:
	void registerOnBus();

	QScopedPointer<WindowStackAdaptor> m_adaptor;

	QDBusConnection m_connection;
};

#endif /* ABSTRACTWINDOWSTACK_H_ */
