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

#ifndef HUD_SERVICE_GMENUCOLLECTOR_H_
#define HUD_SERVICE_GMENUCOLLECTOR_H_

#include <service/Collector.h>

#include <QScopedPointer>

class ComCanonicalUnityWindowStackInterface;

namespace qtgmenu {
class QtGMenuImporter;
}

namespace hud {
namespace service {

class Factory;

class Q_DECL_EXPORT GMenuCollector: public Collector,
		public std::enable_shared_from_this<GMenuCollector> {
Q_OBJECT

public:
	typedef std::shared_ptr<GMenuCollector> Ptr;

	GMenuCollector(const QString &name,
			const QMap<QString, QDBusObjectPath> &actions,
			const QDBusObjectPath &menuPath,
			Factory& factory);

	virtual ~GMenuCollector();

	virtual bool isValid() const override;

	virtual QList<CollectorToken::Ptr> activate() override;

protected Q_SLOTS:
	void menuItemsChanged();

protected:
	void deactivate();

	QWeakPointer<CollectorToken> m_collectorToken;

	QString m_name;

	QMap<QString, QDBusObjectPath> m_actions;

	QDBusObjectPath m_menuPath;

	QSharedPointer<qtgmenu::QtGMenuImporter> m_importer;

	std::shared_ptr<QMenu> m_menu;
};

}
}

#endif /* HUD_SERVICE_GMENUCOLLECTOR_H_ */
