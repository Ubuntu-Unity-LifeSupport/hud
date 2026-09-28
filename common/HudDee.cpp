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

#include <common/HudDee.h>

#include <dee.h>
#include <glib-object.h>

#include <string>

using namespace std;
using namespace hud::common;

class HudDee::Priv {
public:

	Priv() :
			m_model(nullptr) {
	}

	~Priv() {
		g_clear_object(&m_model);
	}

	DeeModel *m_model;

	string m_name;
};

HudDee::HudDee(const string &name) :
		p(new Priv()) {

	p->m_name = name;
	p->m_model = dee_shared_model_new(p->m_name.data());
}

HudDee::~HudDee() {
}

const string & HudDee::name() const {
	return p->m_name;
}

void HudDee::setSchema(const char* const *columnSchemas,
		unsigned int numColumns) {
	dee_model_set_schema_full(p->m_model, columnSchemas, numColumns);
}

void HudDee::beginChangeset() {
	dee_model_begin_changeset(p->m_model);
	dee_model_clear(p->m_model);
}

void HudDee::appendRow(GVariant **row_members) {
	dee_model_append_row(p->m_model, row_members);
}

void HudDee::insertRowSorted(GVariant **row_members, CompareRowFunc cmp_func) {
	dee_model_insert_row_sorted(p->m_model, row_members, cmp_func, NULL);
}

void HudDee::endChangeset() {
//	dee_shared_model_flush_revision_queue(DEE_SHARED_MODEL(p->m_model));
	dee_model_end_changeset(p->m_model);
}
