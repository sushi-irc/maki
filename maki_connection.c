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

struct maki_connection* maki_connection_new (const gchar* server)
{
	gchar* path;
	GKeyFile* key_file;
	struct maki_connection* m_conn = NULL;
	struct maki* m = maki();

	path = g_build_filename(m->directories.servers, server, NULL);
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
		gchar** commands;
		gchar** ignores;
		gint port;

		autoconnect = g_key_file_get_boolean(key_file, "server", "autoconnect", NULL);
		address = g_key_file_get_string(key_file, "server", "address", NULL);
		port = g_key_file_get_integer(key_file, "server", "port", NULL);
		nick = g_key_file_get_string(key_file, "server", "nick", NULL);
		name = g_key_file_get_string(key_file, "server", "name", NULL);
		nickserv = g_key_file_get_string(key_file, "server", "nickserv", NULL);
		commands = g_key_file_get_string_list(key_file, "server", "commands", NULL, NULL);
		ignores = g_key_file_get_string_list(key_file, "server", "ignores", NULL, NULL);

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
		m_conn->server = g_strdup(server);
		m_conn->initial_nick = g_strdup(nick);
		m_conn->name = g_strdup(name);
		m_conn->autoconnect = autoconnect;
		m_conn->connected = FALSE;
		m_conn->reconnect = FALSE;
		m_conn->retries = m->config->reconnect.retries;
		m_conn->connection = sashimi_new(address, port, m->message_queue, m_conn);
		m_conn->channels = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_channel_free);
		m_conn->users = maki_cache_new(maki_user_new, maki_user_free, m_conn);
		m_conn->logs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_log_free);

		m_conn->user = maki_cache_insert(m_conn->users, nick);

		m_conn->nickserv.password = g_strdup(nickserv);
		m_conn->commands = g_strdupv(commands);
		m_conn->ignores = g_strdupv(ignores);
		m_conn->support.chanmodes = NULL;
		m_conn->support.chantypes = g_strdup("#&");
		m_conn->support.prefix.modes = g_strdup("ov");
		m_conn->support.prefix.prefixes = g_strdup("@+");

		sashimi_reconnect(m_conn->connection, maki_reconnect_callback, m_conn);
		sashimi_timeout(m_conn->connection, 60);

		if (m_conn->autoconnect)
		{
			if (maki_connection_connect(m_conn) != 0)
			{
				maki_reconnect_callback(m_conn);
			}
		}

		g_free(address);
		g_free(nick);
		g_free(name);
		g_free(nickserv);
		g_strfreev(commands);
		g_strfreev(ignores);

		groups = g_key_file_get_groups(key_file, NULL);

		for (group = groups; *group != NULL; ++group)
		{
			if (strncmp(*group, "server", 6) != 0)
			{
				gboolean autojoin;
				gchar* key;
				struct maki_channel* m_chan;

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

/**
 * This function gets called when a connection is removed from the connections hash table.
 */
void maki_connection_free (gpointer data)
{
	struct maki_connection* m_conn = data;

	maki_cache_remove(m_conn->users, m_conn->user->nick);

	g_free(m_conn->support.prefix.prefixes);
	g_free(m_conn->support.prefix.modes);
	g_free(m_conn->support.chantypes);
	g_free(m_conn->support.chanmodes);
	g_strfreev(m_conn->ignores);
	g_strfreev(m_conn->commands);
	g_free(m_conn->nickserv.password);
	g_hash_table_destroy(m_conn->logs);
	g_hash_table_destroy(m_conn->channels);
	maki_cache_free(m_conn->users);
	maki_connection_disconnect(m_conn);
	sashimi_free(m_conn->connection);
	g_free(m_conn->name);
	g_free(m_conn->initial_nick);
	g_free(m_conn->server);
	g_free(m_conn);
}

/**
 * This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal.
 */
gint maki_connection_connect (struct maki_connection* m_conn)
{
	gint ret;
	struct maki* m = maki();

	if ((ret = sashimi_connect(m_conn->connection)) == 0)
	{
		gchar* buffer;
		GTimeVal time;

		m_conn->reconnect = TRUE;
		m_conn->retries = m->config->reconnect.retries;

		maki_cache_remove(m_conn->users, m_conn->user->nick);
		m_conn->user = maki_cache_insert(m_conn->users, m_conn->initial_nick);

		maki_out_nick(m_conn, m_conn->initial_nick);

		buffer = g_strdup_printf("USER %s 0 * :%s", m_conn->initial_nick, m_conn->name);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		g_get_current_time(&time);
		maki_dbus_emit_connect(time.tv_sec, m_conn->server);
	}

	return ret;
}

/**
 * This function is a wrapper around sashimi_disconnect().
 */
gint maki_connection_disconnect (struct maki_connection* m_conn)
{
	gint ret;

	m_conn->connected = FALSE;
	ret = sashimi_disconnect(m_conn->connection);

	return ret;
}

/**
 * This function gets called by the quit method after a delay.
 */
gboolean maki_disconnect_timeout (gpointer data)
{
	struct maki_connection* m_conn = data;

	maki_connection_disconnect(m_conn);

	return FALSE;
}
