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

#ifndef QTGMENUEXPORTER_H
#define QTGMENUEXPORTER_H

#include <QObject>
#include <QDBusConnection>
#include <memory>

class QAction;
class QMenu;

namespace qtgmenu
{

class QtGMenuExporterPrivate;

class QtGMenuExporter final : public QObject
{
Q_OBJECT

public:
  QtGMenuExporter( const QString& dbusObjectPath, QMenu* menu, const QDBusConnection& connection =
      QDBusConnection::sessionBus() );
  virtual ~QtGMenuExporter();

private Q_SLOTS:

private:
  Q_DISABLE_COPY(QtGMenuExporter)
  std::unique_ptr< QtGMenuExporterPrivate > d;
};

} // namespace qtgmenu

#endif // QTGMENUEXPORTER_H
