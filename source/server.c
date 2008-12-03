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

#include <string.h>

static gpointer maki_server_thread (gpointer data)
{
	makiServer* serv = data;

	g_main_loop_run(serv->main_loop);

	return NULL;
}

static void maki_server_config_set_defaults (makiServer* serv)
{
	if (!maki_server_config_exists(serv, "server", "autoconnect"))
	{
		maki_server_config_set_boolean(serv, "server", "autoconnect", FALSE);
	}

	if (!maki_server_config_exists(serv, "server", "port"))
	{
		maki_server_config_set_integer(serv, "server", "port", 6667);
	}

	if (!maki_server_config_exists(serv, "server", "nick"))
	{
		maki_server_config_set_string(serv, "server", "nick", g_get_user_name());
	}

	if (!maki_server_config_exists(serv, "server", "name"))
	{
		maki_server_config_set_string(serv, "server", "name", g_get_real_name());
	}

	/*
		nickserv = g_key_file_get_string(key_file, "server", "nickserv", NULL);
		nickserv_ghost = g_key_file_get_boolean(key_file, "server", "nickserv_ghost", NULL);
		commands = g_key_file_get_string_list(key_file, "server", "commands", NULL, NULL);
		ignores = g_key_file_get_string_list(key_file, "server", "ignores", NULL, NULL);
	*/
}

makiServer* maki_server_new (makiInstance* inst, const gchar* server)
{
	gchar* path;
	GKeyFile* key_file;
	makiServer* serv = NULL;

	path = g_build_filename(maki_instance_directory(inst, "servers"), server, NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		gchar* nick;
		gchar** group;
		gchar** groups;

		serv = g_new(makiServer, 1);
		serv->instance = inst;
		serv->server = g_strdup(server);
		serv->key_file = key_file;
		serv->connected = FALSE;
		serv->logged_in = FALSE;
		serv->reconnect.source = 0;
		serv->reconnect.retries = maki_instance_config_get_integer(serv->instance, "reconnect" ,"retries");
		serv->main_context = g_main_context_new();
		serv->main_loop = g_main_loop_new(serv->main_context, FALSE);
		serv->connection = sashimi_new(serv->main_context);
		serv->channels = g_hash_table_new_full(maki_str_hash, maki_str_equal, g_free, maki_channel_free);
		serv->users = maki_cache_new(maki_user_new, maki_user_free, serv);
		serv->logs = g_hash_table_new_full(maki_str_hash, maki_str_equal, g_free, maki_log_free);

		maki_server_config_set_defaults(serv);

		nick = maki_server_config_get_string(serv, "server", "nick");
		serv->user = maki_cache_insert(serv->users, nick);
		g_free(nick);

		serv->support.chanmodes = NULL;
		serv->support.chantypes = g_strdup("#&");
		serv->support.prefix.modes = g_strdup("ov");
		serv->support.prefix.prefixes = g_strdup("@+");

		sashimi_timeout(serv->connection, 60);

		groups = g_key_file_get_groups(key_file, NULL);

		for (group = groups; *group != NULL; ++group)
		{
			if (strcmp(*group, "server") != 0)
			{
				g_hash_table_insert(serv->channels, g_strdup(*group), maki_channel_new(serv, *group));
			}
		}

		g_strfreev(groups);

		g_thread_create(maki_server_thread, serv, FALSE, NULL);

		if (maki_server_autoconnect(serv))
		{
			maki_server_connect(serv);
		}
	}
	else
	{
		g_key_file_free(key_file);
	}

	g_free(path);

	return serv;
}

gboolean maki_server_config_get_boolean (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_boolean(serv->key_file, group, key, NULL);
}

void maki_server_config_set_boolean (makiServer* serv, const gchar* group, const gchar* key, gboolean value)
{
	gchar* path;

	g_key_file_set_boolean(serv->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->server, NULL);
	maki_key_file_to_file(serv->key_file, path);
	g_free(path);
}

gint maki_server_config_get_integer (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_integer(serv->key_file, group, key, NULL);
}

void maki_server_config_set_integer (makiServer* serv, const gchar* group, const gchar* key, gint value)
{
	gchar* path;

	g_key_file_set_integer(serv->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->server, NULL);
	maki_key_file_to_file(serv->key_file, path);
	g_free(path);
}

gchar* maki_server_config_get_string (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_string(serv->key_file, group, key, NULL);
}

void maki_server_config_set_string (makiServer* serv, const gchar* group, const gchar* key, const gchar* string)
{
	gchar* path;

	g_key_file_set_string(serv->key_file, group, key, string);

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->server, NULL);
	maki_key_file_to_file(serv->key_file, path);
	g_free(path);
}

gchar** maki_server_config_get_string_list (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_string_list(serv->key_file, group, key, NULL, NULL);
}

void maki_server_config_set_string_list (makiServer* serv, const gchar* group, const gchar* key, gchar** list)
{
	gchar* path;

	g_key_file_set_string_list(serv->key_file, group, key, (const gchar* const*)list, g_strv_length(list));

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->server, NULL);
	maki_key_file_to_file(serv->key_file, path);
	g_free(path);
}

