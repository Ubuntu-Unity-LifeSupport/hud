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

#ifndef HUD_COMMON_SUGGESTION_H_
#define HUD_COMMON_SUGGESTION_H_

#include <QObject>
#include <QDBusArgument>

namespace hud {
namespace common {

/**
 * sssssv
 * Description
 * Icon
 * 3 Blanks ??
 * ID
 */
class Q_DECL_EXPORT Suggestion {

public:
	explicit Suggestion();

	explicit Suggestion(qulonglong id, const QString &commandName,
			const QList<QPair<int, int>> &commandHighlights,
			const QString &description,
			const QList<QPair<int, int>> &descriptionHighlights,
			const QString &icon);

	virtual ~Suggestion();

	static void registerMetaTypes();

	QString m_description;

	QString m_icon;

	QString m_unknown1;

	QString m_unknown2;

	QString m_unknown3;

	QVariant m_id;
};

}
}

Q_DECL_EXPORT
QDBusArgument &operator<<(QDBusArgument &argument,
		const hud::common::Suggestion &suggestion);

Q_DECL_EXPORT
const QDBusArgument &operator>>(const QDBusArgument &argument,
		hud::common::Suggestion &suggestion);

Q_DECLARE_METATYPE(hud::common::Suggestion)

#endif /* HUD_COMMON_SUGGESTION_H_ */
