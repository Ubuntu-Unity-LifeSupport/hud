/*
 * Copyright (C) 2016 Canonical, Ltd.
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
 * Author: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef HUD_SERVICE_DBUSMENUWINDOWCOLLECTOR_H_
#define HUD_SERVICE_DBUSMENUWINDOWCOLLECTOR_H_

#include <service/Collector.h>

class ComCanonicalAppMenuRegistrarInterface;
class ComCanonicalUnityWindowStackInterface;

namespace qtgmenu {
class QtGMenuImporter;
}

namespace hud {
namespace service {

class Factory;

class DBusMenuWindowCollector: public Collector,
		public std::enable_shared_from_this<DBusMenuWindowCollector> {
Q_OBJECT
public:
	typedef std::shared_ptr<DBusMenuWindowCollector> Ptr;

	DBusMenuWindowCollector(unsigned int windowId,
		QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack,
		QSharedPointer<ComCanonicalAppMenuRegistrarInterface> registrar,
		Factory &factory);
	virtual ~DBusMenuWindowCollector();

	virtual bool isValid() const override;
	virtual QList<CollectorToken::Ptr> activate() override;

protected Q_SLOTS:
	void WindowRegistered(uint windowId, const QString &service,
		const QDBusObjectPath &menuObjectPath);

protected:
    virtual void deactivate() override;

private:
    void windowRegistered(const QString &service,
		const QDBusObjectPath &menuObjectPath);

	Collector::Ptr m_am_collector;
	Collector::Ptr m_collector;
	unsigned int m_windowId;
	QSharedPointer<ComCanonicalAppMenuRegistrarInterface> m_registrar;
	Factory &m_factory;
};

}
}

#endif /* HUD_SERVICE_DBUSMENUWINDOWCOLLECTOR_H_ */
