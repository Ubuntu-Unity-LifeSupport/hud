/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#ifndef HUD_SERVICE_SIGNALHANDLER_H_
#define HUD_SERVICE_SIGNALHANDLER_H_

#include <QObject>
#include <QSocketNotifier>

namespace hud {
namespace service {

class SignalHandler: public QObject {
Q_OBJECT

public:
	SignalHandler(QObject *parent = 0);

	~SignalHandler() = default;

	static int setupUnixSignalHandlers();

protected Q_SLOTS:
	void handleSigInt();

	void handleSigTerm();

protected:
	static void intSignalHandler(int unused);

	static void termSignalHandler(int unused);

	static int sigintFd[2];

	static int sigtermFd[2];

	QSocketNotifier *m_socketNotifierInt;

	QSocketNotifier *m_socketNotifierTerm;
};

}
}

#endif /* HUD_SERVICE_SIGNALHANDLER_H_ */
