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

#ifndef QTGMENUEXPORTERPRIVATE_H
#define QTGMENUEXPORTERPRIVATE_H

#include <QtGMenuImporter.h>
#include <internal/QtGMenuModel.h>
#include <internal/QtGActionGroup.h>

#include <QDBusServiceWatcher>
#include <QMenu>
#include <QTimer>
#include <memory>

#undef signals
#include <gio/gio.h>

namespace qtgmenu
{

class QtGMenuImporterPrivate : public QObject
{
Q_OBJECT

public:
  QtGMenuImporterPrivate( const QString& service, const QDBusObjectPath& menu_path,
                          const QMap<QString, QDBusObjectPath>& action_paths, QtGMenuImporter& parent,
                          const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection);
  virtual ~QtGMenuImporterPrivate();

  QSharedPointer<GMenuModel> GetGMenuModel();
  QSharedPointer<GActionGroup> GetGActionGroup( int index = 0);

  std::shared_ptr< QMenu > GetQMenu();

  void Refresh();

private:
  void ClearMenuModel();
  void ClearActionGroups();

  void LinkMenuActions();

private Q_SLOTS:
  void ServiceRegistered();
  void ServiceUnregistered();

  void RefreshGMenuModel();
  void RefreshGActionGroup();
  void MenuInvalid();

private:
  QDBusServiceWatcher m_service_watcher;

  QtGMenuImporter& m_parent;

  QSharedPointer<GDBusConnection> m_connection;
  QString m_service;
  QDBusObjectPath m_menu_path;
  QMap<QString, QDBusObjectPath> m_action_paths;

  std::shared_ptr< QtGMenuModel > m_menu_model = nullptr;
  std::vector< std::shared_ptr< QtGActionGroup > > m_action_groups;

  bool m_menu_actions_linked = false;
};

} // namespace qtgmenu

#endif /* QTGMENUEXPORTERPRIVATE_H */
