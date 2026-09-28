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

#include <QtGMenuModel.h>
#include <QtGMenuUtils.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>

using namespace qtgmenu;

static const int MAX_NUM_CHILDREN = 100;
static const QRegularExpression SINGLE_UNDERSCORE("(?<![_])[_](?![_])");

QtGMenuModel::QtGMenuModel( QSharedPointer<GDBusConnection> connection, const QString& bus_name,
                            const QString& menu_path, const QMap<QString, QDBusObjectPath>& action_paths )
    : QtGMenuModel( QSharedPointer<GMenuModel>(G_MENU_MODEL( g_dbus_menu_model_get( connection.data(),
                                                         bus_name.toUtf8().constData(),
                                                         menu_path.toUtf8().constData() ) ), &g_object_unref ),
                    LinkType::Root, nullptr, 0 )
{
  m_connection = connection;
  m_bus_name = bus_name;
  m_menu_path = menu_path;
  m_action_paths = action_paths;
}

QtGMenuModel::QtGMenuModel( QSharedPointer<GMenuModel> model, LinkType link_type, QtGMenuModel* parent, int index )
    : m_parent( parent ),
      m_model( model ),
      m_link_type( link_type ),
      m_menu( new QMenu() ),
      m_ext_menu( new QMenu() )
{
  ConnectCallback();

  if( m_parent )
  {
    m_connection = m_parent->m_connection;
    m_bus_name = m_parent->m_bus_name;
    m_menu_path = m_parent->m_menu_path;
    m_action_paths = m_parent->m_action_paths;

    gchar* label = NULL;
    if( g_menu_model_get_item_attribute( m_parent->m_model.data(), index,
            G_MENU_ATTRIBUTE_LABEL, "s", &label ) )
    {
      QString qlabel = QString::fromUtf8( label );
      qlabel.replace( SINGLE_UNDERSCORE, "&" );
      g_free( label );

      m_ext_menu->setTitle( qlabel );
    }

    gchar* action_name = NULL;
    QString qaction_name;
    if( g_menu_model_get_item_attribute( m_parent->m_model.data(), index,
            G_MENU_ATTRIBUTE_ACTION, "s", &action_name ) )
    {
      qaction_name = QString::fromUtf8( action_name );
      g_free( action_name );

      m_ext_menu->menuAction()->setProperty( c_property_actionName, qaction_name );
    }

    // if this model has a "commitLabel" property, it is a libhud parameterized action
    gchar* commit_label = NULL;
    if( g_menu_model_get_item_attribute( m_parent->m_model.data(), index,
            "commitLabel", "s", &commit_label ) )
    {
      g_free( commit_label );

      // is parameterized
      m_ext_menu->menuAction()->setProperty( c_property_isParameterized, true );

      // dbus paths
      m_ext_menu->menuAction()->setProperty( c_property_busName, m_bus_name );
      m_ext_menu->menuAction()->setProperty( c_property_menuPath, m_menu_path );

      if( !qaction_name.isEmpty() )
      {
        QPair<QString, QString> split = QtGMenuUtils::splitPrefixAndName(qaction_name);
        const QString& prefix(split.first);
        if( m_action_paths.contains(prefix) )
        {
          m_ext_menu->menuAction()->setProperty( c_property_actionsPath, m_action_paths[prefix].path() );
        }
      }
    }
  }

  if( m_model )
  {
    m_size = std::min( MAX_NUM_CHILDREN, g_menu_model_get_n_items( m_model.data() ) );
  }

  ChangeMenuItems( 0, m_size, 0 );
}

QtGMenuModel::~QtGMenuModel()
{
  if( m_model )
  {
    if( m_size > 0 )
    {
      ChangeMenuItems( 0, 0, m_size );
    }
    DisconnectCallback();
  }

  m_children.clear();
}

QSharedPointer<GMenuModel> QtGMenuModel::Model() const
{
  return m_model;
}

QtGMenuModel::LinkType QtGMenuModel::Type() const
{
  return m_link_type;
}

QtGMenuModel* QtGMenuModel::Parent() const
{
  return m_parent;
}

QSharedPointer<QtGMenuModel> QtGMenuModel::Child( int index ) const
{
  if( m_children.contains( index ) )
  {
    return m_children.value( index );
  }

  return QSharedPointer<QtGMenuModel>();
}

