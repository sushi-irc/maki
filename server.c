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

struct maki_server* maki_server_new (const gchar* server)
{
	gchar* path;
	GKeyFile* key_file;
	struct maki_server* conn = NULL;
	struct maki* m = maki();

	path = g_build_filename(m->directories.servers, server, NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		gchar** group;
		gchar** groups;
		gboolean autoconnect;
		gboolean nickserv_ghost;
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
		nickserv_ghost = g_key_file_get_boolean(key_file, "server", "nickserv_ghost", NULL);
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

		conn = g_new(struct maki_server, 1);
		conn->server = g_strdup(server);
		conn->initial_nick = g_strdup(nick);
		conn->name = g_strdup(name);
		conn->autoconnect = autoconnect;
		conn->connected = FALSE;
		conn->reconnect = 0;
		conn->retries = m->config->reconnect.retries;
		conn->connection = sashimi_new(address, port, m->message_queue, conn);
		conn->channels = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, maki_channel_free);
		conn->users = maki_cache_new(maki_user_new, maki_user_free, conn);
		conn->logs = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, maki_log_free);

		conn->user = maki_cache_insert(conn->users, nick);

		conn->nickserv.ghost = nickserv_ghost;
		conn->nickserv.password = g_strdup(nickserv);
		conn->commands = g_strdupv(commands);
		conn->ignores = g_strdupv(ignores);
		conn->support.chanmodes = NULL;
		conn->support.chantypes = g_strdup("#&");
		conn->support.prefix.modes = g_strdup("ov");
		conn->support.prefix.prefixes = g_strdup("@+");

		sashimi_timeout(conn->connection, 60);

		if (conn->autoconnect)
		{
			if (maki_server_connect(conn) != 0)
			{
				maki_reconnect_callback(conn);
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
				struct maki_channel* chan;

				autojoin = g_key_file_get_boolean(key_file, *group, "autojoin", NULL);
				key = g_key_file_get_string(key_file, *group, "key", NULL);

				chan = maki_channel_new(*group);
				chan->autojoin = autojoin;
				chan->key = g_strdup(key);

				g_hash_table_replace(conn->channels, chan->name, chan);

				g_free(key);
			}
		}

		g_strfreev(groups);
	}

	g_key_file_free(key_file);
	g_free(path);

	return conn;
}

/**
 * This function gets called when a connection is removed from the servers hash table.
 */
void maki_server_free (gpointer data)
{
	struct maki_server* conn = data;

	if (conn->reconnect != 0)
	{
		g_source_remove(conn->reconnect);
	}

	maki_server_disconnect(conn, NULL);

	maki_cache_remove(conn->users, conn->user->nick);

	g_free(conn->support.prefix.prefixes);
	g_free(conn->support.prefix.modes);
	g_free(conn->support.chantypes);
	g_free(conn->support.chanmodes);
	g_strfreev(conn->ignores);
	g_strfreev(conn->commands);
	g_free(conn->nickserv.password);
	g_hash_table_destroy(conn->logs);
	g_hash_table_destroy(conn->channels);
	maki_cache_free(conn->users);
	sashimi_free(conn->connection);
	g_free(conn->name);
	g_free(conn->initial_nick);
	g_free(conn->server);
	g_free(conn);
}

/**
 * This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal.
 */
gint maki_server_connect (struct maki_server* conn)
{
	gint ret;
	struct maki* m = maki();

	sashimi_reconnect(conn->connection, maki_reconnect_callback, conn);

	if ((ret = sashimi_connect(conn->connection)) == 0)
	{
		GTimeVal time;
		struct maki_user* user;

		if (conn->reconnect != 0)
		{
			g_source_remove(conn->reconnect);
			conn->reconnect = 0;
		}

		conn->retries = m->config->reconnect.retries;

		user = maki_cache_insert(conn->users, conn->initial_nick);
		maki_user_copy(conn->user, user);
		maki_cache_remove(conn->users, conn->user->nick);
		conn->user = user;

		maki_out_nick(conn, conn->initial_nick);

		maki_send_printf(conn, "USER %s 0 * :%s", conn->initial_nick, conn->name);

		g_get_current_time(&time);
		maki_dbus_emit_connect(time.tv_sec, conn->server);
	}

	return ret;
}

/**
 * This function is a wrapper around sashimi_disconnect().
 */
gint maki_server_disconnect (struct maki_server* conn, const gchar* message)
{
	gint ret;
	GList* list;
	GList* tmp;

	sashimi_reconnect(conn->connection, NULL, NULL);

	if (message != NULL)
	{
		maki_out_quit(conn, message);
	}

	conn->connected = FALSE;
	ret = sashimi_disconnect(conn->connection);

	/* Remove all users from all channels, because otherwise phantom users may be left behind. */
	list = g_hash_table_get_values(conn->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;

		g_hash_table_remove_all(chan->users);
	}

	g_list_free(list);

	return ret;
}
