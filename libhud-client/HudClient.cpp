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

#include <libhud-client/hud-client.h>
#include <libhud-client/HudClient.h>
#include <libhud-client/HudToolbarModel.h>
#include <deelistmodel.h>

#include <QDebug>

using namespace hud::client;

// Terrible hack to get around GLib. GLib stores function pointers as gpointer, which violates the C and C++ spec
// because data and function pointers may have different sizes. gcc rightfully emits a warning. There is no #pragma
// in gcc to selectively turn off the warning, however. This hack gets around the problem, by using a union (ick) to
// convert between the two types.

class ToGPointer {
public:
	ToGPointer(void (*cb)()) {
		u_.cb = cb;
	}

	operator gpointer() const{
		return u_.p;
	}

private:
	union {
		void (*cb)();
		gpointer p;
	} u_;
};

#define TO_GPOINTER(cb) (ToGPointer(reinterpret_cast<void(*)()>((cb))))

class HudClient::Priv {
public:
	explicit Priv(HudClient &client) :
			m_client(client), m_clientQuery(nullptr), m_currentActionIndex(0), m_currentActionParam(
					nullptr) {
	}

	static void loadingCB(GObject* /*src*/, gpointer dst) {
		static_cast<HudClient*>(dst)->voiceQueryLoading();
	}

	static void listeningCB(GObject* /*src*/, gpointer dst) {
		static_cast<HudClient*>(dst)->voiceQueryListening();
	}

	static void heardSomethingCB(GObject* /*src*/, gpointer dst) {
		static_cast<HudClient*>(dst)->voiceQueryHeardSomething();
	}

	static void failedCB(GObject* /*src*/, const gchar * /*reason*/,
			gpointer dst) {
		static_cast<HudClient*>(dst)->voiceQueryFailed();
	}

	static void finishedCB(GObject* /*src*/, const gchar* query, gpointer dst) {
		static_cast<HudClient*>(dst)->voiceQueryFinished(
				QString::fromUtf8(query));
	}

	static void modelReadyCB(GObject* /*src*/, gpointer dst) {
		static_cast<Priv*>(dst)->modelReady(true);
	}

	static void modelReallyReadyCB(GObject* /*src*/, gint /*position*/,
			gint /*removed*/, gint /*added*/, gpointer dst) {
		static_cast<Priv*>(dst)->modelReallyReady(true);
	}

	static void modelsChangedCB(GObject* /*src*/, gpointer dst) {
		static_cast<Priv*>(dst)->queryModelsChanged();
	}

	static void toolBarUpdatedCB(GObject* /*src*/, gpointer dst) {
		static_cast<HudToolBarModel*>(dst)->updatedByBackend();
	}

	void modelReady(bool needDisconnect) {
		if (needDisconnect) {
			g_signal_handlers_disconnect_by_func(m_currentActionParam,
					TO_GPOINTER(Priv::modelReadyCB), this);
		}
		GMenuModel *menuModel = hud_client_param_get_model(
				m_currentActionParam);
		if (g_menu_model_get_n_items(menuModel) == 0) {
			g_signal_connect(menuModel, "items-changed",
					G_CALLBACK(Priv::modelReallyReadyCB), this);
		} else {
			modelReallyReady(false);
		}
	}

	static QVariant QVariantFromGVariant(GVariant *value) {
		// Only handle the cases we care for now
		switch (g_variant_classify(value)) {
		case G_VARIANT_CLASS_BOOLEAN:
			return QVariant((bool) g_variant_get_boolean(value));
		case G_VARIANT_CLASS_DOUBLE:
			return QVariant(g_variant_get_double(value));
		case G_VARIANT_CLASS_STRING:
			return QVariant(
					QString::fromUtf8(g_variant_get_string(value, NULL)));
		default:
			return QVariant();
		}
	}

	static void addAttribute(QVariantMap &properties, GMenuModel *menuModel,
			int item, const char *attribute) {
		GVariant *v = g_menu_model_get_item_attribute_value(menuModel, item,
				attribute, NULL);

		if (v == NULL)
			return;

		properties.insert(attribute, QVariantFromGVariant(v));
		g_variant_unref(v);
	}

