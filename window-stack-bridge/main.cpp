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

#include <common/Localisation.h>
#include <WindowStack.h>

#include <QDBusConnection>
#include <QDebug>
#include <QCoreApplication>
#include <csignal>

using namespace std;

static void exitQt(int sig) {
	Q_UNUSED(sig);
	QCoreApplication::exit(0);
}

int main(int argc, char *argv[]) {
	QCoreApplication application(argc, argv);

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain(GETTEXT_PACKAGE);

	signal(SIGINT, &exitQt);
	signal(SIGTERM, &exitQt);

	try {
		WindowStack windowStack(QDBusConnection::sessionBus());
		return application.exec();
	} catch (std::exception &e) {
		qWarning() << _("Window Stack Bridge:") << e.what();
		return application.exec();
	}
}
