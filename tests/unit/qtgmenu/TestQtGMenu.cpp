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

#include <libqtgmenu/QtGMenuImporter.h>
#include <libqtgmenu/internal/QtGMenuUtils.h>
#include <common/GDBusHelper.h>

#include <QMenu>
#include <QSignalSpy>

#include <libqtdbustest/DBusTestRunner.h>

#include <gtest/gtest.h>

#undef signals
#include <gio/gio.h>

using namespace qtgmenu;
using namespace testing;
using namespace QtDBusTest;

namespace
{

class TestQtGMenu : public QObject,
                    public Test
{
Q_OBJECT

protected:
  TestQtGMenu()
      : m_connection(newSessionBusConnection(nullptr), &g_object_unref),
        m_importer( c_service, QDBusObjectPath( c_path ), "app", QDBusObjectPath( c_path ), dbus.sessionConnection(), m_connection ),

        m_items_changed_spy( &m_importer, SIGNAL( MenuItemsChanged() ) ),

        m_action_added_spy( &m_importer, SIGNAL( ActionAdded( QString ) ) ),
        m_action_removed_spy( &m_importer, SIGNAL( ActionRemoved( QString ) ) ),
        m_action_enabled_spy( &m_importer, SIGNAL( ActionEnabled( QString, bool ) ) ),
        m_action_state_changed_spy( &m_importer,
            SIGNAL( ActionStateChanged( QString, QVariant ) ) ),

        m_action_activated_spy( this, SIGNAL( ActionActivated( QString, QVariant ) ) )
  {
      dbus.startServices();
  }

  void ownBus()
  {
      m_owner_id = g_bus_own_name_on_connection( m_connection.data(), c_service, G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL,
                  NULL, NULL );
  }

  void SetUp() override
  {
    m_menu.reset(g_menu_new(), &g_object_unref);
    m_actions.reset(g_simple_action_group_new(), &g_object_unref);
  }

  void TearDown() override
  {
    UnexportGMenu();

    if (m_owner_id > 0)
    {
      g_bus_unown_name( m_owner_id );
    }
  }

  int GetGMenuSize()
  {
    QSharedPointer<GMenuModel> menu = m_importer.GetGMenuModel();

    if( !menu )
    {
      return 0;
    }

    gint item_count = g_menu_model_get_n_items( G_MENU_MODEL( menu.data() ) );

    return item_count;
  }

  int GetGActionCount()
  {
    QSharedPointer<GActionGroup> actions = m_importer.GetGActionGroup();

    if( !actions )
    {
      return 0;
    }

    gchar** actions_list = g_action_group_list_actions( actions.data() );

    int action_count = 0;
    while( actions_list[action_count] != nullptr )
    {
      ++action_count;
    }

    g_strfreev( actions_list );
    return action_count;
  }

  static void ActivateAction( GSimpleAction* simple, GVariant* parameter, gpointer user_data )
  {
    TestQtGMenu* self = reinterpret_cast< TestQtGMenu* >( user_data );

    const gchar* action_name = g_action_get_name( G_ACTION( simple ) );
    emit self->ActionActivated( action_name, QtGMenuUtils::GVariantToQVariant( parameter ) );
  }

  void ExportGMenu()
  {
    // build m_menu

    QSharedPointer<GMenu> menus_section(g_menu_new(), &g_object_unref);

    g_menu_append_section( m_menu.data(), "Menus", G_MENU_MODEL( menus_section.data() ) );

    // export menu
    m_menu_export_id = g_dbus_connection_export_menu_model( m_connection.data(), c_path,
                                                            G_MENU_MODEL( m_menu.data() ), NULL );

    // export actions
    m_actions_export_id = g_dbus_connection_export_action_group( m_connection.data(), c_path,
                                                                 G_ACTION_GROUP( m_actions.data() ), NULL );
    ownBus();

    if (m_items_changed_spy.isEmpty())
    {
        ASSERT_TRUE(m_items_changed_spy.wait());
    }
    ASSERT_FALSE( m_items_changed_spy.empty() );
    m_items_changed_spy.clear();

    //-- build file menu

    QSharedPointer<GMenu> file_submenu(g_menu_new(), &g_object_unref);
    QSharedPointer<GMenu> files_section(g_menu_new(), &g_object_unref);

    QSharedPointer<GMenuItem> menu_item(g_menu_item_new( "New", "app.new" ), &g_object_unref);
    g_menu_item_set_attribute_value( menu_item.data(), "accel", g_variant_new_string( "<Control>N" ) );
    g_menu_append_item( files_section.data(), menu_item.data() );

    g_menu_append( files_section.data(), "Open", "app.open" );

    g_menu_append_section( file_submenu.data(), "Files", G_MENU_MODEL( files_section.data() ) );

    QSharedPointer<GMenu> mod_section(g_menu_new(), &g_object_unref);

    g_menu_append( mod_section.data(), "Lock", "app.lock" );

    g_menu_append_section( file_submenu.data(), "Modify", G_MENU_MODEL( mod_section.data() ) );

    g_menu_append_submenu( menus_section.data(), "File", G_MENU_MODEL( file_submenu.data() ) );
    if (m_items_changed_spy.isEmpty())
    {
        ASSERT_TRUE(m_items_changed_spy.wait());
    }
    ASSERT_FALSE( m_items_changed_spy.empty() );
    m_items_changed_spy.clear();

    //-- build edit menu

    QSharedPointer<GMenu> edit_submenu(g_menu_new(), &g_object_unref);
    QSharedPointer<GMenu> style_submenu(g_menu_new(), &g_object_unref);

    g_menu_append( style_submenu.data(), "Plain", "app.text_plain" );
    g_menu_append( style_submenu.data(), "Bold", "app.text_bold" );

    g_menu_append_submenu( menus_section.data(), "Edit", G_MENU_MODEL( edit_submenu.data() ) );
    if (m_items_changed_spy.isEmpty())
    {
        ASSERT_TRUE(m_items_changed_spy.wait());
    }
    ASSERT_FALSE( m_items_changed_spy.empty() );
    m_items_changed_spy.clear();

    g_menu_append_submenu( edit_submenu.data(), "Style", G_MENU_MODEL( style_submenu.data() ) );
    if (m_items_changed_spy.isEmpty())
    {
        ASSERT_TRUE(m_items_changed_spy.wait());
    }
    ASSERT_FALSE( m_items_changed_spy.empty() );
    m_items_changed_spy.clear();

    //-- stateless
    m_action_added_spy.clear();

    QSharedPointer<GSimpleAction> action(g_simple_action_new( "new", nullptr ), &g_object_unref);
    m_exported_actions.push_back(
        std::make_pair( action,
            g_signal_connect( action.data(), "activate", G_CALLBACK( ActivateAction ), this ) ) );
    g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action.data() ) );
    if (m_action_added_spy.isEmpty())
    {
        ASSERT_TRUE(m_action_added_spy.wait());
    }
    ASSERT_FALSE( m_action_added_spy.empty() );
    m_action_added_spy.clear();