	void modelReallyReady(bool needDisconnect) {
		GMenuModel *menuModel = hud_client_param_get_model(
				m_currentActionParam);
		if (needDisconnect) {
			g_signal_handlers_disconnect_by_func(menuModel,
					TO_GPOINTER(Priv::modelReallyReadyCB), this);
		}

		QVariantList items;
		for (int i = 0; i < g_menu_model_get_n_items(menuModel); i++) {
			GVariant *v = g_menu_model_get_item_attribute_value(menuModel, i,
					"parameter-type", G_VARIANT_TYPE_STRING);

			if (v == NULL)
				continue;

			const QString type = QString::fromUtf8(
					g_variant_get_string(v, NULL));
			if (type == "slider") {
				const char *sliderAttributes[] = { "label", "min", "max",
						"step", "value", "live", "action" };
				QVariantMap properties;
				properties.insert("parameter-type", "slider");
				for (uint j = 0;
						j
								< sizeof(sliderAttributes)
										/ sizeof(sliderAttributes[0]); ++j) {
					addAttribute(properties, menuModel, i, sliderAttributes[j]);
				}
				items << properties;
			}
			g_variant_unref(v);
		}

		DeeModel *model = hud_client_query_get_results_model(m_clientQuery);
		DeeModelIter *iter = dee_model_get_iter_at_row(model,
				m_currentActionIndex);
		QString actionText = QString::fromUtf8(
				hud_client_query_results_get_command_name(m_clientQuery, iter));
		Q_EMIT m_client.showParametrizedAction(actionText, QVariant::fromValue(items));
	}

	void queryModelsChanged() {
		m_results->setModel(
				hud_client_query_get_results_model(m_clientQuery));
		m_appstack->setModel(
				hud_client_query_get_appstack_model(m_clientQuery));

		Q_EMIT m_client.modelsChanged();
	}

	HudClient &m_client;

	HudClientQuery *m_clientQuery;

	QScopedPointer<DeeListModel> m_results;

	QScopedPointer<DeeListModel> m_appstack;

	QScopedPointer<QAbstractListModel> m_toolBarModel;

	int m_currentActionIndex;

	HudClientParam *m_currentActionParam;
};

HudClient::HudClient() :
		p(new Priv(*this)) {
	p->m_clientQuery = hud_client_query_new("");
	p->m_results.reset(new DeeListModel());
	p->m_appstack.reset(new DeeListModel());
	p->m_toolBarModel.reset(new HudToolBarModel(p->m_clientQuery));
	p->m_currentActionParam = NULL;
	p->m_results->setModel(
			hud_client_query_get_results_model(p->m_clientQuery));
	p->m_appstack->setModel(
			hud_client_query_get_appstack_model(p->m_clientQuery));

	g_signal_connect(G_OBJECT(p->m_clientQuery), "voice-query-loading",
			G_CALLBACK(Priv::loadingCB), this);
	g_signal_connect(G_OBJECT(p->m_clientQuery), "voice-query-listening",
			G_CALLBACK(Priv::listeningCB), this);
	g_signal_connect(G_OBJECT(p->m_clientQuery), "voice-query-heard-something",
			G_CALLBACK(Priv::heardSomethingCB), this);
	g_signal_connect(G_OBJECT(p->m_clientQuery), "voice-query-finished",
			G_CALLBACK(Priv::finishedCB), this);
	g_signal_connect(G_OBJECT(p->m_clientQuery), "voice-query-failed",
			G_CALLBACK(Priv::failedCB), this);
	g_signal_connect(G_OBJECT(p->m_clientQuery),
			HUD_CLIENT_QUERY_SIGNAL_MODELS_CHANGED,
			G_CALLBACK(Priv::modelsChangedCB), p.data());
	g_signal_connect(G_OBJECT(p->m_clientQuery),
			HUD_CLIENT_QUERY_SIGNAL_TOOLBAR_UPDATED,
			G_CALLBACK(Priv::toolBarUpdatedCB), p->m_toolBarModel.data());
}

HudClient::~HudClient() {
	g_signal_handlers_disconnect_by_func(G_OBJECT(p->m_clientQuery),
			TO_GPOINTER(Priv::loadingCB), this);
	g_signal_handlers_disconnect_by_func(G_OBJECT(p->m_clientQuery),
			TO_GPOINTER(Priv::listeningCB), this);
	g_signal_handlers_disconnect_by_func(G_OBJECT(p->m_clientQuery),
			TO_GPOINTER(Priv::heardSomethingCB), this);
	g_signal_handlers_disconnect_by_func(G_OBJECT(p->m_clientQuery),
			TO_GPOINTER(Priv::finishedCB), this);
	g_signal_handlers_disconnect_by_func(G_OBJECT(p->m_clientQuery),
			TO_GPOINTER(Priv::toolBarUpdatedCB), p->m_toolBarModel.data());

	g_object_unref(p->m_clientQuery);
}

