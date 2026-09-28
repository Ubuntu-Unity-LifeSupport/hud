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

#ifndef QTGACTIONGROUP_H
#define QTGACTIONGROUP_H

#include <QObject>
#include <QSharedPointer>
#include <QVariant>

#undef signals
#include <gio/gio.h>

namespace qtgmenu
{

class QtGActionGroup : public QObject
{
Q_OBJECT

public:
  QtGActionGroup(QSharedPointer<GDBusConnection> connection,
            const QString& action_prefix,
            const QString& service,
            const QString& path);
  virtual ~QtGActionGroup();

  QSharedPointer<GActionGroup> ActionGroup() const;

Q_SIGNALS:
  void ActionAdded( QString action_name );
  void ActionRemoved( QString action_name );
  void ActionEnabled( QString action_name, bool enabled );
  void ActionParameterized( QString action_name, bool parameterized );
  void ActionStateChanged( QString action_name, QVariant value );

private Q_SLOTS:
  void TriggerAction( QString action_name, bool checked );
  void EmitStates();

private:
  static QString FullName( const QString& prefix, const QString& name );
  static void ActionAddedCallback( GActionGroup* action_group, gchar* action_name,
      gpointer user_data );
  static void ActionRemovedCallback( GActionGroup* action_group, gchar* action_name,
      gpointer user_data );
  static void ActionEnabledCallback( GActionGroup* action_group, gchar* action_name,
      gboolean enabled, gpointer user_data );
  static void ActionStateChangedCallback( GActionGroup* action_group, gchar* action_name,
      GVariant* value, gpointer user_data );

  void ConnectCallbacks();
  void DisconnectCallbacks();

private:

  QString m_action_prefix;
  QSharedPointer<GActionGroup> m_action_group;

  gulong m_action_added_handler = 0;
  gulong m_action_removed_handler = 0;
  gulong m_action_enabled_handler = 0;
  gulong m_action_state_changed_handler = 0;
};

} // namespace qtgmenu

#endif // QTGACTIONGROUP_H