gboolean maki_server_config_remove (makiServer* serv, const gchar* group, const gchar* key)
{
	gchar* path;
	gboolean ret;

	ret = g_key_file_remove_key(serv->key_file, group, key, NULL);

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->server, NULL);
	maki_key_file_to_file(serv->key_file, path);
	g_free(path);

	return ret;
}

gboolean maki_server_config_exists (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_has_key(serv->key_file, group, key, NULL);
}

gboolean maki_server_autoconnect (makiServer* serv)
{
	return maki_server_config_get_boolean(serv, "server", "autoconnect");
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

	maki_server_disconnect(serv, NULL);

	if (serv->reconnect.source != 0)
	{
		maki_source_remove(serv->main_context, serv->reconnect.source);
	}

	g_main_loop_quit(serv->main_loop);

	g_main_loop_unref(serv->main_loop);
	g_main_context_unref(serv->main_context);

	maki_cache_remove(serv->users, serv->user->nick);

	g_key_file_free(serv->key_file);

	g_free(serv->support.prefix.prefixes);
	g_free(serv->support.prefix.modes);
	g_free(serv->support.chantypes);
	g_free(serv->support.chanmodes);
	g_hash_table_destroy(serv->logs);
	g_hash_table_destroy(serv->channels);
	maki_cache_free(serv->users);
	sashimi_free(serv->connection);
	g_free(serv->server);
	g_free(serv);
}

/* This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal. */
gboolean maki_server_connect (makiServer* serv)
{
	gboolean ret = TRUE;

	if (!serv->connected)
	{
		gchar* address;
		GTimeVal time;

		sashimi_connect_callback(serv->connection, maki_server_connect_callback, serv);
		sashimi_read_callback(serv->connection, maki_in_callback, serv);
		sashimi_reconnect_callback(serv->connection, maki_server_reconnect_callback, serv);

		g_get_current_time(&time);
		maki_dbus_emit_connect(time.tv_sec, serv->server);

		address = maki_server_config_get_string(serv, "server", "address");

		if (address != NULL && !sashimi_connect(serv->connection, address, maki_server_config_get_integer(serv, "server", "port")))
		{
			maki_server_reconnect_callback(serv);
			ret = FALSE;
		}

		g_free(address);
	}

	return ret;
}

/* This function is a wrapper around sashimi_disconnect(). */
gboolean maki_server_disconnect (makiServer* serv, const gchar* message)
{
	if (serv->connected)
	{
		sashimi_connect_callback(serv->connection, NULL, NULL);
		sashimi_read_callback(serv->connection, NULL, NULL);
		sashimi_reconnect_callback(serv->connection, NULL, NULL);

		if (message != NULL)
		{
			GHashTableIter iter;
			gpointer key, value;

			maki_out_quit(serv, message);

			g_hash_table_iter_init(&iter, serv->channels);

			while (g_hash_table_iter_next(&iter, &key, &value))
			{
				makiChannel* chan = value;

				maki_channel_set_joined(chan, FALSE);
			}
		}

		serv->connected = FALSE;
		serv->logged_in = FALSE;
		return sashimi_disconnect(serv->connection);
	}

	return TRUE;
}

/* This function handles unexpected reconnects. */
static gboolean maki_server_reconnect (gpointer data)
{
	makiServer* serv = data;

	maki_server_disconnect(serv, NULL);

	if (serv->reconnect.retries > 0)
	{
		serv->reconnect.retries--;
	}
	else if (serv->reconnect.retries == 0)
	{
		/* Finally give up. */
		serv->reconnect.source = 0;
		return FALSE;
	}

	if (maki_server_connect(serv))
	{
		serv->reconnect.source = 0;
		return FALSE;
	}

	return TRUE;
}

void maki_server_connect_callback (gpointer data)
{
	gchar* initial_nick;
	gchar* name;
	GTimeVal time;
	makiServer* serv = data;
	makiUser* user;

	if (serv->reconnect.source != 0)
	{
		maki_source_remove(serv->main_context, serv->reconnect.source);
		serv->reconnect.source = 0;
	}

	serv->reconnect.retries = maki_instance_config_get_integer(serv->instance, "reconnect", "retries");

	initial_nick = maki_server_config_get_string(serv, "server", "nick");
	name = maki_server_config_get_string(serv, "server", "name");

	user = maki_cache_insert(serv->users, initial_nick);
	maki_user_copy(serv->user, user);
	maki_cache_remove(serv->users, serv->user->nick);
	serv->user = user;

	maki_out_nick(serv, initial_nick);

	maki_server_send_printf(serv, "USER %s 0 * :%s", initial_nick, name);

	serv->connected = TRUE;

	g_get_current_time(&time);
	maki_dbus_emit_connected(time.tv_sec, serv->server);
	maki_dbus_emit_nick(time.tv_sec, serv->server, "", serv->user->nick);

	g_free(initial_nick);
	g_free(name);
}

/* This function is called by sashimi if the connection drops.
 * It schedules maki_server_reconnect() to be called regularly. */
void maki_server_reconnect_callback (gpointer data)
{
	makiServer* serv = data;

	/* Prevent maki_server_reconnect() from running twice. */
	if (serv->reconnect.source != 0)
	{
		return;
	}

	serv->reconnect.source = maki_timeout_add_seconds(serv->main_context, maki_instance_config_get_integer(serv->instance, "reconnect", "timeout"), maki_server_reconnect, serv);
}
