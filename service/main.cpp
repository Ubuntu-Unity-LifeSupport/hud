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
#include <service/Factory.h>
#include <service/SignalHandler.h>

#include <QDebug>
#include <QApplication>

using namespace std;
using namespace hud::service;

int main(int argc, char *argv[]) {
	qputenv("QT_QPA_PLATFORM", "minimal");
	QApplication application(argc, argv);

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain(GETTEXT_PACKAGE);

	try {
		Factory factory;
		factory.singletonHudService();
		SignalHandler handler;
		handler.setupUnixSignalHandlers();
		return application.exec();
	} catch (std::exception &e) {
		qWarning() << _("Hud Service:") << e.what();
		return 1;
	}
}