std::shared_ptr< QMenu > QtGMenuModel::GetQMenu()
{
  auto top_menu = std::make_shared< QMenu >();

  AppendQMenu( top_menu );

  return top_menu;
}

void QtGMenuModel::ActionTriggered( bool checked )
{
  QAction* action = dynamic_cast< QAction* >( QObject::sender() );
  emit ActionTriggered( action->property( c_property_actionName ).toString(), checked );
}

void QtGMenuModel::ActionEnabled( QString action_name, bool enabled )
{
  auto action_it = m_actions.find( action_name );
  if( action_it != end( m_actions ) )
  {
    for( auto& action : action_it->second )
    {
      action->setEnabled( enabled );
    }
  }
}

void QtGMenuModel::ActionParameterized( QString action_name, bool parameterized )
{
  auto action_it = m_actions.find( action_name );
  if( action_it != end( m_actions ) )
  {
    for( auto& action : action_it->second )
    {
      action->setProperty( c_property_isParameterized, parameterized );
    }
  }
}

QSharedPointer<QtGMenuModel> QtGMenuModel::CreateChild( QtGMenuModel* parent_qtgmenu, QSharedPointer<GMenuModel> parent_gmenu, int child_index )
{
  QSharedPointer<QtGMenuModel> new_child;

  GMenuLinkIter* link_it = g_menu_model_iterate_item_links( parent_gmenu.data(), child_index );

  // get the first link, if it exists, create the child accordingly
  if( link_it && g_menu_link_iter_next( link_it ) )
  {
    // if link is a sub menu
    if( strcmp( g_menu_link_iter_get_name( link_it ), G_MENU_LINK_SUBMENU ) == 0 )
    {
      new_child.reset(
                new QtGMenuModel(
                        QSharedPointer<GMenuModel>(
                                g_menu_link_iter_get_value(link_it),
                                &g_object_unref), LinkType::SubMenu,
                        parent_qtgmenu, child_index));
    }
    // else if link is a section
    else if( strcmp( g_menu_link_iter_get_name( link_it ), G_MENU_LINK_SECTION ) == 0 )
    {
      new_child.reset(
                    new QtGMenuModel(
                            QSharedPointer<GMenuModel>(
                                    g_menu_link_iter_get_value(link_it),
                                    &g_object_unref), LinkType::Section,
                            parent_qtgmenu, child_index));
    }
  }

  g_object_unref( link_it );
  return new_child;
}

void QtGMenuModel::MenuItemsChangedCallback( GMenuModel* model, gint index, gint removed,
    gint added, gpointer user_data )
{
  QtGMenuModel* self = reinterpret_cast< QtGMenuModel* >( user_data );

  if( self->m_model != model )
  {
    qWarning() << "\"items-changed\" signal received from an unrecognised menu model";
    return;
  }

  self->ChangeMenuItems( index, added, removed );
}

void QtGMenuModel::ChangeMenuItems( const int index, int added, const int removed )
{
  const int n_items = g_menu_model_get_n_items( m_model.data() );

  if( index < 0 || added < 0 || removed < 0 || index + added > n_items || index + removed > m_size )
  {
    ReportRecoverableError(index, added, removed);
    return;
  }

  // Limit the menus exported to avoid memory wastage
  if ( m_size + (added - removed) > MAX_NUM_CHILDREN )
  {
    added = MAX_NUM_CHILDREN - (m_size - removed);
  }

  // process removed items first (see "items-changed" on the GMenuModel man page)
  if( removed > 0 )
  {
    // remove QAction from 'index' of our QMenu, 'removed' times
    for( int i = 0; i < removed; ++i )
    {
      if( index < m_menu->actions().size() )
      {
        QAction* at_action = m_menu->actions().at( index );
        ActionRemoved( at_action->property( c_property_actionName ).toString(), at_action );
        m_menu->removeAction( at_action );
      }
    }

    // update m_children
    for( int i = index; i < m_size; ++i )
    {
      // remove children from index until ( index + removed )
      if( i < ( index + removed ) )
      {
        m_children.take( i );
      }
      // shift children from ( index + removed ) to m_size into the now empty positions
      else if( m_children.contains( i ) )
      {
        m_children.insert( i - removed, m_children.take( i ) );
      }
    }

    // update m_size
    m_size -= removed;
  }

  // now process added items
  if( added > 0 )
  {
    // update m_children (start from the end and work backwards as not to overlap items as we shift them up)
    for( int i = m_size - 1; i >= index; --i )
    {
      // shift 'added' items up from their current index to ( index + added )
      if( m_children.contains( i ) )
      {
        m_children.insert( i + added, m_children.take( i ) );
      }
    }

    // update m_size
    m_size += added;

    // now add a new QAction to our QMenu for each new item
    for( int i = index; i < ( index + added ); ++i )
    {
      QAction* at_action = nullptr;
      if( i < m_menu->actions().size() )
      {
        at_action = m_menu->actions().at( i );
      }

      // try first to create a child model
      QSharedPointer< QtGMenuModel > model = CreateChild( this, m_model, i );

      // if this is a menu item and not a model
      if( !model )
      {
        QAction* new_action = CreateAction( i );
        ActionAdded( new_action->property( c_property_actionName ).toString(), new_action );
        m_menu->insertAction( at_action, new_action );
      }
      // else if this is a section model
      else if( model->Type() == LinkType::Section )
      {
        InsertChild( model, i );
        m_menu->insertSeparator( at_action );
      }
      // else if this is a sub menu model
      else if( model->Type() == LinkType::SubMenu )
      {
        InsertChild( model, i );
        ActionAdded( model->m_ext_menu->menuAction()->property( c_property_actionName ).toString(),
                     model->m_ext_menu->menuAction() );
        m_menu->insertMenu( at_action, model->m_ext_menu.data() );
      }
    }
  }

  // update external menu
  UpdateExtQMenu();

  // now tell the outside world that items have changed
  emit MenuItemsChanged( this, index, removed, added );
}

