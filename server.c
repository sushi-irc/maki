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
	struct maki_server* serv = NULL;
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

		serv = g_new(struct maki_server, 1);
		serv->server = g_strdup(server);
		serv->initial_nick = nick;
		serv->name = name;
		serv->autoconnect = autoconnect;
		serv->connected = FALSE;
		serv->reconnect = 0;
		serv->retries = m->config->reconnect.retries;
		serv->connection = sashimi_new(address, port, m->message_queue, serv);
		serv->channels = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, maki_channel_free);
		serv->users = maki_cache_new(maki_user_new, maki_user_free, serv);
		serv->logs = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, maki_log_free);

		serv->user = maki_cache_insert(serv->users, serv->initial_nick);

		serv->nickserv.ghost = nickserv_ghost;
		serv->nickserv.password = nickserv;
		serv->commands = commands;
		serv->ignores = ignores;
		serv->support.chanmodes = NULL;
		serv->support.chantypes = g_strdup("#&");
		serv->support.prefix.modes = g_strdup("ov");
		serv->support.prefix.prefixes = g_strdup("@+");

		sashimi_timeout(serv->connection, 60);

		if (serv->autoconnect)
		{
			if (maki_server_connect(serv) != 0)
			{
				maki_reconnect_callback(serv);
			}
		}

		g_free(address);

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
				chan->key = key;

				g_hash_table_replace(serv->channels, chan->name, chan);
			}
		}

		g_strfreev(groups);
	}

	g_key_file_free(key_file);
	g_free(path);

	return serv;
}

/**
 * This function gets called when a connection is removed from the servers hash table.
 */
void maki_server_free (gpointer data)
{
	struct maki_server* serv = data;

	if (serv->reconnect != 0)
	{
		g_source_remove(serv->reconnect);
	}

	maki_server_disconnect(serv, NULL);

	maki_cache_remove(serv->users, serv->user->nick);

	g_free(serv->support.prefix.prefixes);
	g_free(serv->support.prefix.modes);
	g_free(serv->support.chantypes);
	g_free(serv->support.chanmodes);
	g_strfreev(serv->ignores);
	g_strfreev(serv->commands);
	g_free(serv->nickserv.password);
	g_hash_table_destroy(serv->logs);
	g_hash_table_destroy(serv->channels);
	maki_cache_free(serv->users);
	sashimi_free(serv->connection);
	g_free(serv->name);
	g_free(serv->initial_nick);
	g_free(serv->server);
	g_free(serv);
}

/**
 * This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal.
 */
gint maki_server_connect (struct maki_server* serv)
{
	gint ret;
	struct maki* m = maki();

	sashimi_reconnect(serv->connection, maki_reconnect_callback, serv);

	if ((ret = sashimi_connect(serv->connection)) == 0)
	{
		GTimeVal time;
		struct maki_user* user;

		if (serv->reconnect != 0)
		{
			g_source_remove(serv->reconnect);
			serv->reconnect = 0;
		}

		serv->retries = m->config->reconnect.retries;

		user = maki_cache_insert(serv->users, serv->initial_nick);
		maki_user_copy(serv->user, user);
		maki_cache_remove(serv->users, serv->user->nick);
		serv->user = user;

		maki_out_nick(serv, serv->initial_nick);

		maki_send_printf(serv, "USER %s 0 * :%s", serv->initial_nick, serv->name);

		g_get_current_time(&time);
		maki_dbus_emit_connect(time.tv_sec, serv->server);
	}

	return ret;
}

/**
 * This function is a wrapper around sashimi_disconnect().
 */
gint maki_server_disconnect (struct maki_server* serv, const gchar* message)
{
	gint ret;
	GList* list;
	GList* tmp;

	sashimi_reconnect(serv->connection, NULL, NULL);

	if (message != NULL)
	{
		maki_out_quit(serv, message);
	}

	serv->connected = FALSE;
	ret = sashimi_disconnect(serv->connection);

	/* Remove all users from all channels, because otherwise phantom users may be left behind. */
	list = g_hash_table_get_values(serv->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;

		g_hash_table_remove_all(chan->users);
	}

	g_list_free(list);

	return ret;
}