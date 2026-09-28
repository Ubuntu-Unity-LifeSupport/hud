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
 * Author: Marcus Tomlinson <marcus.tomlinson@canonical.com>
 */

#include <MainWindow.h>
#include <QApplication>
#include <QDebug>

int main( int argc, char **argv )
{
  QApplication application( argc, argv );

  if(argc != 4) {
	  qWarning() << "Usage:" << argv[0] << "BUS_NAME ACTION_PATH MENU_PATH";
	  return 1;
  }

  MainWindow mainWindow(argv[1], QDBusObjectPath( argv[2] ), QDBusObjectPath( argv[3] ), QDBusConnection::sessionBus(), QSharedPointer<GDBusConnection>(g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr), &g_object_unref) );
  mainWindow.show();

  return application.exec();
}