void QtGMenuModel::ConnectCallback()
{
  if( m_model && m_items_changed_handler == 0 )
  {
    m_items_changed_handler = g_signal_connect( m_model.data(), "items-changed",
        G_CALLBACK( MenuItemsChangedCallback ), this );
  }
}

void QtGMenuModel::DisconnectCallback()
{
  if( m_model && m_items_changed_handler != 0 )
  {
    g_signal_handler_disconnect( m_model.data(), m_items_changed_handler );
  }

  m_items_changed_handler = 0;
}

void QtGMenuModel::InsertChild( QSharedPointer<QtGMenuModel> child, int index )
{
  if( m_children.contains( index ) )
  {
    return;
  }

  child->m_parent = this;
  m_children.insert( index, child );

  connect( child.data(), SIGNAL( MenuItemsChanged( QtGMenuModel*, int, int, int ) ), this,
      SIGNAL( MenuItemsChanged( QtGMenuModel*, int, int, int ) ) );

  connect( child.data(), SIGNAL( ActionTriggered( QString, bool ) ), this,
      SIGNAL( ActionTriggered( QString, bool ) ) );

  connect( child.data(), SIGNAL( MenuInvalid() ), this, SIGNAL( MenuInvalid() ) );

  // emit signal informing subscribers that this child has added all of its menu items
  emit MenuItemsChanged( child.data(), 0, 0, child->m_size );
}

QAction* QtGMenuModel::CreateAction( int index )
{
  QAction* action = new QAction( m_menu.data() );

  // action label
  gchar* label = NULL;
  if( g_menu_model_get_item_attribute( m_model.data(), index, G_MENU_ATTRIBUTE_LABEL, "s", &label ) ) {
    QString qlabel = QString::fromUtf8( label );
    qlabel.replace( SINGLE_UNDERSCORE, "&" );
    g_free( label );

    action->setText( qlabel );
  }

  // action name
  gchar* action_name = NULL;
  if( g_menu_model_get_item_attribute( m_model.data(), index,
	      G_MENU_ATTRIBUTE_ACTION, "s", &action_name ) )
  {
    QString qaction_name = QString::fromUtf8( action_name );
    g_free( action_name );

    action->setProperty( c_property_actionName, qaction_name );
  }

  // is parameterized (set false by default until signal received)
  action->setProperty( c_property_isParameterized, false );

  // dbus paths
  action->setProperty( c_property_busName, m_bus_name );
  action->setProperty( c_property_menuPath, m_menu_path );

  // action icon
  GVariant* icon = g_menu_model_get_item_attribute_value( m_model.data(), index, G_MENU_ATTRIBUTE_ICON,
      G_VARIANT_TYPE_VARIANT );

  if( icon )
  {
    g_variant_unref( icon );
  }

  // action shortcut
  gchar* shortcut = NULL;
  if( g_menu_model_get_item_attribute( m_model.data(), index, "accel", "s", &shortcut ) )
  {
    QString qshortcut = QString::fromUtf8( shortcut );
    g_free( shortcut );

    action->setShortcut( QtGMenuUtils::QStringToQKeySequence( qshortcut ) );
  }

  // action shortcut
  gchar* toolbar_item = NULL;
  if( g_menu_model_get_item_attribute( m_model.data(), index, c_property_hud_toolbar_item, "s", &toolbar_item ) )
  {
    QString qtoolbar_item = QString::fromUtf8( toolbar_item );
    g_free( toolbar_item );

    action->setProperty( c_property_hud_toolbar_item, qtoolbar_item );
  }

  // action keywords
  gchar* keywords = NULL;
  if( g_menu_model_get_item_attribute( m_model.data(), index, c_property_keywords, "s", &keywords ) )
  {
    QVariant qkeywords = QString::fromUtf8( keywords );
    g_free( keywords );

    action->setProperty( c_property_keywords, qkeywords );
  }

  // action trigger
  connect( action, SIGNAL( triggered( bool ) ), this, SLOT( ActionTriggered( bool ) ) );

  return action;
}