    action.reset(g_simple_action_new( "open", nullptr ), &g_object_unref);
    m_exported_actions.push_back(
        std::make_pair( action,
            g_signal_connect( action.data(), "activate", G_CALLBACK( ActivateAction ), this ) ) );
    g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action.data() ) );
    if (m_action_added_spy.isEmpty())
    {
        ASSERT_TRUE(m_action_added_spy.wait());
    }
    ASSERT_FALSE( m_action_added_spy.empty() );
    m_action_added_spy.clear();

    //-- boolean state

    action.reset(g_simple_action_new_stateful( "lock", nullptr, g_variant_new_boolean( false ) ), &g_object_unref);
    m_exported_actions.push_back(
        std::make_pair( action,
            g_signal_connect( action.data(), "activate", G_CALLBACK( ActivateAction ), this ) ) );
    g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action.data() ) );
    if (m_action_added_spy.isEmpty())
    {
        ASSERT_TRUE(m_action_added_spy.wait());
    }
    ASSERT_FALSE( m_action_added_spy.empty() );
    m_action_added_spy.clear();

    //-- string param + state

    action.reset(g_simple_action_new_stateful( "text_plain", G_VARIANT_TYPE_STRING,
        g_variant_new_string( "app.text_plain" ) ), &g_object_unref);
    m_exported_actions.push_back(
        std::make_pair( action,
            g_signal_connect( action.data(), "activate", G_CALLBACK( ActivateAction ), this ) ) );
    g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action.data() ) );
    if (m_action_added_spy.isEmpty())
    {
        ASSERT_TRUE(m_action_added_spy.wait());
    }
    ASSERT_FALSE( m_action_added_spy.empty() );
    m_action_added_spy.clear();

    action.reset(g_simple_action_new_stateful( "text_bold", G_VARIANT_TYPE_STRING,
        g_variant_new_string( "app.text_plain" ) ), &g_object_unref);
    m_exported_actions.push_back(
        std::make_pair( action,
            g_signal_connect( action.data(), "activate", G_CALLBACK( ActivateAction ), this ) ) );
    g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action.data() ) );
    if (m_action_added_spy.isEmpty())
    {
        ASSERT_TRUE(m_action_added_spy.wait());
    }
    ASSERT_FALSE( m_action_added_spy.empty() );
    m_action_added_spy.clear();
  }

  void UnexportGMenu()
  {
    // disconnect signal handlers
    for( auto& action : m_exported_actions )
    {
      g_signal_handler_disconnect( action.first.data(), action.second );
    }

    // unexport menu

    if (m_menu_export_id > 0)
    {
      g_dbus_connection_unexport_menu_model( m_connection.data(), m_menu_export_id );
    }

    m_importer.Refresh();

    EXPECT_EQ( 0, GetGMenuSize() );

    // unexport actions

    if (m_actions_export_id > 0)
    {
      g_dbus_connection_unexport_action_group( m_connection.data(), m_actions_export_id );
    }

    m_importer.Refresh();

    EXPECT_EQ( 0, GetGActionCount() );
  }

