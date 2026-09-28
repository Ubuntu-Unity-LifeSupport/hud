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

#ifndef HUD_SERVICE_ITEMSTORE_H_
#define HUD_SERVICE_ITEMSTORE_H_

#include <service/Item.h>
#include <service/Query.h>
#include <service/Result.h>
#include <service/SearchSettings.h>
#include <service/UsageTracker.h>

#include <QSharedPointer>
#include <QMenu>
#include <QStringList>
#include <Corpus.hh>
#include <Matcher.hh>

QT_BEGIN_NAMESPACE
class QDBusObjectPath;
QT_END_NAMESPACE

namespace hud {
namespace service {

class ItemStore: public QObject {
Q_OBJECT

public:
	typedef QSharedPointer<ItemStore> Ptr;

	ItemStore(const QString &applicationId, UsageTracker::Ptr usageTracker,
			SearchSettings::Ptr searchSettings);

	virtual ~ItemStore();

	void indexMenu(const QMenu *menu);

	void search(const QString &query, Query::EmptyBehaviour emptyBehaviour,
			QList<Result> &results);

	void execute(unsigned long long commandId);

	QString executeParameterized(unsigned long long commandId,
			QString &prefix, QString &baseAction, QDBusObjectPath &actionPath,
			QDBusObjectPath &modelPath);

	void executeToolbar(const QString &item);

	QList<QStringList> commands() const;

	QStringList toolbarItems() const;

protected Q_SLOTS:
	void settingChanged();

protected:
	void indexMenu(const QMenu *menu, const QMenu *root,
			const QStringList &stack, const QList<int> &index);

	void addResult(DocumentID id, const QStringMatcher &stringMatcher,
			const int queryLength, const double relevancy,
			QList<Result> &results);

	void executeItem(Item::Ptr item);

	Columbus::Corpus m_corpus;

	Columbus::Matcher m_matcher;

	QString m_applicationId;

	UsageTracker::Ptr m_usageTracker;

	DocumentID m_nextId;

	SearchSettings::Ptr m_settings;

	QMap<DocumentID, Item::Ptr> m_items;

	QMap<QString, Item::Ptr> m_toolbarItems;

	QMap<QChar, int> m_mnemonic2DocumentId;
};

}
}
#endif /* HUD_SERVICE_ITEMSTORE_H_ */
