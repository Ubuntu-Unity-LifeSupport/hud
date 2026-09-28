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

#include <service/Result.h>

using namespace hud::service;

Result::Result() :
		m_id(0), m_distance(0), m_parameterized(false) {
}

Result::Result(qulonglong id, const QString &commandName,
		const HighlightList &commandHighlights, const QString &description,
		const HighlightList &descriptionHighlights, const QString &shortcut,
		int distance, bool parameterized) :
		m_id(id), m_commandName(commandName), m_commandHighlights(
				commandHighlights), m_description(description), m_descriptionHighlights(
				descriptionHighlights), m_shortcut(shortcut), m_distance(
				distance), m_parameterized(parameterized) {
}

Result::~Result() {
}

qulonglong Result::id() const {
	return m_id;
}

const QString & Result::commandName() const {
	return m_commandName;
}

const Result::HighlightList & Result::commandHighlights() const {
	return m_commandHighlights;
}

const QString & Result::description() const {
	return m_description;
}

const Result::HighlightList & Result::descriptionHighlights() const {
	return m_descriptionHighlights;
}

const QString & Result::shortcut() const {
	return m_shortcut;
}

int Result::distance() const {
	return m_distance;
}

bool Result::parameterized() const {
	return m_parameterized;
}
