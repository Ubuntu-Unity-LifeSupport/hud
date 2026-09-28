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

#include <QtGMenuImporter.h>
#include <internal/QtGMenuImporterPrivate.h>

#include <QMenu>

using namespace qtgmenu;

QtGMenuImporter::QtGMenuImporter( const QString& service, const QDBusObjectPath& menu_path,
                                  const QString& action_prefix, const QDBusObjectPath& action_path,
                                  const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection,
                                  QObject* parent )
    : QObject( parent )
{
    QMap<QString, QDBusObjectPath> action_paths;
    action_paths[action_prefix] = action_path;
    d.reset( new QtGMenuImporterPrivate( service, menu_path, action_paths, *this, connection, gconnection ) );
}

QtGMenuImporter::QtGMenuImporter( const QString& service, const QDBusObjectPath& menu_path,
                                  const QMap<QString, QDBusObjectPath>& action_paths,
                                  const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection,
                                  QObject* parent )
    : QObject( parent ),
      d( new QtGMenuImporterPrivate( service, menu_path, action_paths, *this, connection, gconnection ) )
{
}

QtGMenuImporter::~QtGMenuImporter()
{
}

QSharedPointer<GMenuModel> QtGMenuImporter::GetGMenuModel() const
{
  return d->GetGMenuModel();
}

QSharedPointer<GActionGroup> QtGMenuImporter::GetGActionGroup( int index ) const
{
  return d->GetGActionGroup( index );
}

std::shared_ptr< QMenu > QtGMenuImporter::GetQMenu() const
{
  return d->GetQMenu();
}

void QtGMenuImporter::Refresh()
{
  d->Refresh();
}
