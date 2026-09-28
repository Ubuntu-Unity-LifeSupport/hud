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

#ifndef QTGMENUMODEL_H
#define QTGMENUMODEL_H

#include <QDBusObjectPath>
#include <QObject>
#include <QMap>
#include <QMenu>

#include <memory>
#include <deque>

#undef signals
#include <gio/gio.h>

namespace qtgmenu
{

class QtGMenuModel : public QObject
{
Q_OBJECT

public:
  enum class LinkType
  {
    Root, Section, SubMenu
  };

  QtGMenuModel( QSharedPointer<GDBusConnection> connection, const QString& bus_name, const QString& menu_path, const QMap<QString, QDBusObjectPath>& action_paths );
  virtual ~QtGMenuModel();

  QSharedPointer<GMenuModel> Model() const;
  LinkType Type() const;

  QtGMenuModel* Parent() const;
  QSharedPointer<QtGMenuModel> Child( int index ) const;

  std::shared_ptr< QMenu > GetQMenu();

  constexpr static const char* c_property_actionName = "actionName";
  constexpr static const char* c_property_isParameterized = "isParameterized";
  constexpr static const char* c_property_busName = "busName";
  constexpr static const char* c_property_actionsPath = "actionsPath";
  constexpr static const char* c_property_menuPath = "menuPath";
  constexpr static const char* c_property_keywords = "keywords";
  constexpr static const char* c_property_hud_toolbar_item = "hud-toolbar-item";

Q_SIGNALS:
  void MenuItemsChanged( QtGMenuModel* model, int index, int removed, int added );
  void ActionTriggered( QString action_name, bool checked );
  void MenuInvalid();

public Q_SLOTS:
  void ActionEnabled( QString action_name, bool enabled );
  void ActionParameterized( QString action_name, bool parameterized );

private Q_SLOTS:
  void ActionTriggered( bool );

private:
  QtGMenuModel( QSharedPointer<GMenuModel> model, LinkType link_type, QtGMenuModel* parent, int index );

  static QSharedPointer<QtGMenuModel> CreateChild( QtGMenuModel* parent_qtgmenu, QSharedPointer<GMenuModel> parent_gmenu, int child_index );

  static void MenuItemsChangedCallback( GMenuModel* model, gint index, gint removed, gint added,
      gpointer user_data );

  void ChangeMenuItems( const int index, int added, const int removed );

  void ConnectCallback();
  void DisconnectCallback();

  void InsertChild( QSharedPointer<QtGMenuModel> child, int index );

  QAction* CreateAction( int index );

  void AppendQMenu( std::shared_ptr< QMenu > top_menu );
  void UpdateExtQMenu();

  void ActionAdded( const QString& name, QAction* action );
  void ActionRemoved( const QString& name, QAction* action );

  void ReportRecoverableError(const int index, const int added, const int removed);

private:
  QtGMenuModel* m_parent = nullptr;
  QMap< int, QSharedPointer<QtGMenuModel>> m_children;

  QSharedPointer<GMenuModel> m_model;
  gulong m_items_changed_handler = 0;

  LinkType m_link_type;
  int m_size = 0;

  QScopedPointer<QMenu> m_menu;
  QScopedPointer<QMenu> m_ext_menu;

  QSharedPointer<GDBusConnection> m_connection;
  QString m_bus_name;
  QString m_menu_path;
  QMap<QString, QDBusObjectPath> m_action_paths;

  // a map of QActions indexed by their name and stored with a reference count
  std::map< QString, std::vector< QAction* > > m_actions;

  bool m_error_reported = false;
};

} // namespace qtgmenu

#endif // QTGMENUMODEL_H
