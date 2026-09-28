/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#ifndef COMMON_GDBUSHELPER_H_
#define COMMON_GDBUSHELPER_H_

#include <gio/gio.h>

static inline GDBusConnection * newSessionBusConnection(GCancellable* cancellable)
{
	GError *error = NULL;

	gchar *address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION,
			NULL, &error);
	if (!address) {
		g_assert(error != NULL);
		if (error->domain != G_IO_ERROR
				|| error->code != G_IO_ERROR_CANCELLED) {
		}
		g_error_free(error);
		return NULL;
	}

	error = NULL;
	GDBusConnection* connection =
			g_dbus_connection_new_for_address_sync(address,
					(GDBusConnectionFlags) (G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT
							| G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
							NULL, cancellable, &error);
	g_free(address);

	if (connection == NULL) {
		g_assert(error != NULL);
		if (error->domain != G_IO_ERROR
				|| error->code != G_IO_ERROR_CANCELLED) {
		}
		g_error_free(error);
		return NULL;
	}

	g_dbus_connection_set_exit_on_close(connection, FALSE);

	return connection;
}

#endif // COMMON_GDBUSHELPER_H_
