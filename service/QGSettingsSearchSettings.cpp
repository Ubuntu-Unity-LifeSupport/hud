/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <service/QGSettingsSearchSettings.h>

#include <QVariant>

using namespace hud::service;

QGSettingsSearchSettings::QGSettingsSearchSettings() :
		m_settings("com.canonical.indicator.appmenu.hud.search",
				"/com/canonical/indicator/appmenu/hud/search/") {

	connect(&m_settings, SIGNAL(changed(const QString&)), this,
			SIGNAL(changed()));
}

QGSettingsSearchSettings::~QGSettingsSearchSettings() {
}

uint QGSettingsSearchSettings::addPenalty() const {
	return m_settings.get("addPenalty").toUInt();
}

uint QGSettingsSearchSettings::dropPenalty() const {
	return m_settings.get("dropPenalty").toUInt();
}

uint QGSettingsSearchSettings::endDropPenalty() const {
	return m_settings.get("endDropPenalty").toUInt();
}

uint QGSettingsSearchSettings::swapPenalty() const {
	return m_settings.get("swapPenalty").toUInt();
}
