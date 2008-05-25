/*
 * Copyright (c) 2008 Michael Kuhn
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

/**
 * This function handles unexpected reconnects.
 */
gboolean maki_reconnect (gpointer data)
{
	GTimeVal time;
	struct maki_connection* m_conn = data;
	struct maki* m = maki();

	maki_connection_disconnect(m_conn);

	if (m_conn->retries > 0)
	{
		m_conn->retries--;
	}
	else if (m_conn->retries == 0)
	{
		/* Finally give up. */
		return FALSE;
	}

	g_get_current_time(&time);
	maki_dbus_emit_reconnect(time.tv_sec, m_conn->server);

	if (maki_connection_connect(m_conn) == 0)
	{
		return FALSE;
	}

	return TRUE;
}

/**
 * This function is called by sashimi if the connection drops.
 * It schedules maki_reconnect() to be called regularly.
 */
void maki_reconnect_callback (gpointer data)
{
	struct maki_connection* m_conn = data;
	struct maki* m = maki();

	if (m_conn->reconnect != 0)
	{
		return;
	}

	m_conn->reconnect = g_timeout_add_seconds(m->config->reconnect.timeout, maki_reconnect, m_conn);
}

void maki_servers (void)
{
	const gchar* file;
	GDir* servers;
	struct maki* m = maki();

	servers = g_dir_open(m->directories.servers, 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		struct maki_connection* m_conn;

		if ((m_conn = maki_connection_new(file)) != NULL)
		{
			g_hash_table_replace(m->connections, m_conn->server, m_conn);
		}

	}

	g_dir_close(servers);
}