void QtGMenuModel::AppendQMenu( std::shared_ptr< QMenu > top_menu )
{
  if( m_link_type == LinkType::Root )
  {
    for( QAction* action : m_ext_menu->actions() )
    {
      if( !action->menu() )
      {
        top_menu->addAction( action );
      }
    }
  }
  else if( m_link_type == LinkType::SubMenu )
  {
    top_menu->addAction( m_ext_menu->menuAction() );
  }

  if( m_link_type != LinkType::SubMenu )
  {
    for( auto& child : m_children )
    {
      child->AppendQMenu( top_menu );
    }
  }
}

void QtGMenuModel::UpdateExtQMenu()
{
  m_ext_menu->clear();

  for( int i = 0; i < m_menu->actions().size(); ++i )
  {
    QAction* action = m_menu->actions().at( i );

    if( action->isSeparator() )
    {
      QSharedPointer<QtGMenuModel> child = Child( i );
      if( !child || child->Type() != LinkType::Section )
      {
        continue;
      }

      for( QAction* sub_action : child->m_ext_menu->actions() )
      {
        m_ext_menu->addAction( sub_action );
      }
      m_ext_menu->addSeparator();
    }
    else
    {
      m_ext_menu->addAction( action );
    }
  }

  if( m_ext_menu->actions().size() > 0 )
  {
    QAction* last_action = m_ext_menu->actions().last();
    if( last_action && last_action->isSeparator() )
    {
      m_ext_menu->removeAction( last_action );
    }
  }

  // if this is a section within a parent menu, we need to update the parent menu as well
  if( m_link_type == LinkType::Section && m_parent )
  {
    m_parent->UpdateExtQMenu();
  }
}

void QtGMenuModel::ActionAdded( const QString& name, QAction* action )
{
  // add action to top menu's m_actions
  if( m_parent )
  {
    m_parent->ActionAdded( name, action );
  }
  else
  {
    // check if the action name is already in our map
    if( m_actions.find( name ) != m_actions.end() )
    {
      // add the QAction pointer to the list of actions under this name
      m_actions[name].push_back( action );
    }
    else
    {
      // otherwise insert the new action into the map
      m_actions.insert( std::make_pair( name, std::vector< QAction* >{ action } ) );
    }
  }
}

void QtGMenuModel::ActionRemoved( const QString& name, QAction* action )
{
  // remove action from top menu's m_actions
  if( m_parent )
  {
    m_parent->ActionRemoved( name, action );
  }
  else
  {
    // check if this action is actually in our map
    if( m_actions.find( name ) != m_actions.end() )
    {
      // remove the QAction pointer from the list of actions under this name
      auto& actionList = m_actions[name];
      auto actionIt = std::find( actionList.begin(), actionList.end(), action );

      if( actionIt != actionList.end())
      {
        actionList.erase( actionIt );
      }

      // if there are no more references to this action, remove it from the map
      if( actionList.size() == 0 )
      {
        m_actions.erase( name );
      }
    }
  }
}

static void write_pair(QIODevice& device, const QString& key, const QString& value, bool last = false)
{
  device.write(key.toUtf8());
  device.write("", 1);
  device.write(value.toUtf8());
  if( !last )
  {
    device.write("", 1);
  }

  if( !value.isEmpty())
  {
    qWarning() << key << " =" << value;
  }
}

