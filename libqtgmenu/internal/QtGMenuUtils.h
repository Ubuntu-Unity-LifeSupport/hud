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

#ifndef QTGMENUUTILS_H
#define QTGMENUUTILS_H

#include <QPair>

class _GVariant;
typedef _GVariant GVariant;

class QKeySequence;
class QString;
class QVariant;

namespace qtgmenu
{

class QtGMenuUtils final
{
public:
  static QVariant GVariantToQVariant( GVariant* gvariant );
  static QKeySequence QStringToQKeySequence( QString& shortcut );
  static QPair<QString, QString> splitPrefixAndName( const QString& name );
};

} // namespace qtgmenu

#endif // QTGMENUUTILS_H
