/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HUDTOOLBARMODEL_H
#define HUDTOOLBARMODEL_H

#include <QAbstractListModel>
#include <QScopedPointer>

typedef struct _HudClientQuery HudClientQuery;

namespace hud {
namespace client {

class HudToolBarModel: public QAbstractListModel {
	class Priv;

Q_OBJECT

public:
	explicit HudToolBarModel(HudClientQuery *query);

	~HudToolBarModel();

	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &parent, int role) const;

	QHash<int, QByteArray> roleNames() const;

	void updatedByBackend();

private:
	QScopedPointer<Priv> p;
};

}
}

#endif
