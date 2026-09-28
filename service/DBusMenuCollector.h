/*
 * Copyright (C) 2013-2016 Canonical, Ltd.
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
 *         Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef HUD_SERVICE_DBUSMENUCOLLECTOR_H_
#define HUD_SERVICE_DBUSMENUCOLLECTOR_H_

#include <service/Collector.h>

#include <QDBusObjectPath>

class DBusMenuImporter;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace hud {
namespace service {

class DBusMenuCollector: public Collector,
	public std::enable_shared_from_this<DBusMenuCollector> {
public:
	typedef std::shared_ptr<DBusMenuCollector> Ptr;

	DBusMenuCollector(const QString &service, const QDBusObjectPath &menuObjectPath);
	virtual ~DBusMenuCollector();

	virtual bool isValid() const override;
	virtual QList<CollectorToken::Ptr> activate() override;

protected:
	virtual void deactivate() override;

	void openMenu(QMenu *menu, unsigned int &limit);
	void hideMenu(QMenu *menu, unsigned int &limit);

	QWeakPointer<CollectorToken> m_collectorToken;
	QSharedPointer<DBusMenuImporter> m_menuImporter;

	QString m_service;
	QDBusObjectPath m_path;
};

}
}

#endif /* HUD_SERVICE_DBUSMENUCOLLECTOR_H_ */
