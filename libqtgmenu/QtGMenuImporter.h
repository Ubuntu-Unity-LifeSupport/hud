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

#ifndef QTGMENUIMPORTER_H
#define QTGMENUIMPORTER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <memory>
#include <gio/gio.h>

class QMenu;

class _GMenuModel;
typedef _GMenuModel GMenuModel;

class _GActionGroup;
typedef _GActionGroup GActionGroup;

namespace qtgmenu
{

class QtGMenuImporterPrivate;

class QtGMenuImporter final : public QObject
{
Q_OBJECT

public:
  QtGMenuImporter( const QString& service, const QDBusObjectPath& menu_path,
                   const QString& action_prefix, const QDBusObjectPath& action_path,
                   const QDBusConnection& connection, QSharedPointer<GDBusConnection>,
                   QObject* parent = 0 );
  QtGMenuImporter( const QString& service, const QDBusObjectPath& menu_path,
                   const QMap<QString, QDBusObjectPath>& action_paths,
                   const QDBusConnection& connection, QSharedPointer<GDBusConnection>,
                   QObject* parent = 0 );
  virtual ~QtGMenuImporter();

  QSharedPointer<GMenuModel> GetGMenuModel() const;
  QSharedPointer<GActionGroup> GetGActionGroup( int index = 0 ) const;

  std::shared_ptr< QMenu > GetQMenu() const;

  void Refresh();

Q_SIGNALS:
  void MenuItemsChanged();

  void ActionAdded( QString action_name );
  void ActionRemoved( QString action_name );
  void ActionEnabled( QString action_name, bool enabled );
  void ActionStateChanged( QString action_name, QVariant value );

private:
  Q_DISABLE_COPY (QtGMenuImporter)
  std::unique_ptr< QtGMenuImporterPrivate > d;
};

} // namespace qtgmenu

#endif // QTGMENUIMPORTER_H
