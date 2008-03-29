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

void maki_server_new (struct maki* maki, const gchar* server)
{
	gchar* path;
	gchar** group;
	gchar** groups;
	GKeyFile* key_file;

	path = g_build_filename(maki->directories.servers, server, NULL);

	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		groups = g_key_file_get_groups(key_file, NULL);

		for (group = groups; *group != NULL; ++group)
		{
			if (g_ascii_strncasecmp(*group, "server", 6) == 0)
			{
				gchar* address;
				gchar* nick;
				gchar* name;
				gchar* nickserv;
				gint port;
				struct maki_connection* m_conn;

				address = g_key_file_get_string(key_file, *group, "address", NULL);
				port = g_key_file_get_integer(key_file, *group, "port", NULL);
				nick = g_key_file_get_string(key_file, *group, "nick", NULL);
				name = g_key_file_get_string(key_file, *group, "name", NULL);
				nickserv = g_key_file_get_string(key_file, *group, "nickserv", NULL);

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

				m_conn = g_new(struct maki_connection, 1);

				m_conn->maki = maki;
				m_conn->server = g_strdup(server);
				m_conn->initial_nick = g_strdup(nick);
				m_conn->nick = g_strdup(nick);
				m_conn->name = g_strdup(name);
				m_conn->connected = FALSE;
				m_conn->reconnect = TRUE;
				m_conn->retries = maki->config.reconnect.retries;
				m_conn->connection = sashimi_new(address, port, maki->message_queue, m_conn);
				m_conn->channels = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_channel_destroy);

				m_conn->nickserv.password = g_strdup(nickserv);
				m_conn->support.chanmodes = NULL;
				m_conn->support.prefix.modes = g_strdup("ov");
				m_conn->support.prefix.prefixes = g_strdup("@+");

				sashimi_reconnect(m_conn->connection, maki_reconnect_callback, m_conn);

				if (maki_connect(m_conn) != 0)
				{
					maki_reconnect_callback(m_conn);
				}

				g_hash_table_replace(maki->connections, m_conn->server, m_conn);

				g_free(address);
				g_free(nick);
				g_free(name);
				g_free(nickserv);
			}
			else
			{
				gchar* key;
				struct maki_connection* m_conn;

				key = g_key_file_get_string(key_file, *group, "key", NULL);

				if ((m_conn = g_hash_table_lookup(maki->connections, server)) != NULL)
				{
					struct maki_channel* m_chan;

					m_chan = g_new(struct maki_channel, 1);

					m_chan->name = g_strdup(*group);
					m_chan->joined = FALSE;
					m_chan->key = g_strdup(key);
					m_chan->users = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_user_destroy);

					g_hash_table_replace(m_conn->channels, m_chan->name, m_chan);
				}

				g_free(key);
			}
		}

		g_strfreev(groups);
	}

	g_key_file_free(key_file);
	g_free(path);
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
