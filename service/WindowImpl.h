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

#ifndef HUD_SERVICE_WINDOWIMPL_H_
#define HUD_SERVICE_WINDOWIMPL_H_

#include <service/WindowContextImpl.h>
#include <service/DBusMenuCollector.h>
#include <service/GMenuCollector.h>
#include <service/ItemStore.h>

#include <QList>
#include <QSharedPointer>
#include <QTimer>

namespace hud {
namespace service {

class Factory;
class WindowImpl;

class Q_DECL_EXPORT WindowTokenImpl: public WindowToken {
Q_OBJECT

	friend WindowImpl;

public:
	WindowTokenImpl(const QList<CollectorToken::Ptr> &tokens, ItemStore::Ptr itemStore);

	virtual ~WindowTokenImpl();

	void search(const QString &query, Query::EmptyBehaviour emptyBehaviour,
			QList<Result> &results) override;

	void execute(unsigned long long commandId) override;

	QString executeParameterized(unsigned long long commandId, QString &prefix,
			QString &baseAction, QDBusObjectPath &actionPath,
			QDBusObjectPath &modelPath) override;

	void executeToolbar(const QString &item) override;

	QList<QStringList> commands() const override;

	QStringList toolbarItems() const override;

	const QList<CollectorToken::Ptr> & tokens() const override;

protected Q_SLOTS:
	void childChanged();

protected:
	ItemStore::Ptr m_items;

	QList<CollectorToken::Ptr> m_tokens;

	QTimer m_timer;
};

class WindowImpl: public WindowContextImpl, public Window {
	friend WindowTokenImpl;

public:
	WindowImpl(unsigned int windowId, const QString &applicationId,
			WindowContext::Ptr allWindowsContext, Factory &factory);

	virtual ~WindowImpl();

	virtual WindowToken::Ptr activate();

protected:
	QString m_applicationId;

	WindowContext::Ptr m_allWindowsContext;

	Collector::Ptr m_dbusMenuCollector;

	Collector::Ptr m_gMenuCollector;

	QWeakPointer<WindowToken> m_windowToken;
};

}
}
#endif /* HUD_SERVICE_WINDOWIMPL_H_ */
