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

#include <service/Factory.h>
#include <service/WindowImpl.h>
#include <QDebug>

using namespace hud::service;

WindowTokenImpl::WindowTokenImpl(const QList<CollectorToken::Ptr> &tokens,
		ItemStore::Ptr itemStore) :
		m_items(itemStore), m_tokens(tokens) {
	m_timer.setSingleShot(true);
	connect(&m_timer, SIGNAL(timeout()), this, SIGNAL(changed()));

	for (CollectorToken::Ptr token : tokens) {
		connect(token.data(), SIGNAL(changed()), this, SLOT(childChanged()));
		m_items->indexMenu(token->menu());
	}
}

WindowTokenImpl::~WindowTokenImpl() {
}

void WindowTokenImpl::childChanged() {
	m_timer.start();
}

const QList<CollectorToken::Ptr> & WindowTokenImpl::tokens() const {
	return m_tokens;
}

void WindowTokenImpl::search(const QString &query,
		Query::EmptyBehaviour emptyBehaviour, QList<Result> &results) {
	m_items->search(query, emptyBehaviour, results);
}

void WindowTokenImpl::execute(unsigned long long commandId) {
	m_items->execute(commandId);
}

QString WindowTokenImpl::executeParameterized(unsigned long long commandId,
		QString &prefix, QString &baseAction, QDBusObjectPath &actionPath,
		QDBusObjectPath &modelPath) {
	return m_items->executeParameterized(commandId, prefix, baseAction,
			actionPath, modelPath);
}

void WindowTokenImpl::executeToolbar(const QString &item) {
	m_items->executeToolbar(item);
}

QList<QStringList> WindowTokenImpl::commands() const {
	return m_items->commands();
}

QStringList WindowTokenImpl::toolbarItems() const {
	return m_items->toolbarItems();
}

WindowImpl::WindowImpl(unsigned int windowId, const QString &applicationId,
		WindowContext::Ptr allWindowsContext, Factory &factory) :
		WindowContextImpl(factory), m_applicationId(applicationId), m_allWindowsContext(
				allWindowsContext) {

	m_dbusMenuCollector = factory.newDBusMenuWindowCollector(windowId);
	m_gMenuCollector = factory.newGMenuWindowCollector(windowId, applicationId);
}

WindowImpl::~WindowImpl() {
}

WindowToken::Ptr WindowImpl::activate() {
	WindowToken::Ptr windowToken(m_windowToken);

	QList<Collector::Ptr> collectors;
	collectors << m_dbusMenuCollector << m_gMenuCollector
			<< m_allWindowsContext->activeCollector() << activeCollector();

	QList<CollectorToken::Ptr> tokens;
	for (Collector::Ptr collector : collectors) {
		if (collector && collector->isValid()) {
			tokens.append(collector->activate());
		}
	}

	bool newToken(false);

	if (windowToken) {
		// If we have an existing token
		if (tokens != windowToken->tokens()) {
			// If any of the sub-tokens have changed
			newToken = true;
		}
	} else {
		// We don't have an existing token
		newToken = true;
	}

	if (newToken) {
		windowToken = m_factory.newWindowToken(m_applicationId, tokens);
		m_windowToken = windowToken;
		connect(this, SIGNAL(contextChanged()), windowToken.data(),
				SLOT(childChanged()));
		connect(m_allWindowsContext.data(), SIGNAL(contextChanged()),
				windowToken.data(), SLOT(childChanged()));
	}

	return windowToken;
}
