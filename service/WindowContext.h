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

#ifndef HUD_SERVICE_WINDOWCONTEXT_H_
#define HUD_SERVICE_WINDOWCONTEXT_H_

#include <service/Collector.h>

#include <QDBusObjectPath>
#include <QString>
#include <QSharedPointer>

#include <QMenu>

namespace qtgmenu {
class QtGMenuImporter;
}

namespace hud {
namespace service {

class WindowContext: public QObject {
Q_OBJECT

public:
	struct MenuDefinition {
		explicit MenuDefinition() {
		}

		explicit MenuDefinition(const QString &name) :
				name(name) {
		}

		bool operator==(const MenuDefinition &other) const {
			return name == other.name && actionPath == other.actionPath
					&& actionPrefix == other.actionPrefix
					&& menuPath == other.menuPath;
		}

		QString name;

		QDBusObjectPath actionPath;

		QString actionPrefix;

		QDBusObjectPath menuPath;
	};

	typedef QSharedPointer<WindowContext> Ptr;

	explicit WindowContext();

	virtual ~WindowContext();

	virtual void setContext(const QString &context) = 0;

	virtual void addMenu(const QString &context,
			const MenuDefinition &menuDefinition) = 0;

	virtual Collector::Ptr activeCollector() = 0;

Q_SIGNALS:
	void contextChanged();
};

}
}
#endif /* HUD_SERVICE_WINDOWCONTEXT_H_ */