Q_SIGNALS:
  void ActionActivated( QString action_name, QVariant parameter );

protected:
  constexpr static const char* c_service = "com.canonical.qtgmenu";
  constexpr static const char* c_path = "/com/canonical/qtgmenu";

  DBusTestRunner dbus;

  guint m_owner_id = 0;
  guint m_menu_export_id = 0;
  guint m_actions_export_id = 0;

  QSharedPointer<GDBusConnection> m_connection;
  QSharedPointer<GMenu> m_menu;
  QSharedPointer<GSimpleActionGroup> m_actions;

  QtGMenuImporter m_importer;

  QSignalSpy m_items_changed_spy;

  QSignalSpy m_action_added_spy;
  QSignalSpy m_action_removed_spy;
  QSignalSpy m_action_enabled_spy;
  QSignalSpy m_action_state_changed_spy;

  QSignalSpy m_action_activated_spy;

  std::vector< std::pair< QSharedPointer<GSimpleAction>, gulong > > m_exported_actions;
};

TEST_F( TestQtGMenu, DISABLED_ExportImportGMenu )
{
  // no menu exported

  EXPECT_EQ( 0, GetGMenuSize() );

  // export menu

  m_menu_export_id = g_dbus_connection_export_menu_model( m_connection.data(), c_path,
      G_MENU_MODEL( m_menu.data() ), NULL );

  ownBus();

  // add 1 item
  m_items_changed_spy.clear();
  g_menu_append( m_menu.data(), "New", "app.new" );
  if (m_items_changed_spy.isEmpty())
  {
    ASSERT_TRUE(m_items_changed_spy.wait());
  }
  ASSERT_FALSE( m_items_changed_spy.empty() );

  EXPECT_NE( nullptr, m_importer.GetQMenu() );
  ASSERT_EQ( 1, GetGMenuSize() );

  // add 2 items

  m_items_changed_spy.clear();
  g_menu_append( m_menu.data(), "Add", "app.add" );
  if (m_items_changed_spy.isEmpty())
  {
    ASSERT_TRUE(m_items_changed_spy.wait());
  }
  ASSERT_FALSE( m_items_changed_spy.empty() );

  m_items_changed_spy.clear();
  g_menu_append( m_menu.data(), "Del", "app.del" );
  if (m_items_changed_spy.isEmpty())
  {
    ASSERT_TRUE(m_items_changed_spy.wait());
  }
  ASSERT_FALSE( m_items_changed_spy.empty() );

  ASSERT_EQ( 3, GetGMenuSize() );

  // remove 1 item

  m_items_changed_spy.clear();
  g_menu_remove( m_menu.data(), 2 );
  if (m_items_changed_spy.isEmpty())
  {
    ASSERT_TRUE(m_items_changed_spy.wait());
  }
  ASSERT_FALSE( m_items_changed_spy.empty() );

  ASSERT_EQ( 2, GetGMenuSize() );
}

