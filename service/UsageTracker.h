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

#ifndef HUD_SERVICE_USAGETRACKER_H_
#define HUD_SERVICE_USAGETRACKER_H_

#include <QObject>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace hud {
namespace service {

class UsageTracker: public QObject {
public:
	typedef QSharedPointer<UsageTracker> Ptr;

	UsageTracker();

	virtual ~UsageTracker();

	virtual void markUsage(const QString &applicationId,
			const QString &entry) = 0;

	virtual unsigned int usage(const QString &applicationId,
			const QString &entry) const = 0;
};

}
}

#endif /* HUD_SERVICE_USAGETRACKER_H_ */
