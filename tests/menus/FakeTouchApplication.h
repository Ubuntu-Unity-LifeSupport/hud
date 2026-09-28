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

#ifndef FAKETOUCHAPPLICATION_H_
#define FAKETOUCHAPPLICATION_H_

#include <QDBusConnection>
#include <QSharedPointer>
#include <libhud/hud.h>

class TestAdaptor;

class FakeTouchApplication: public QObject {
public:
	static const constexpr char *UNITY_ACTION_EXPORT_PATH =
			"/com/canonical/unity/actions";

	explicit FakeTouchApplication(const QString &applicationId,
			const QDBusConnection &connection, const QString &dbusName,
			const QString &dbusPath);

	~FakeTouchApplication();

	static void actionCallback(GSimpleAction *simple, GVariant *parameter,
			gpointer user_data);

protected:
	QSharedPointer<TestAdaptor> m_adaptor;

	QDBusConnection m_connection;

	GDBusConnection *m_sessionBus;

	HudManager *m_hudManager;

	GSimpleActionGroup *m_actionGroup;

	HudActionPublisher *m_actionPublisher;

	uint m_exportId;

};

#endif /* FAKETOUCHAPPLICATION_H_ */
