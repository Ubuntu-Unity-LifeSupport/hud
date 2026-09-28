/*
 * Copyright (C) 2012, 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HUDCLIENT_H
#define HUDCLIENT_H

#include <QObject>
#include <QScopedPointer>

#include <libhud-client/toolbar-items.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QAbstractListModel;
class QString;
QT_END_NAMESPACE

namespace hud {
namespace client {

class Q_DECL_EXPORT HudClient: public QObject {
	class Priv;
	friend Priv;

Q_OBJECT

public:
	explicit HudClient();

	~HudClient();

	Q_PROPERTY(QAbstractListModel* results READ results)
	QAbstractListModel * results() const;

	Q_PROPERTY(QAbstractListModel* appstack READ appstack)
	QAbstractListModel * appstack() const;

	Q_PROPERTY(QAbstractListModel* toolBarModel READ toolBarModel)
	QAbstractListModel * toolBarModel() const;

	Q_INVOKABLE
	void executeCommand(int index);

	Q_INVOKABLE
	void setQuery(const QString &newQuery);

	Q_INVOKABLE
	void setAppstackApp(const QString &applicationId);

	Q_INVOKABLE
	void startVoiceQuery();

	Q_INVOKABLE
	void executeParametrizedAction(const QVariant &values);

	Q_INVOKABLE
	void updateParametrizedAction(const QVariant &values);

	Q_INVOKABLE
	void cancelParametrizedAction();

	Q_INVOKABLE
	void executeToolBarAction(HudClientQueryToolbarItems action);

Q_SIGNALS:
	void voiceQueryLoading();

	void voiceQueryListening();

	void voiceQueryHeardSomething();

	void voiceQueryFailed();

	void voiceQueryFinished(const QString &query);

	void commandExecuted();

	void showParametrizedAction(const QString &action, const QVariant &items);

	void modelsChanged();

protected:
	QScopedPointer<Priv> p;
};

}
}

Q_DECLARE_METATYPE(HudClientQueryToolbarItems)

#endif