TEST_F( TestQtGMenu, DISABLED_ExportImportGActions )
{
  // no actions exported

  EXPECT_EQ( 0, GetGActionCount() );

  // export actions

  m_actions_export_id = g_dbus_connection_export_action_group( m_connection.data(), c_path,
      G_ACTION_GROUP( m_actions.data() ), NULL );

  ownBus();

  // add 1 action

  GSimpleAction* action = g_simple_action_new_stateful( "new", nullptr,
      g_variant_new_boolean( false ) );
  g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action ) );

  if (m_action_added_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_added_spy.wait());
  }
  ASSERT_FALSE( m_action_added_spy.empty() );
  EXPECT_EQ( "new", m_action_added_spy.at( 0 ).at( 0 ).toString().toStdString() );

  EXPECT_NE( nullptr, m_importer.GetGActionGroup() );
  EXPECT_EQ( 1, GetGActionCount() );

  // disable / enable action

  m_action_enabled_spy.clear();
  g_simple_action_set_enabled( action, false );

  if (m_action_enabled_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_enabled_spy.wait());
  }
  ASSERT_FALSE( m_action_enabled_spy.empty() );
  EXPECT_EQ( "app.new", m_action_enabled_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "false", m_action_enabled_spy.at( 0 ).at( 1 ).toString().toStdString() );

  m_action_enabled_spy.clear();
  g_simple_action_set_enabled( action, true );

  if (m_action_enabled_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_enabled_spy.wait());
  }
  ASSERT_FALSE( m_action_enabled_spy.empty() );
  EXPECT_EQ( "app.new", m_action_enabled_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "true", m_action_enabled_spy.at( 0 ).at( 1 ).toString().toStdString() );

  // change action state

  m_action_state_changed_spy.clear();
  g_action_change_state( G_ACTION( action ), g_variant_new_boolean( true ) );

  if (m_action_state_changed_spy.isEmpty())
  {
      ASSERT_TRUE(m_action_state_changed_spy.wait());
  }
  ASSERT_FALSE( m_action_state_changed_spy.empty() );
  EXPECT_EQ( "new", m_action_state_changed_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "true", m_action_state_changed_spy.at( 0 ).at( 1 ).toString().toStdString() );

  m_action_state_changed_spy.clear();
  g_action_change_state( G_ACTION( action ), g_variant_new_boolean( false ) );

  if (m_action_state_changed_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_state_changed_spy.wait());
  }
  ASSERT_FALSE( m_action_state_changed_spy.empty() );
  EXPECT_EQ( "new", m_action_state_changed_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "false", m_action_state_changed_spy.at( 0 ).at( 1 ).toString().toStdString() );
  m_action_state_changed_spy.clear();

  // add 2 actions

  m_action_added_spy.clear();
  action = g_simple_action_new_stateful( "add", G_VARIANT_TYPE_BOOLEAN, FALSE );
  g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action ) );

  if (m_action_added_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_added_spy.wait());
  }
  ASSERT_FALSE( m_action_added_spy.empty() );
  EXPECT_EQ( "add", m_action_added_spy.at( 0 ).at( 0 ).toString().toStdString() );

  m_action_added_spy.clear();
  action = g_simple_action_new_stateful( "del", G_VARIANT_TYPE_BOOLEAN, FALSE );
  g_action_map_add_action( G_ACTION_MAP( m_actions.data() ), G_ACTION( action ) );

  if (m_action_added_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_added_spy.wait());
  }
  ASSERT_FALSE( m_action_added_spy.empty() );
  EXPECT_EQ( "del", m_action_added_spy.at( 0 ).at( 0 ).toString().toStdString() );

  EXPECT_EQ( 3, GetGActionCount() );

  // remove 1 action
  m_action_removed_spy.clear();
  g_action_map_remove_action( G_ACTION_MAP( m_actions.data() ), "del" );

  if (m_action_removed_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_removed_spy.wait());
  }
  ASSERT_FALSE( m_action_removed_spy.empty() );
  EXPECT_EQ( "del", m_action_removed_spy.at( 0 ).at( 0 ).toString().toStdString() );

  EXPECT_EQ( 2, GetGActionCount() );
}

