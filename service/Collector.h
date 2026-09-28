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

#ifndef HUD_SERVICE_COLLECTOR_H_
#define HUD_SERVICE_COLLECTOR_H_

#include <service/Result.h>

#include <QObject>
#include <QSharedPointer>
#include <QMenu>

#include <memory>

namespace hud {
namespace service {

class Collector;
class DBusMenuCollector;
class GMenuCollector;

class CollectorToken: public QObject {
Q_OBJECT
public:
	CollectorToken(std::shared_ptr<Collector> collector, QMenu *menu);

	typedef QSharedPointer<CollectorToken> Ptr;

	virtual ~CollectorToken();

	QMenu *menu();

Q_SIGNALS:
	void changed();

protected:

	std::weak_ptr<Collector> m_collector;

	QMenu *m_menu;
};

class Collector: public QObject {
Q_OBJECT

	friend CollectorToken;

public:
	typedef std::shared_ptr<Collector> Ptr;

	explicit Collector(QObject *parent = 0);

	virtual ~Collector();

	virtual bool isValid() const = 0;

	virtual QList<CollectorToken::Ptr> activate() = 0;

protected:
	virtual void deactivate() = 0;
};

}
}

#endif /* HUD_SERVICE_COLLECTOR_H_ */
