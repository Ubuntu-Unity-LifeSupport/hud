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

#include <service/Collector.h>

using namespace hud::service;

Collector::Collector(QObject *parent) :
		QObject(parent) {

}

Collector::~Collector() {
}

CollectorToken::CollectorToken(Collector::Ptr collector, QMenu *menu) :
		m_collector(collector), m_menu(menu) {
}

CollectorToken::~CollectorToken() {
	if (Collector::Ptr collector = m_collector.lock()) {
		collector->deactivate();
	}
}

QMenu * CollectorToken::menu() {
	return m_menu;
}