TEST_F( TestQtGMenu, DISABLED_QMenuStructure )
{
  ExportGMenu();

  // import QMenu

  std::shared_ptr< QMenu > menu = m_importer.GetQMenu();
  ASSERT_NE( nullptr, menu );

  ASSERT_EQ( 2, menu->actions().size() );

  EXPECT_EQ( "File", menu->actions().at( 0 )->text() );
  EXPECT_EQ( "Edit", menu->actions().at( 1 )->text() );

  // check file menu structure

  QMenu* file_menu = menu->actions().at( 0 )->menu();
  ASSERT_NE( nullptr, file_menu );

  ASSERT_EQ( 4, file_menu->actions().size() );

  EXPECT_EQ( "File", file_menu->title() );

  EXPECT_EQ( "New", file_menu->actions().at( 0 )->text() );
  EXPECT_EQ( "Ctrl+N", file_menu->actions().at( 0 )->shortcut().toString().toStdString() );

  EXPECT_EQ( "Open", file_menu->actions().at( 1 )->text() );
  EXPECT_TRUE( file_menu->actions().at( 2 )->isSeparator() );
  EXPECT_EQ( "Lock", file_menu->actions().at( 3 )->text() );

  // check edit menu structure

  QMenu* edit_menu = menu->actions().at( 1 )->menu();
  ASSERT_NE( nullptr, edit_menu );

  ASSERT_EQ( 1, edit_menu->actions().size() );

  EXPECT_EQ( "Edit", edit_menu->title() );

  EXPECT_EQ( "Style", edit_menu->actions().at( 0 )->text() );

  // check style submenu structure

  QMenu* style_submenu = edit_menu->actions().at( 0 )->menu();
  ASSERT_NE( nullptr, style_submenu );

  ASSERT_EQ( 2, style_submenu->actions().size() );

  EXPECT_EQ( "Style", style_submenu->title() );

  EXPECT_EQ( "Plain", style_submenu->actions().at( 0 )->text() );
  EXPECT_EQ( "Bold", style_submenu->actions().at( 1 )->text() );
}