void HudClient::setQuery(const QString &new_query) {
	hud_client_query_set_query(p->m_clientQuery,
			new_query.toUtf8().constData());
}

void HudClient::setAppstackApp(const QString &applicationId) {
	hud_client_query_set_appstack_app(p->m_clientQuery,
			applicationId.toUtf8().constData());
}

void HudClient::startVoiceQuery() {
	hud_client_query_voice_query(p->m_clientQuery);
}

void HudClient::executeParametrizedAction(const QVariant &values) {
	updateParametrizedAction(values);
	hud_client_param_send_commit(p->m_currentActionParam);
	g_object_unref(p->m_currentActionParam);
	p->m_currentActionParam = NULL;
	Q_EMIT commandExecuted();
}

void HudClient::updateParametrizedAction(const QVariant &values) {
	if (p->m_currentActionParam != NULL) {
		const QVariantMap map = values.value<QVariantMap>();
		GActionGroup *ag = hud_client_param_get_actions(
				p->m_currentActionParam);

		auto it = map.begin();
		for (; it != map.end(); ++it) {
			const QString action = it.key();
			const QVariant value = it.value();
			const GVariantType *actionType =
					g_action_group_get_action_parameter_type(ag,
							action.toUtf8().constData());
			if (g_variant_type_equal(actionType, G_VARIANT_TYPE_DOUBLE)
					&& value.canConvert(QVariant::Double)) {
				g_action_group_activate_action(ag, action.toUtf8().constData(),
						g_variant_new_double(value.toDouble()));
			} else {
				qWarning()
						<< "Unsuported action type in HudClient::executeParametrizedAction";
			}
		}
	} else {
		qWarning()
				<< "Got to HudClient::updateParametrizedAction with no m_currentActionParam";
	}
}

void HudClient::cancelParametrizedAction() {
	if (p->m_currentActionParam != NULL) {
		hud_client_param_send_cancel(p->m_currentActionParam);
		g_object_unref(p->m_currentActionParam);
		p->m_currentActionParam = NULL;
	}
}

void HudClient::executeToolBarAction(HudClientQueryToolbarItems action) {
	hud_client_query_execute_toolbar_item(p->m_clientQuery, action, /* timestamp */
	0);
	Q_EMIT commandExecuted();
}

QAbstractListModel * HudClient::results() const {
	return p->m_results.data();
}

QAbstractListModel * HudClient::appstack() const {
	return p->m_appstack.data();
}

QAbstractListModel * HudClient::toolBarModel() const {
	return p->m_toolBarModel.data();
}

void HudClient::executeCommand(int index) {
	p->m_currentActionIndex = index;
	DeeModel *model = hud_client_query_get_results_model(p->m_clientQuery);
	DeeModelIter *iter = dee_model_get_iter_at_row(model, index);

	GVariant *command_key = hud_client_query_results_get_command_id(p->m_clientQuery, iter);
	if (hud_client_query_results_is_parameterized(p->m_clientQuery, iter)) {
		p->m_currentActionParam = hud_client_query_execute_param_command(
				p->m_clientQuery, command_key, /* timestamp */0);
		if (p->m_currentActionParam != NULL) {
			GMenuModel *menuModel = hud_client_param_get_model(
					p->m_currentActionParam);
			if (menuModel == NULL) {
				g_signal_connect(p->m_currentActionParam,
						HUD_CLIENT_PARAM_SIGNAL_MODEL_READY,
						G_CALLBACK(Priv::modelReadyCB), p.data());
			} else {
				p->modelReady(false);
			}
		} else {
			qWarning()
					<< "HudClient::executeCommand::Could not get the HudClientParam for parametrized action with index"
					<< index;
		}
	} else {
		hud_client_query_execute_command(p->m_clientQuery, command_key, /* timestamp */
		0);
		Q_EMIT commandExecuted();
	}
	g_variant_unref(command_key);
}

