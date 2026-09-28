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

#ifndef HUD_SERVICE_APPLICATION_H_
#define HUD_SERVICE_APPLICATION_H_

#include <service/Window.h>
#include <service/Application.h>

#include <QSharedPointer>
#include <QDBusObjectPath>

namespace hud {
namespace service {

class Application: public QObject {
Q_OBJECT

public:
	static const unsigned int WINDOW_ID_ALL_WINDOWS;

	typedef QSharedPointer<Application> Ptr;

	explicit Application(QObject *parent = 0);

	virtual ~Application();

	virtual const QString & id() const = 0;

	virtual const QString & icon() = 0;

	virtual void addWindow(unsigned int windowId) = 0;

	virtual void removeWindow(unsigned int windowId) = 0;

	virtual Window::Ptr window(unsigned int windowId) = 0;

	virtual bool isEmpty() const = 0;

	virtual const QDBusObjectPath & path() const = 0;
};

}
}

#endif /* HUD_SERVICE_APPLICATION_H_ */
