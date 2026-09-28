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

#include <tests/menus/FakeTouchApplication.h>
#include <QCoreApplication>
#include <QDebug>
#include <csignal>

static void exitQt(int sig) {
	Q_UNUSED(sig);
	QCoreApplication::exit(0);
}

int main(int argc, char **argv) {
	QCoreApplication application(argc, argv);

	if (argc != 4) {
		qCritical() << argv[0]
				<< "APP_ID DBUS_NAME DBUS_PATH is how you should use this program.\n";
		return 1;
	}

	signal(SIGINT, &exitQt);
	signal(SIGTERM, &exitQt);

	FakeTouchApplication fakeApp(argv[1], QDBusConnection::sessionBus(),
			argv[2], argv[3]);

	return application.exec();
}
