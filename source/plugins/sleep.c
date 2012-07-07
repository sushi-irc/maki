/*
 * Copyright (c) 2009-2012 Michael Kuhn
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
#include "plugin.h"

gboolean sleeping (void);

static GDBusProxy* upower_proxy;
static GDBusProxy* nm_proxy;
static gboolean is_sleeping;

static
void
upower_on_signal (GDBusProxy* proxy, gchar* sender, gchar* signal_name, GVariant* parameters, gpointer data)
{
	makiInstance* inst = maki_instance_get_default();

	if (g_strcmp0(signal_name, "Sleeping") == 0)
	{
		is_sleeping = TRUE;

		maki_network_disconnect(maki_instance_network(inst), "Computer is going to sleep.");
	}
	else if (g_strcmp0(signal_name, "Resuming") == 0)
	{
		gchar* owner;
		gboolean handle_connect;

		is_sleeping = FALSE;

		owner = g_dbus_proxy_get_name_owner(nm_proxy);
		handle_connect = (owner == NULL);
		g_free(owner);

		if (handle_connect)
		{
			maki_network_connect(maki_instance_network(inst), FALSE);
		}
	}
}

/* FIXME */
static
void
nm_on_signal (GDBusProxy* proxy, gchar* sender, gchar* signal_name, GVariant* parameters, gpointer data)
{
	makiInstance* inst = maki_instance_get_default();

	if (g_strcmp0(signal_name, "StateChanged") == 0)
	{
		guint32 state;

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
				maki_network_connect(maki_instance_network(inst), FALSE);
				break;
			default:
				;
		}
	}
}

G_MODULE_EXPORT
gboolean
init (void)
{
	is_sleeping = FALSE;

	upower_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
		"org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower",
		NULL, NULL);

	if (upower_proxy != NULL)
	{
		g_signal_connect(upower_proxy, "g-signal", G_CALLBACK(upower_on_signal), NULL);
	}

	nm_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
		"org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager", "org.freedesktop.NetworkManager",
		NULL, NULL);

	if (nm_proxy != NULL)
	{
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

	if (upower_proxy != NULL)
	{
		g_object_unref(upower_proxy);
	}
}

G_MODULE_EXPORT
gboolean
sleeping (void)
{
	return is_sleeping;
}
