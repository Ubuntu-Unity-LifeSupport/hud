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

#include <service/Item.h>

#include <QDebug>

using namespace hud::service;

Item::Item(const QMenu *root, const QList<int> &index, int lastIndex) :
		m_root(root), m_index(index), m_lastIndex(lastIndex) {
}

Item::~Item() {
}

QList<QAction *> Item::context() const {
	QList<QAction *> context;

	const QMenu *menu(m_root);

	const QList<int> &index(m_index);
	for (int i : index) {
		QList<QAction*> actions(menu->actions());
		if (i >= actions.size()) {
			qWarning() << "Action could not be found" << m_index << m_lastIndex;
			break;
		}
		QAction *action(actions.at(i));
		context.append(action);
		menu = action->menu();
	}

	return context;
}

QAction * Item::action() const {
	const QMenu *menu(m_root);

	const QList<int> &index(m_index);
	for (int i : index) {
		QList<QAction*> actions(menu->actions());
		if (i >= actions.size()) {
			qWarning() << "Action could not be found" << m_index << m_lastIndex;
			return nullptr;
		}
		QAction *action(actions.at(i));
		menu = action->menu();
	}

	if (menu == nullptr) {
		qWarning() << "Menu could not be found" << m_index << m_lastIndex;
		return nullptr;
	}

	QList<QAction*> actions(menu->actions());
	if (m_lastIndex >= actions.size()) {
		qWarning() << "Action could not be found" << m_index << m_lastIndex;
		return nullptr;
	}

	return actions.at(m_lastIndex);
}
