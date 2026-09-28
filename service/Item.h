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

#ifndef HUD_SERVICE_ITEM_H_
#define HUD_SERVICE_ITEM_H_

#include <QSharedPointer>
#include <QMenu>
#include <QList>

namespace hud {
namespace service {

class Item {
public:
	typedef QSharedPointer<Item> Ptr;

	Item(const QMenu *root, const QList<int> &index, int lastIndex);

	virtual ~Item();

	QList<QAction *> context() const;

	QAction *action() const;

protected:
	const QMenu *m_root;

	QList<int> m_index;

	int m_lastIndex;
};

}
}

#endif /* HUD_SERVICE_ITEM_H_ */
