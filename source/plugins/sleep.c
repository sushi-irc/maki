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

#include "maki.h"

#include "instance.h"

#include <glib-object.h>
#include <gmodule.h>

#include <nm-client.h>

gboolean init (void);
void deinit (void);

static NMClient* nm_client;
static NMState nm_state;

static void notify_state_cb (NMClient* client, GParamSpec* pspec, gpointer data)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();
	NMState state;

	state = nm_client_get_state(client);

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		/* FIXME thread safety */
		/* Connected */
		if (state == NM_STATE_UNKNOWN || state == NM_STATE_CONNECTED)
		{
			if (maki_server_autoconnect(serv))
			{
				maki_server_connect(serv);
			}
		}
		/* Asleep or Disconnected */
		else if (state == NM_STATE_ASLEEP || state == NM_STATE_CONNECTING || state == NM_STATE_DISCONNECTED)
		{
			maki_server_disconnect(serv, "NetworkManager disconnected.");
		}
	}

	nm_state = state;
}

G_MODULE_EXPORT
gboolean init (void)
{
	nm_client = nm_client_new();
	nm_state = nm_client_get_state(nm_client);

	/* FIXME do something with state */

	g_signal_connect(nm_client, "notify::state", G_CALLBACK(notify_state_cb), NULL);

	return TRUE;
}

G_MODULE_EXPORT
void deinit (void)
{
	g_object_unref(nm_client);
}
