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

#include <QtGActionGroup.h>
#include <QtGMenuUtils.h>

using namespace qtgmenu;

QtGActionGroup::QtGActionGroup( QSharedPointer<GDBusConnection> connection,
        const QString& action_prefix,
        const QString& service,
        const QString& path )
    : m_action_prefix( action_prefix ),
      m_action_group(
                G_ACTION_GROUP(
                        g_dbus_action_group_get(connection.data(),
                                service.toUtf8().constData(),
                                path.toUtf8().constData())), &g_object_unref)
{
  ConnectCallbacks();

  auto actions_list = g_action_group_list_actions( m_action_group.data() );
  if( actions_list )
  {
    for( int i = 0; actions_list[i]; ++i )
    {
      emit ActionAdded( actions_list[i] );
    }

    g_strfreev( actions_list );
  }
}

QtGActionGroup::~QtGActionGroup()
{
  if( m_action_group == nullptr )
  {
    return;
  }

  auto actions_list = g_action_group_list_actions( m_action_group.data() );
  if( actions_list )
  {
    for( int i = 0; actions_list[i]; ++i )
    {
      emit ActionRemoved( actions_list[i] );
    }

    g_strfreev( actions_list );
  }

  DisconnectCallbacks();
}

QSharedPointer<GActionGroup> QtGActionGroup::ActionGroup() const
{
  return m_action_group;
}

void QtGActionGroup::TriggerAction( QString action_name, bool checked )
{
  QPair<QString, QString> split = QtGMenuUtils::splitPrefixAndName(action_name);
  const QString& prefix(split.first);

  if( prefix != m_action_prefix )
  {
    return;
  }

  const QString& action(split.second);
  QByteArray action_utf = action.toUtf8();

  const GVariantType* type = g_action_group_get_action_parameter_type( m_action_group.data(),
		  action_utf.constData() );

  if( type == nullptr )
  {
    g_action_group_activate_action( m_action_group.data(), action_utf.constData(), nullptr );
  }
  else
  {
    ///! need to evaluate and send parameter value
    if( g_variant_type_equal( type, G_VARIANT_TYPE_STRING ) )
    {
      g_action_group_activate_action( m_action_group.data(), action_utf.constData(), g_variant_new_string( action_utf.constData() ) );
    }
  }
}

QString QtGActionGroup::FullName( const QString& prefix, const QString& action_name )
{
  if ( prefix.isEmpty() )
  {
    return action_name;
  }
  else
  {
    return prefix + "." + action_name;
  }
}

void QtGActionGroup::EmitStates()
{
  auto actions_list = g_action_group_list_actions( m_action_group.data() );

  for( int i = 0; actions_list && actions_list[i]; ++i )
  {
    gchar* action_name = actions_list[i];

    bool enabled = G_ACTION_GROUP_GET_IFACE( m_action_group.data() ) ->get_action_enabled( m_action_group.data(),
        action_name );
    emit ActionEnabled( FullName(m_action_prefix, action_name), enabled );

    const GVariantType* type = g_action_group_get_action_parameter_type( m_action_group.data(),
        action_name );
    emit ActionParameterized( FullName(m_action_prefix, action_name), type != nullptr );
  }

  g_strfreev( actions_list );
}

void QtGActionGroup::ActionAddedCallback( GActionGroup* action_group, gchar* action_name,
    gpointer user_data )
{
  QtGActionGroup* self = reinterpret_cast< QtGActionGroup* >( user_data );
  emit self->ActionAdded( action_name );

  bool enabled = G_ACTION_GROUP_GET_IFACE( self->m_action_group.data() ) ->get_action_enabled(
      self->m_action_group.data(), action_name );
  if( !enabled )
    emit self->ActionEnabled( FullName(self->m_action_prefix, action_name), enabled );

  const GVariantType* type = g_action_group_get_action_parameter_type( self->m_action_group.data(),
      action_name );
  if( type != nullptr )
    emit self->ActionParameterized( FullName(self->m_action_prefix, action_name), type != nullptr );
}

void QtGActionGroup::ActionRemovedCallback( GActionGroup* action_group, gchar* action_name,
    gpointer user_data )
{
  QtGActionGroup* self = reinterpret_cast< QtGActionGroup* >( user_data );
  emit self->ActionRemoved( action_name );
}

void QtGActionGroup::ActionEnabledCallback( GActionGroup* action_group, gchar* action_name,
    gboolean enabled, gpointer user_data )
{
  QtGActionGroup* self = reinterpret_cast< QtGActionGroup* >( user_data );
  emit self->ActionEnabled( FullName(self->m_action_prefix, action_name), enabled );
}

void QtGActionGroup::ActionStateChangedCallback( GActionGroup* action_group, gchar* action_name,
    GVariant* value, gpointer user_data )
{
  QtGActionGroup* self = reinterpret_cast< QtGActionGroup* >( user_data );
  emit self->ActionStateChanged( action_name, QtGMenuUtils::GVariantToQVariant( value ) );
}

void QtGActionGroup::ConnectCallbacks()
{
  if( m_action_group && m_action_added_handler == 0 )
  {
    m_action_added_handler = g_signal_connect( m_action_group.data(), "action-added",
        G_CALLBACK( ActionAddedCallback ), this );
  }
  if( m_action_group && m_action_removed_handler == 0 )
  {
    m_action_removed_handler = g_signal_connect( m_action_group.data(), "action-removed",
        G_CALLBACK( ActionRemovedCallback ), this );
  }
  if( m_action_group && m_action_enabled_handler == 0 )
  {
    m_action_enabled_handler = g_signal_connect( m_action_group.data(), "action-enabled-changed",
        G_CALLBACK( ActionEnabledCallback ), this );
  }
  if( m_action_group && m_action_state_changed_handler == 0 )
  {
    m_action_state_changed_handler = g_signal_connect( m_action_group.data(), "action-state-changed",
        G_CALLBACK( ActionStateChangedCallback ), this );
  }
}

void QtGActionGroup::DisconnectCallbacks()
{
  if( m_action_group && m_action_added_handler != 0 )
  {
    g_signal_handler_disconnect( m_action_group.data(), m_action_added_handler );
  }
  if( m_action_group && m_action_removed_handler != 0 )
  {
    g_signal_handler_disconnect( m_action_group.data(), m_action_removed_handler );
  }
  if( m_action_group && m_action_enabled_handler != 0 )
  {
    g_signal_handler_disconnect( m_action_group.data(), m_action_enabled_handler );
  }
  if( m_action_group && m_action_state_changed_handler != 0 )
  {
    g_signal_handler_disconnect( m_action_group.data(), m_action_state_changed_handler );
  }

  m_action_added_handler = 0;
  m_action_removed_handler = 0;
  m_action_enabled_handler = 0;
  m_action_state_changed_handler = 0;
}
