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

#include <service/HardCodedSearchSettings.h>

using namespace hud::service;

HardCodedSearchSettings::HardCodedSearchSettings() {
}

HardCodedSearchSettings::~HardCodedSearchSettings() {
}

uint HardCodedSearchSettings::addPenalty() const {
	return m_addPenalty;
}

uint HardCodedSearchSettings::dropPenalty() const {
	return m_dropPenalty;
}

uint HardCodedSearchSettings::endDropPenalty() const {
	return m_endDropPenalty;
}

uint HardCodedSearchSettings::swapPenalty() const {
	return m_swapPenalty;
}

void HardCodedSearchSettings::setAddPenalty(uint penalty) {
	if (m_addPenalty != penalty) {
		m_addPenalty = penalty;
		changed();
	}
}

void HardCodedSearchSettings::setDropPenalty(uint penalty) {
	if (m_dropPenalty != penalty) {
		m_dropPenalty = penalty;
		changed();
	}
}

void HardCodedSearchSettings::setEndDropPenalty(uint penalty) {
	if (m_endDropPenalty != penalty) {
		m_endDropPenalty = penalty;
		changed();
	}
}

void HardCodedSearchSettings::setSwapPenalty(uint penalty) {
	if (m_swapPenalty != penalty) {
		m_swapPenalty = penalty;
		changed();
	}
}
