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

#include <string.h>

#include "maki.h"
#include "maki_cache.h"
#include "maki_misc.h"

/**
 * This function handles unexpected reconnects.
 */
gboolean maki_reconnect (gpointer data)
{
	GTimeVal time;
	struct maki_connection* m_conn = data;

	/*
	 * m_conn->reconnect is set to FALSE by the quit method.
	 */
	if (!m_conn->reconnect)
	{
		return FALSE;
	}

	maki_disconnect(m_conn);

	if (m_conn->retries > 0)
	{
		m_conn->retries--;
	}
	else if (m_conn->retries == 0)
	{
		/*
		 * Finally give up and free the connection.
		 */
		g_hash_table_remove(m_conn->maki->connections, m_conn->server);

		return FALSE;
	}

	g_get_current_time(&time);
	maki_dbus_emit_reconnect(m_conn->maki->bus, time.tv_sec, m_conn->server);

	if (maki_connect(m_conn) == 0)
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

	g_timeout_add_seconds(m_conn->maki->config.reconnect.timeout, maki_reconnect, m_conn);
}

struct maki_connection* maki_server_new (struct maki* maki, const gchar* server)
{
	gchar* path;
	GKeyFile* key_file;
	struct maki_connection* m_conn = NULL;

	path = g_build_filename(maki->directories.servers, server, NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		gchar** group;
		gchar** groups;
		gboolean autoconnect;
		gchar* address;
		gchar* nick;
		gchar* name;
		gchar* nickserv;
		gint port;

		autoconnect = g_key_file_get_boolean(key_file, "server", "autoconnect", NULL);
		address = g_key_file_get_string(key_file, "server", "address", NULL);
		port = g_key_file_get_integer(key_file, "server", "port", NULL);
		nick = g_key_file_get_string(key_file, "server", "nick", NULL);
		name = g_key_file_get_string(key_file, "server", "name", NULL);
		nickserv = g_key_file_get_string(key_file, "server", "nickserv", NULL);

		if (port == 0)
		{
			port = 6667;
		}

		if (nick == NULL)
		{
			nick = g_strdup(g_get_user_name());
		}

		if (name == NULL)
		{
			name = g_strdup(g_get_real_name());
		}

		m_conn = maki_connection_new(maki, server, address, port, nick, name);
		m_conn->autoconnect = autoconnect;
		m_conn->nickserv.password = g_strdup(nickserv);

		sashimi_reconnect(m_conn->connection, maki_reconnect_callback, m_conn);
		sashimi_timeout(m_conn->connection, 60);

		if (m_conn->autoconnect)
		{
			if (maki_connect(m_conn) != 0)
			{
				maki_reconnect_callback(m_conn);
			}
		}

		g_hash_table_replace(maki->connections, m_conn->server, m_conn);

		g_free(address);
		g_free(nick);
		g_free(name);
		g_free(nickserv);

		groups = g_key_file_get_groups(key_file, NULL);

		for (group = groups; *group != NULL; ++group)
		{
			if (strncmp(*group, "server", 6) != 0)
			{
				gboolean autojoin;
				gchar* key;
				struct maki_channel* m_chan;

				if (m_conn == NULL)
				{
					continue;
				}

				autojoin = g_key_file_get_boolean(key_file, *group, "autojoin", NULL);
				key = g_key_file_get_string(key_file, *group, "key", NULL);

				m_chan = maki_channel_new(*group);
				m_chan->autojoin = autojoin;
				m_chan->key = g_strdup(key);

				g_hash_table_replace(m_conn->channels, m_chan->name, m_chan);

				g_free(key);
			}
		}

		g_strfreev(groups);
	}

	g_key_file_free(key_file);
	g_free(path);

	return m_conn;
}

void maki_servers (struct maki* maki)
{
	const gchar* file;
	GDir* servers;

	g_mkdir_with_parents(maki->directories.servers, 0755);

	servers = g_dir_open(maki->directories.servers, 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		maki_server_new(maki, file);
	}

	g_dir_close(servers);
}
