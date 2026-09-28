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

#include "QtGMenuExporter.h"

#include <QMenu>

using namespace qtgmenu;

namespace qtgmenu
{

class QtGMenuExporterPrivate
{
};

} // namespace qtgmenu

QtGMenuExporter::QtGMenuExporter( const QString& dbusObjectPath, QMenu* menu,
    const QDBusConnection& connection )
    : QObject( menu ),
      d( new QtGMenuExporterPrivate() )
{
}

QtGMenuExporter::~QtGMenuExporter()
{
}
