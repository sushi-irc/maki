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

makiServer* maki_server_new (makiInstance* inst, const gchar* server)
{
	gchar* path;
	GKeyFile* key_file;
	makiServer* serv = NULL;

	path = g_build_filename(maki_instance_directory(inst, "servers"), server, NULL);
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

		serv = g_new(makiServer, 1);
		serv->instance = inst;
		serv->server = g_strdup(server);
		serv->initial_nick = nick;
		serv->name = name;
		serv->autoconnect = autoconnect;
		serv->connected = FALSE;
		serv->reconnect.source = 0;
		serv->reconnect.retries = maki_config_get_int(maki_instance_config(serv->instance), "reconnect" ,"retries");
		serv->connection = sashimi_new(address, port, maki_instance_queue(serv->instance), serv);
		serv->channels = g_hash_table_new_full(maki_str_hash, maki_str_equal, g_free, maki_channel_free);
		serv->users = maki_cache_new(maki_user_new, maki_user_free, serv);
		serv->logs = g_hash_table_new_full(maki_str_hash, maki_str_equal, g_free, maki_log_free);

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
			if (!maki_server_connect(serv))
			{
				maki_server_reconnect_callback(serv);
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
				makiChannel* chan;

				autojoin = g_key_file_get_boolean(key_file, *group, "autojoin", NULL);
				key = g_key_file_get_string(key_file, *group, "key", NULL);

				chan = maki_channel_new();
				maki_channel_set_autojoin(chan, autojoin);
				maki_channel_set_key(chan, key);

				g_hash_table_insert(serv->channels, g_strdup(*group), chan);

				g_free(key);
			}
		}

		g_strfreev(groups);
	}

	g_key_file_free(key_file);
	g_free(path);

	return serv;
}

makiChannel* maki_server_add_channel (makiServer* serv, const gchar* name, makiChannel* chan)
{
	g_hash_table_insert(serv->channels, g_strdup(name), chan);

	return chan;
}

makiChannel* maki_server_get_channel (makiServer* serv, const gchar* name)
{
	return g_hash_table_lookup(serv->channels, name);
}

void maki_server_remove_channel (makiServer* serv, const gchar* name)
{
	g_hash_table_remove(serv->channels, name);
}

guint maki_server_channels_count (makiServer* serv)
{
	return g_hash_table_size(serv->channels);
}

void maki_server_channels_iter (makiServer* serv, GHashTableIter* iter)
{
	g_hash_table_iter_init(iter, serv->channels);
}

gboolean maki_server_queue (makiServer* serv, const gchar* message, gboolean force)
{
	if (force)
	{
		return sashimi_queue(serv->connection, message);
	}
	else
	{
		return sashimi_send_or_queue(serv->connection, message);
	}
}

gboolean maki_server_send (makiServer* serv, const gchar* message)
{
	return sashimi_send(serv->connection, message);
}

gboolean maki_server_send_printf (makiServer* serv, const gchar* format, ...)
{
	gboolean ret;
	gchar* buffer;
	va_list args;

	va_start(args, format);
	buffer = g_strdup_vprintf(format, args);
	va_end(args);

	ret = sashimi_send(serv->connection, buffer);
	g_free(buffer);

	return ret;
}

/* This function gets called when a server is removed from the servers hash table. */
void maki_server_free (gpointer data)
{
	makiServer* serv = data;

	if (serv->reconnect.source != 0)
	{
		g_source_remove(serv->reconnect.source);
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
gboolean maki_server_connect (makiServer* serv)
{
	gboolean ret;

	sashimi_reconnect_callback(serv->connection, maki_server_reconnect_callback, serv);

	if ((ret = sashimi_connect(serv->connection)))
	{
		GTimeVal time;
		makiUser* user;

		if (serv->reconnect.source != 0)
		{
			g_source_remove(serv->reconnect.source);
			serv->reconnect.source = 0;
		}

		serv->reconnect.retries = maki_config_get_int(maki_instance_config(serv->instance), "reconnect", "retries");

		user = maki_cache_insert(serv->users, serv->initial_nick);
		maki_user_copy(serv->user, user);
		maki_cache_remove(serv->users, serv->user->nick);
		serv->user = user;

		maki_out_nick(serv, serv->initial_nick);

		maki_server_send_printf(serv, "USER %s 0 * :%s", serv->initial_nick, serv->name);

		g_get_current_time(&time);
		maki_dbus_emit_connect(time.tv_sec, serv->server);
	}

	return ret;
}

/**
 * This function is a wrapper around sashimi_disconnect().
 */
gboolean maki_server_disconnect (makiServer* serv, const gchar* message)
{
	gboolean ret;
	GHashTableIter iter;
	gpointer key, value;

	sashimi_reconnect_callback(serv->connection, NULL, NULL);

	if (message != NULL)
	{
		maki_out_quit(serv, message);
	}

	serv->connected = FALSE;
	ret = sashimi_disconnect(serv->connection);

	/* Remove all users from all channels, because otherwise phantom users may be left behind. */
	g_hash_table_iter_init(&iter, serv->channels);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiChannel* chan = value;

		maki_channel_remove_users(chan);
	}

	return ret;
}

/* This function handles unexpected reconnects. */
static gboolean maki_server_reconnect (gpointer data)
{
	GTimeVal time;
	makiServer* serv = data;

	maki_server_disconnect(serv, NULL);

	if (serv->reconnect.retries > 0)
	{
		serv->reconnect.retries--;
	}
	else if (serv->reconnect.retries == 0)
	{
		/* Finally give up. */
		return FALSE;
	}

	g_get_current_time(&time);
	maki_dbus_emit_reconnect(time.tv_sec, serv->server);

	if (maki_server_connect(serv))
	{
		return FALSE;
	}

	return TRUE;
}

/* This function is called by sashimi if the connection drops.
 * It schedules maki_server_reconnect() to be called regularly. */
void maki_server_reconnect_callback (gpointer data)
{
	makiServer* serv = data;

	if (serv->reconnect.source != 0)
	{
		return;
	}

	serv->reconnect.source = g_timeout_add_seconds(maki_config_get_int(maki_instance_config(serv->instance), "reconnect", "timeout"), maki_server_reconnect, serv);
}