void QtGMenuModel::ReportRecoverableError(const int index, const int added, const int removed)
{
  if( m_error_reported )
  {
    return;
  }

  // gmenumodel properties
  int gmenu_item_count = 0;
  QString gmenu_action_names;

  gmenu_item_count = g_menu_model_get_n_items( m_model.data() );

  qWarning() << "Illegal arguments when updating GMenuModel: position ="
             << index << ", added =" << added << ", removed =" << removed
             << ", size =" << gmenu_item_count;

  for( int i = 0; i < gmenu_item_count; ++i )
  {
    gchar* action_name = NULL;
    if( g_menu_model_get_item_attribute( m_model.data(), i,
          G_MENU_ATTRIBUTE_ACTION, "s", &action_name ) )
    {
      gmenu_action_names += action_name;
      gmenu_action_names += ";";
      g_free( action_name );
    }
  }

  // parent model properties
  QString parent_menu_label;
  QString parent_menu_name;
  QString parent_action_names;
  QString parent_link_type;

  if( m_parent )
  {
    parent_menu_label = m_parent->m_menu->menuAction()->text();
    parent_menu_name = m_parent->m_menu->menuAction()->property( c_property_actionName ).toString();

    for( QAction* action : m_parent->m_menu->actions() )
    {
      parent_action_names += action->property( c_property_actionName ).toString() + ";";
    }

    switch( m_parent->m_link_type )
    {
    case LinkType::Root:
      parent_link_type = "root";
      break;
    case LinkType::Section:
      parent_link_type = "section";
      break;
    case LinkType::SubMenu:
      parent_link_type = "sub menu";
      break;
    }
  }

  // local model properties
  QString menu_label;
  QString menu_name;
  QString action_names;
  QString link_type;
  QString action_paths;

  menu_label = m_menu->menuAction()->text();
  menu_name = m_menu->menuAction()->property( c_property_actionName ).toString();
  for( QAction* action : m_menu->actions() )
  {
    action_names += action->property( c_property_actionName ).toString() + ";";
  }

  switch( m_link_type )
  {
  case LinkType::Root:
    link_type = "root";
    break;
  case LinkType::Section:
    link_type = "section";
    break;
  case LinkType::SubMenu:
    link_type = "sub menu";
    break;
  }

  for( auto const& action : m_action_paths )
  {
    action_paths += action.path() + ";";
  }

  uint sender_pid = QDBusConnection::sessionBus().interface()->servicePid(
            m_bus_name);
  if( sender_pid == 0 ) {
      qWarning() << "Failed to read PID, cannot report error";
      return;
  }

  QProcess recoverable;
  recoverable.setProcessChannelMode(QProcess::ForwardedChannels);
  recoverable.start("/usr/share/apport/recoverable_problem",
              QStringList() << "-p" << QString::number(sender_pid));
  if (recoverable.waitForStarted())
  {
    write_pair(recoverable, "DuplicateSignature", "GMenuModelItemsChangedInvalidIndex");
    write_pair(recoverable, "BusName", m_bus_name);
    write_pair(recoverable, "Position", QString::number(index));
    write_pair(recoverable, "Added", QString::number(added));
    write_pair(recoverable, "Removed", QString::number(removed));
    write_pair(recoverable, "ItemCount", QString::number(gmenu_item_count));
    write_pair(recoverable, "ActionNames", gmenu_action_names);

    if ( m_parent )
    {
      write_pair(recoverable, "ParentMenuLabel", parent_menu_label);
      write_pair(recoverable, "ParentMenuName", parent_menu_name);
      write_pair(recoverable, "ParentActionNames", parent_action_names);
      write_pair(recoverable, "ParentLinkType", parent_link_type);
    }

    write_pair(recoverable, "MenuLabel", menu_label);
    write_pair(recoverable, "MenuName", menu_name);
    write_pair(recoverable, "ActionNames", action_names);
    write_pair(recoverable, "LinkType", link_type);

    write_pair(recoverable, "MenuPath", m_menu_path);
    write_pair(recoverable, "ActionPaths", action_paths, true);

    recoverable.closeWriteChannel();
    recoverable.waitForFinished();

    m_error_reported = true;
  }
  else
  {
    qWarning() << "Failed to report recoverable error";
  }

  emit MenuInvalid();
}
