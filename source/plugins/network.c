/*
 * Copyright (c) 2009-2011 Michael Kuhn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

#include <instance.h>

#include "network.h"

gboolean init (void);
void deinit (void);

static GDBusProxy* nm_proxy;
static gboolean is_connected;

static
void
servers_connect (void)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		if (maki_server_autoconnect(serv))
		{
			maki_server_connect(serv);
		}
	}
}

static
void
servers_disconnect (const gchar* message)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, message);
	}
}

static
void
nm_on_signal (GDBusProxy* proxy, gchar* sender, gchar* signal_name, GVariant* parameters, gpointer data)
{
	if (g_strcmp0(signal_name, "StateChanged") == 0)
	{
		makiInstance* inst = maki_instance_get_default();
		guint32 state;
		gboolean is_sleeping = FALSE;

		gboolean (*sleeping) (void);

		if (maki_instance_plugin_method(inst, "sleep", "sleeping", (gpointer*)&sleeping))
		{
			is_sleeping = sleeping();
		}

		if (is_sleeping)
		{
			return;
		}

		g_variant_get(parameters, "(u)", &state);

		switch (state)
		{
			/* case NM_OLD_STATE_UNKNOWN: */
			case NM_OLD_STATE_CONNECTED:
			case NM_STATE_UNKNOWN:
			case NM_STATE_CONNECTED_LOCAL:
			case NM_STATE_CONNECTED_SITE:
			case NM_STATE_CONNECTED_GLOBAL:
				if (!is_connected)
				{
					is_connected = TRUE;
					servers_connect();
				}

				break;

			case NM_OLD_STATE_ASLEEP:
			case NM_OLD_STATE_CONNECTING:
			case NM_OLD_STATE_DISCONNECTED:
			case NM_STATE_ASLEEP:
			case NM_STATE_DISCONNECTED:
			case NM_STATE_DISCONNECTING:
			case NM_STATE_CONNECTING:
				if (is_connected)
				{
					is_connected = FALSE;
					servers_disconnect("Network configuration changed.");
				}

				break;

			default:
				g_warn_if_reached();
		}
	}
}

G_MODULE_EXPORT
gboolean
init (void)
{
	GVariant* variant;

	is_connected = TRUE;

	nm_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,
		"org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager", "org.freedesktop.NetworkManager",
		NULL, NULL);

	if (nm_proxy != NULL)
	{
		guint32 state;

		variant = g_dbus_proxy_get_cached_property(nm_proxy, "State");
		g_variant_get(variant, "u", &state);
		g_variant_unref(variant);

		switch (state)
		{
			/* case NM_OLD_STATE_UNKNOWN: */
			case NM_OLD_STATE_CONNECTED:
			case NM_STATE_UNKNOWN:
			case NM_STATE_CONNECTED_LOCAL:
			case NM_STATE_CONNECTED_SITE:
			case NM_STATE_CONNECTED_GLOBAL:
				is_connected = TRUE;
				break;
			default:
				is_connected = FALSE;
		}

		g_signal_connect(nm_proxy, "g-signal", G_CALLBACK(nm_on_signal), NULL);
	}

	return TRUE;
}

G_MODULE_EXPORT
void
deinit (void)
{
	if (nm_proxy != NULL)
	{
		g_object_unref(nm_proxy);
	}
}
