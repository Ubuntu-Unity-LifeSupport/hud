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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#include <testutils/RawDBusTransformer.h>
#include <QDebug>

using namespace hud::testutils;

RawDBusTransformer::RawDBusTransformer() {

}

RawDBusTransformer::~RawDBusTransformer() {
}

QVariant RawDBusTransformer::transform(const QDBusArgument & value) {
	switch (value.currentType()) {
	case QDBusArgument::ArrayType: {
		value.beginArray();
		QVariantList list = transform(value).toList();
		value.endArray();
		return list;
	}
	case QDBusArgument::MapType: {
		QVariantMap map;
		value >> map;
		transform(map);
		return map;
	}
	case QDBusArgument::StructureType: {
		value.beginStructure();
		QVariantList list;
		while (!value.atEnd()) {
			list << value.asVariant();
		}
		value.endStructure();
		return list;
	}
	default:
		break;
	}

	return QVariant();
}
void RawDBusTransformer::transform(QVariant &variant) {
	if (variant.canConvert<QVariantList>()) {
		QVariantList list = variant.toList();
		transform(list);
		variant = list;
	} else if (variant.canConvert<QDBusArgument>()) {
		QDBusArgument value(variant.value<QDBusArgument>());
		variant = transform(value);
	}
}

void RawDBusTransformer::transform(QVariantMap &map) {
	for (auto it(map.begin()); it != map.end(); ++it) {
		transform(*it);
	}
}

void RawDBusTransformer::transform(QVariantList &list) {
	for (auto it(list.begin()); it != list.end(); ++it) {
		transform(*it);
	}
}
