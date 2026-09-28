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

#ifndef HUD_COMMON_HUDDEE_H_
#define HUD_COMMON_HUDDEE_H_

#include <memory>

typedef struct _GVariant GVariant;

typedef int (*CompareRowFunc)(GVariant** row1, GVariant** row2,
		void* user_data);

namespace hud {
namespace common {

class HudDee {
public:
	explicit HudDee(const std::string &resultsName);

	virtual ~HudDee();

	const std::string & name() const;

	void beginChangeset();

	void endChangeset();

protected:
	void setSchema(const char* const *columnSchemas, unsigned int numColumns);

	void appendRow(GVariant **row_members);

	void insertRowSorted(GVariant **row_members, CompareRowFunc cmp_func);

	class Priv;

	std::shared_ptr<Priv> p;
};

}
}

#endif /* HUD_COMMON_HUDDEE_H_ */
