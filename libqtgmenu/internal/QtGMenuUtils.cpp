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

#include <QtGMenuUtils.h>

#include <QtCore>
#include <QKeySequence>

#undef signals
#include <gio/gio.h>

using namespace qtgmenu;

static QVariant makeBoolQVariant( GVariant* gvariant )
{
  bool value = ( g_variant_get_boolean( gvariant ) ? true : false );
  return QVariant::fromValue( value );
}

static QVariant makeIntQVariant( GVariant* gvariant )
{
  qint32 value = 0;

  if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_INT16 ) )
    value = g_variant_get_int16( gvariant );
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_INT32 ) )
    value = g_variant_get_int32( gvariant );

  return QVariant::fromValue( value );
}

static QVariant makeUIntQVariant( GVariant* gvariant )
{
  quint32 value = 0;

  if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_BYTE ) )
    value = g_variant_get_byte( gvariant );
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_UINT16 ) )
    value = g_variant_get_uint16( gvariant );
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_UINT32 ) )
    value = g_variant_get_uint32( gvariant );

  return QVariant::fromValue( value );
}

static QVariant makeLongLongQVariant( GVariant* gvariant )
{
  qint64 value = g_variant_get_int64( gvariant );
  return QVariant::fromValue( value );
}

static QVariant makeULongLongQVariant( GVariant* gvariant )
{
  quint64 value = g_variant_get_uint64( gvariant );
  return QVariant::fromValue( value );
}

static QVariant makeStringQVariant( GVariant* gvariant )
{
  const char* string = NULL;
  gsize length = 0;
  string = g_variant_get_string( gvariant, &length );

  QString value( QString::fromUtf8( string, length ) );
  return QVariant::fromValue( value );
}

static QVariant makeStringListQVariant( GVariant* gvariant )
{
  int i = 0;
  const gchar* *stringArray = g_variant_get_strv( gvariant, NULL );
  QStringList value;

  if( !stringArray )
    return QVariant::fromValue( value );

  while( stringArray[i] != NULL )
    value << QString::fromUtf8( stringArray[i++] );

  g_free (const_cast<gchar**>(stringArray));

  return QVariant::fromValue( value );
}

static QVariant makeVariantQVariant( GVariant* gvariant )
{
  return QVariant::fromValue( QtGMenuUtils::GVariantToQVariant( gvariant ) );
}

static QVariant makeByteArrayQVariant( GVariant* gvariant )
{
  QByteArray value;
  GVariantIter iter;
  guchar* byte;

  g_variant_iter_init( &iter, gvariant );
  while( g_variant_iter_loop( &iter, "y", &byte ) )
    value.append( *byte );

  return QVariant::fromValue( value );
}

static QVariant makeVariantListQVariant( GVariant* gvariant )
{
  QVariantList value;
  GVariantIter iter;
  GVariant* var;

  g_variant_iter_init( &iter, gvariant );
  while( g_variant_iter_loop( &iter, "v", &var ) )
    value.append( QtGMenuUtils::GVariantToQVariant( var ) );

  return QVariant::fromValue( value );
}

static QVariant makeVariantMapQVariant( GVariant* gvariant )
{
  QVariantMap value;
  GVariantIter iter;
  gchar* key;
  GVariant* var;

  g_variant_iter_init( &iter, gvariant );
  while( g_variant_iter_loop( &iter, "{sv}", &key, &var ) )
    value.insert( QString::fromUtf8( key ), QtGMenuUtils::GVariantToQVariant( var ) );

  return QVariant::fromValue( value );
}

QVariant QtGMenuUtils::GVariantToQVariant( GVariant* gvariant )
{
  if( !gvariant )
  {
    return QVariant();
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_BOOLEAN ) )
  {
    return makeBoolQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_BYTE )
      || g_variant_is_of_type( gvariant, G_VARIANT_TYPE_UINT16 )
      || g_variant_is_of_type( gvariant, G_VARIANT_TYPE_UINT32 ) )
  {
    return makeUIntQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_INT16 )
      || g_variant_is_of_type( gvariant, G_VARIANT_TYPE_INT32 ) )
  {
    return makeIntQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_INT64 ) )
  {
    return makeLongLongQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_UINT64 ) )
  {
    return makeLongLongQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_DOUBLE ) )
  {
    return makeULongLongQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_STRING ) )
  {
    return makeStringQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE( "as" ) ) )
  {
    return makeStringListQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE_VARIANT ) )
  {
    return makeVariantQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE( "ay" ) ) )
  {
    return makeByteArrayQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE( "av" ) ) )
  {
    return makeVariantListQVariant( gvariant );
  }
  else if( g_variant_is_of_type( gvariant, G_VARIANT_TYPE( "a{sv}" ) ) )
  {
    return makeVariantMapQVariant( gvariant );
  }
  else
  {
    qCritical( ) << "Cannot convert the type " << g_variant_get_type_string( gvariant )
        << " into QVariant";
  }

  return QVariant();
}

QKeySequence QtGMenuUtils::QStringToQKeySequence( QString& shortcut )
{
  if( shortcut.isEmpty() )
  {
    return QKeySequence();
  }

  shortcut.replace( "<primary>", "Ctrl+", Qt::CaseInsensitive );
  shortcut.replace( "<control>", "Ctrl+", Qt::CaseInsensitive );
  shortcut.replace( "<shift>", "Shift+", Qt::CaseInsensitive );
  shortcut.replace( "<alt>", "Alt+", Qt::CaseInsensitive );
  shortcut.replace( "<meta>", "Meta+", Qt::CaseInsensitive );
  shortcut.replace( "<super>", "Super+", Qt::CaseInsensitive );
  shortcut.replace( "<hyper>", "Hyper+", Qt::CaseInsensitive );
  shortcut.replace( "plus", "+", Qt::CaseInsensitive );
  shortcut.replace( "minus", "-", Qt::CaseInsensitive );
  shortcut.replace( "page_up", "PgUp", Qt::CaseInsensitive );
  shortcut.replace( "page_down", "PgDown", Qt::CaseInsensitive );

  return QKeySequence::fromString( shortcut );
}

QPair<QString, QString> QtGMenuUtils::splitPrefixAndName( const QString& name )
{
  int index = name.indexOf( '.' );

  if( index == -1 )
  {
    return qMakePair(QString(), name);
  }
  else
  {
    return qMakePair(name.left( index ), name.mid( index + 1 ));
  }
}
