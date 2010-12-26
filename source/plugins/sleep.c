/*
 * Copyright (c) 2009 Michael Kuhn
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

#ifdef HAVE_LIBNM_GLIB
#include <nm-client.h>
#endif

#include "instance.h"

gboolean init (void);
void deinit (void);

static GDBusProxy* dbus_proxy;
static gboolean sleeping;

#ifdef HAVE_LIBNM_GLIB
static NMClient* nm_client;
#endif

static void
servers_connect (void)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		/* FIXME thread safety */
		if (maki_server_autoconnect(serv))
		{
			maki_server_connect(serv);
		}
	}
}

static void
servers_disconnect (const gchar* message)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		/* FIXME thread safety */
		maki_server_disconnect(serv, message);
	}
}

static void
on_signal (GDBusProxy* proxy, gchar* sender, gchar* signal_name, GVariant* parameters, gpointer data)
{
	if (g_strcmp0(signal_name, "Sleeping") == 0)
	{
		sleeping = TRUE;

		servers_disconnect("Computer is going to sleep.");
	}
	else if (g_strcmp0(signal_name, "Resuming") == 0)
	{
		sleeping = FALSE;

#ifndef HAVE_LIBNM_GLIB
		servers_connect();
#endif
	}
}

#ifdef HAVE_LIBNM_GLIB
static void
on_notify_state (NMClient* client, GParamSpec* pspec, gpointer data)
{
	NMState state;

	state = nm_client_get_state(client);

	/* Connected */
	if (state == NM_STATE_UNKNOWN || state == NM_STATE_CONNECTED)
	{
		if (!sleeping)
		{
			servers_connect();
		}
	}
	/* Asleep or Disconnected */
	else if (state == NM_STATE_ASLEEP || state == NM_STATE_CONNECTING || state == NM_STATE_DISCONNECTED)
	{
		if (!sleeping)
		{
			servers_disconnect("Network configuration changed.");
		}
	}
}
#endif

G_MODULE_EXPORT
gboolean
init (void)
{
	sleeping = FALSE;

	dbus_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
		"org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower",
		NULL, NULL);

	if (dbus_proxy != NULL)
	{
		g_signal_connect(dbus_proxy, "g-signal", G_CALLBACK(on_signal), NULL);
	}

#ifdef HAVE_LIBNM_GLIB
	/* NMState state; */

	nm_client = nm_client_new();

	/* FIXME do something with state */
	/* state = nm_client_get_state(nm_client); */

	if (nm_client != NULL)
	{
		g_signal_connect(nm_client, "notify::state", G_CALLBACK(on_notify_state), NULL);
	}
#endif

	return TRUE;
}

G_MODULE_EXPORT
void
deinit (void)
{
#ifdef HAVE_LIBNM_GLIB
	if (nm_client != NULL)
	{
		g_object_unref(nm_client);
	}
#endif

	if (dbus_proxy != NULL)
	{
		g_object_unref(dbus_proxy);
	}
}
