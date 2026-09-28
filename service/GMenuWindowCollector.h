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

#ifndef HUD_SERVICE_GMENUWINDOWCOLLECTOR_H_
#define HUD_SERVICE_GMENUWINDOWCOLLECTOR_H_

#include <service/Collector.h>

#include <QScopedPointer>

class ComCanonicalUnityWindowStackInterface;

namespace qtgmenu {
class QtGMenuImporter;
}

namespace hud {
namespace service {

class Factory;

class GMenuWindowCollector: public Collector,
		public std::enable_shared_from_this<GMenuWindowCollector> {
public:
	typedef std::shared_ptr<GMenuCollector> Ptr;

	GMenuWindowCollector(unsigned int windowId, const QString &applicationId,
			QSharedPointer<ComCanonicalUnityWindowStackInterface> windowStack,
			Factory &factory);

	virtual ~GMenuWindowCollector();

	virtual bool isValid() const override;

	virtual QList<CollectorToken::Ptr> activate() override;

protected:
	virtual void deactivate();

	QSharedPointer<ComCanonicalUnityWindowStackInterface> m_windowStack;

	QString m_busName;

	QList<Collector::Ptr> m_collectors;
};

}
}

#endif /* HUD_SERVICE_GMENUWINDOWCOLLECTOR_H_ */