TEST_F( TestQtGMenu, DISABLED_QMenuActionTriggers )
{
  ExportGMenu();

  // import QMenu

  std::shared_ptr< QMenu > menu = m_importer.GetQMenu();
  EXPECT_NE( nullptr, m_importer.GetQMenu() );

  // trigger file menu items

  ASSERT_FALSE(menu->actions().empty());
  QMenu* file_menu = menu->actions().at( 0 )->menu();
  ASSERT_NE( nullptr, file_menu );

  m_action_activated_spy.clear();
  file_menu->actions().at( 0 )->trigger();

  if (m_action_activated_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_activated_spy.wait());
  }
  ASSERT_FALSE( m_action_activated_spy.empty() );
  EXPECT_EQ( "new", m_action_activated_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "", m_action_activated_spy.at( 0 ).at( 1 ).toString().toStdString() );

  m_action_activated_spy.clear();
  file_menu->actions().at( 1 )->trigger();

  if (m_action_activated_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_activated_spy.wait());
  }
  ASSERT_FALSE( m_action_activated_spy.empty() );
  EXPECT_EQ( "open", m_action_activated_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "", m_action_activated_spy.at( 0 ).at( 1 ).toString().toStdString() );

  m_action_activated_spy.clear();
  file_menu->actions().at( 3 )->trigger();

  if (m_action_activated_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_activated_spy.wait());
  }
  ASSERT_FALSE( m_action_activated_spy.empty() );
  EXPECT_EQ( "lock", m_action_activated_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "", m_action_activated_spy.at( 0 ).at( 1 ).toString().toStdString() );

  // trigger edit menu items

  QMenu* edit_menu = menu->actions().at( 1 )->menu();
  ASSERT_NE( nullptr, edit_menu );
  QMenu* style_submenu = edit_menu->actions().at( 0 )->menu();
  ASSERT_NE( nullptr, style_submenu );

  m_action_activated_spy.clear();
  style_submenu->actions().at( 0 )->trigger();

  if (m_action_activated_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_activated_spy.wait());
  }
  ASSERT_FALSE( m_action_activated_spy.empty() );
  EXPECT_EQ( "text_plain", m_action_activated_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "text_plain", m_action_activated_spy.at( 0 ).at( 1 ).toString().toStdString() );

  m_action_activated_spy.clear();
  style_submenu->actions().at( 1 )->trigger();

  if (m_action_activated_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_activated_spy.wait());
  }
  ASSERT_FALSE( m_action_activated_spy.empty() );
  EXPECT_EQ( "text_bold", m_action_activated_spy.at( 0 ).at( 0 ).toString().toStdString() );
  EXPECT_EQ( "text_bold", m_action_activated_spy.at( 0 ).at( 1 ).toString().toStdString() );
}

TEST_F( TestQtGMenu, DISABLED_QMenuActionStates )
{
  ExportGMenu();

  // import QMenu

  std::shared_ptr< QMenu > menu = m_importer.GetQMenu();
  EXPECT_NE( nullptr, m_importer.GetQMenu() );

  // enable / disable menu items

  QMenu* file_menu = menu->actions().at( 0 )->menu();
  ASSERT_NE( nullptr, file_menu );

  EXPECT_TRUE( file_menu->actions().at( 0 )->isEnabled() );

  m_action_enabled_spy.clear();
  g_simple_action_set_enabled( m_exported_actions[0].first.data(), false );
  if (m_action_enabled_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_enabled_spy.wait());
  }

  EXPECT_FALSE( file_menu->actions().at( 0 )->isEnabled() );

  m_action_enabled_spy.clear();
  g_simple_action_set_enabled( m_exported_actions[0].first.data(), true );
  if (m_action_enabled_spy.isEmpty())
  {
    ASSERT_TRUE(m_action_enabled_spy.wait());
  }

  EXPECT_TRUE( file_menu->actions().at( 0 )->isEnabled() );
}

} // namespace

#include <TestQtGMenu.moc>
