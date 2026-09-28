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

#include <QtGMenuImporterPrivate.h>
#include <QtGMenuUtils.h>

#include <QDebug>
#include <QDBusConnection>
#include <QEventLoop>

using namespace qtgmenu;

QtGMenuImporterPrivate::QtGMenuImporterPrivate( const QString& service, const QDBusObjectPath& menu_path,
                                                const QMap<QString, QDBusObjectPath>& action_paths, QtGMenuImporter& parent,
                                                const QDBusConnection& connection, QSharedPointer<GDBusConnection> gconnection)
    : QObject( 0 ),
      m_service_watcher( service, connection,
          QDBusServiceWatcher::WatchForOwnerChange ),
      m_parent( parent ),
      m_connection ( gconnection ),
      m_service( service ),
      m_menu_path( menu_path ),
      m_action_paths( action_paths )
{
  connect( &m_service_watcher, SIGNAL( serviceRegistered( const QString& ) ), this,
      SLOT( ServiceRegistered() ) );

  connect( &m_service_watcher, SIGNAL( serviceUnregistered( const QString& ) ), this,
      SLOT( ServiceUnregistered() ) );

  Refresh();
}

QtGMenuImporterPrivate::~QtGMenuImporterPrivate()
{
  ClearMenuModel();
  ClearActionGroups();
}

QSharedPointer<GMenuModel> QtGMenuImporterPrivate::GetGMenuModel()
{
  if( m_menu_model == nullptr )
  {
    return QSharedPointer<GMenuModel>();
  }

  return m_menu_model->Model();
}

QSharedPointer<GActionGroup> QtGMenuImporterPrivate::GetGActionGroup( int index )
{
  if( index >= m_action_groups.size() ||
      m_action_groups[index] == nullptr )
  {
    return QSharedPointer<GActionGroup>();
  }

  return m_action_groups[index]->ActionGroup();
}

std::shared_ptr< QMenu > QtGMenuImporterPrivate::GetQMenu()
{
  if( m_menu_model == nullptr )
  {
    return nullptr;
  }

  return m_menu_model->GetQMenu();
}

void QtGMenuImporterPrivate::Refresh()
{
  if( !m_menu_path.path().isEmpty() )
  {
    RefreshGMenuModel();
  }

  if( !m_action_paths.empty() )
  {
    RefreshGActionGroup();
  }
}

void QtGMenuImporterPrivate::ClearMenuModel()
{
  if( m_menu_model == nullptr )
  {
    return;
  }

  m_menu_model->disconnect();
  m_menu_actions_linked = false;

  m_menu_model = nullptr;
}

void QtGMenuImporterPrivate::ClearActionGroups()
{
  for( auto& action_group : m_action_groups )
  {
    action_group->disconnect();
  }

  m_menu_actions_linked = false;
  m_action_groups.clear();
}

void QtGMenuImporterPrivate::LinkMenuActions()
{
  if( m_menu_model && !m_action_groups.empty() && !m_menu_actions_linked )
  {
    for( auto& action_group : m_action_groups )
    {
      connect( m_menu_model.get(), SIGNAL( ActionTriggered( QString, bool ) ), action_group.get(),
          SLOT( TriggerAction( QString, bool ) ) );

      connect( m_menu_model.get(), SIGNAL( MenuItemsChanged( QtGMenuModel*, int, int, int ) ),
          action_group.get(), SLOT( EmitStates() ) );

      connect( action_group.get(), SIGNAL( ActionEnabled( QString, bool ) ), m_menu_model.get(),
          SLOT( ActionEnabled( QString, bool ) ) );

      connect( action_group.get(), SIGNAL( ActionParameterized( QString, bool ) ),
          m_menu_model.get(), SLOT( ActionParameterized( QString, bool ) ) );
    }

    m_menu_actions_linked = true;
  }
}

void QtGMenuImporterPrivate::ServiceRegistered()
{
  Refresh();
}

void QtGMenuImporterPrivate::ServiceUnregistered()
{
  ClearMenuModel();
  ClearActionGroups();
}

void QtGMenuImporterPrivate::RefreshGMenuModel()
{
  // clear the menu model for the refresh
  ClearMenuModel();

  QString menu_path = m_menu_path.path();
  m_menu_model = std::make_shared< QtGMenuModel > ( m_connection, m_service, menu_path, m_action_paths );

  connect( m_menu_model.get(), SIGNAL( MenuItemsChanged( QtGMenuModel*, int, int,
          int ) ), &m_parent, SIGNAL( MenuItemsChanged()) );

  connect( m_menu_model.get(), SIGNAL( MenuInvalid() ), this, SLOT( MenuInvalid() ) );
}

void QtGMenuImporterPrivate::RefreshGActionGroup()
{
  // clear the action groups for the refresh
  ClearActionGroups();

  QMapIterator<QString, QDBusObjectPath> action_path_it(m_action_paths);
  while( action_path_it.hasNext() )
  {
    action_path_it.next();

    QString action_path = action_path_it.value().path();
    m_action_groups.push_back(
                std::make_shared<QtGActionGroup>(m_connection,
                        action_path_it.key(), m_service, action_path));

    auto action_group = m_action_groups.back();

    connect( action_group.get(), SIGNAL( ActionAdded( QString ) ), &m_parent,
        SIGNAL( ActionAdded( QString ) ) );
    connect( action_group.get(), SIGNAL( ActionRemoved( QString ) ), &m_parent,
        SIGNAL( ActionRemoved( QString ) ) );
    connect( action_group.get(), SIGNAL( ActionEnabled( QString, bool ) ), &m_parent,
        SIGNAL( ActionEnabled( QString, bool ) ) );
    connect( action_group.get(), SIGNAL( ActionStateChanged( QString,
            QVariant ) ), &m_parent, SIGNAL( ActionStateChanged( QString, QVariant) ) );
  }

  LinkMenuActions();
}

void QtGMenuImporterPrivate::MenuInvalid()
{
  disconnect( &m_service_watcher, SIGNAL( serviceRegistered( const QString& ) ), this,
        SLOT( ServiceRegistered() ) );

  disconnect( &m_service_watcher, SIGNAL( serviceUnregistered( const QString& ) ), this,
        SLOT( ServiceUnregistered() ) );

  ServiceUnregistered();
}
