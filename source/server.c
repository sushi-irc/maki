/*
 * Copyright (c) 2008-2009 Michael Kuhn
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

#include "in.h"
#include "instance.h"
#include "log.h"
#include "misc.h"
#include "out.h"
#include "server.h"

#include "ilib.h"

#include <string.h>

static gboolean maki_server_away (gpointer data)
{
	makiServer* serv = data;
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init(&iter, serv->channels);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		const gchar* chan_name = key;
		makiChannel* chan = value;

		if (maki_channel_joined(chan) && maki_channel_users_count(chan) <= 100)
		{
			maki_server_send_printf(serv, "WHO %s", chan_name);
		}
	}

	return TRUE;
}

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

	if (!maki_server_config_exists(serv, "server", "ssl"))
	{
		maki_server_config_set_boolean(serv, "server", "ssl", FALSE);
	}

	if (!maki_server_config_exists(serv, "server", "nick"))
	{
		maki_server_config_set_string(serv, "server", "nick", g_get_user_name());
	}

	if (!maki_server_config_exists(serv, "server", "user"))
	{
		maki_server_config_set_string(serv, "server", "user", g_get_user_name());
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

makiServer* maki_server_new (const gchar* name)
{
	gchar* nick;
	gchar* path;
	gchar** group;
	gchar** groups;
	makiInstance* inst = maki_instance_get_default();
	makiServer* serv;

	serv = g_new(makiServer, 1);
	serv->name = g_strdup(name);
	serv->key_file = g_key_file_new();
	serv->connected = FALSE;
	serv->logged_in = FALSE;
	serv->reconnect.source = 0;
	serv->reconnect.retries = maki_instance_config_get_integer(inst, "reconnect" ,"retries");
	serv->sources.away = 0;
	serv->main_context = g_main_context_new();
	serv->main_loop = g_main_loop_new(serv->main_context, FALSE);
	serv->connection = sashimi_new(serv->main_context);
	serv->channels = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, maki_channel_free);
	serv->users = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, NULL);
	serv->logs = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, maki_log_free);

	path = g_build_filename(maki_instance_directory(inst, "servers"), name, NULL);
	g_key_file_load_from_file(serv->key_file, path, G_KEY_FILE_NONE, NULL);
	g_free(path);

	maki_server_config_set_defaults(serv);

	nick = maki_server_config_get_string(serv, "server", "nick");
	serv->user = maki_user_new(serv, nick);
	g_free(nick);

	serv->support.chanmodes = NULL;
	serv->support.chantypes = g_strdup("#&");
	serv->support.prefix.modes = g_strdup("ov");
	serv->support.prefix.prefixes = g_strdup("@+");

	serv->ref_count = 1;

	sashimi_timeout(serv->connection, 60);

	groups = g_key_file_get_groups(serv->key_file, NULL);

	for (group = groups; *group != NULL; ++group)
	{
		if (strcmp(*group, "server") != 0)
		{
			g_hash_table_insert(serv->channels, g_strdup(*group), maki_channel_new(serv, *group));
		}
	}

	g_strfreev(groups);

	serv->thread = g_thread_create(maki_server_thread, serv, TRUE, NULL);

	if (maki_server_autoconnect(serv))
	{
		maki_server_connect(serv);
	}

	return serv;
}

gboolean maki_server_config_get_boolean (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_boolean(serv->key_file, group, key, NULL);
}

void maki_server_config_set_boolean (makiServer* serv, const gchar* group, const gchar* key, gboolean value)
{
	gchar* path;
	makiInstance* inst = maki_instance_get_default();

	g_key_file_set_boolean(serv->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);
}

gint maki_server_config_get_integer (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_integer(serv->key_file, group, key, NULL);
}

void maki_server_config_set_integer (makiServer* serv, const gchar* group, const gchar* key, gint value)
{
	gchar* path;
	makiInstance* inst = maki_instance_get_default();

	g_key_file_set_integer(serv->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);
}

gchar* maki_server_config_get_string (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_string(serv->key_file, group, key, NULL);
}

void maki_server_config_set_string (makiServer* serv, const gchar* group, const gchar* key, const gchar* string)
{
	gchar* path;
	makiInstance* inst = maki_instance_get_default();

	g_key_file_set_string(serv->key_file, group, key, string);

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);
}

gchar** maki_server_config_get_string_list (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_get_string_list(serv->key_file, group, key, NULL, NULL);
}

void maki_server_config_set_string_list (makiServer* serv, const gchar* group, const gchar* key, gchar** list)
{
	gchar* path;
	makiInstance* inst = maki_instance_get_default();

	g_key_file_set_string_list(serv->key_file, group, key, (const gchar* const*)list, g_strv_length(list));

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);
}

gboolean maki_server_config_remove_key (makiServer* serv, const gchar* group, const gchar* key)
{
	gchar* path;
	gboolean ret;
	makiInstance* inst = maki_instance_get_default();

	ret = g_key_file_remove_key(serv->key_file, group, key, NULL);

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);

	return ret;
}

gboolean maki_server_config_remove_group (makiServer* serv, const gchar* group)
{
	gchar* path;
	gboolean ret;
	makiInstance* inst = maki_instance_get_default();

	ret = g_key_file_remove_group(serv->key_file, group, NULL);

	path = g_build_filename(maki_instance_directory(inst, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);

	return ret;
}

gchar** maki_server_config_get_keys (makiServer* serv, const gchar* group)
{
	return g_key_file_get_keys(serv->key_file, group, NULL, NULL);
}

gchar** maki_server_config_get_groups (makiServer* serv)
{
	return g_key_file_get_groups(serv->key_file, NULL);
}

gboolean maki_server_config_exists (makiServer* serv, const gchar* group, const gchar* key)
{
	return g_key_file_has_key(serv->key_file, group, key, NULL);
}

makiUser* maki_server_user (makiServer* serv)
{
	return serv->user;
}

void maki_server_add_user (makiServer* serv, const gchar* nick, makiUser* user)
{
	g_hash_table_insert(serv->users, g_strdup(nick), user);
}

makiUser* maki_server_get_user (makiServer* serv, const gchar* nick)
{
	return g_hash_table_lookup(serv->users, nick);
}

gboolean maki_server_remove_user (makiServer* serv, const gchar* nick)
{
	return g_hash_table_remove(serv->users, nick);
}

const gchar* maki_server_name (makiServer* serv)
{
	return serv->name;
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

gboolean maki_server_remove_channel (makiServer* serv, const gchar* name)
{
	return g_hash_table_remove(serv->channels, name);
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

makiServer* maki_server_ref (makiServer* serv)
{
	g_return_val_if_fail(serv != NULL, NULL);

	serv->ref_count++;

	return serv;
}

/* This function gets called when a server is removed from the servers hash table. */
void maki_server_unref (gpointer data)
{
	makiServer* serv = data;

	serv->ref_count--;

	if (serv->ref_count == 0)
	{
		maki_server_disconnect(serv, NULL);

		if (serv->reconnect.source != 0)
		{
			i_source_remove(serv->reconnect.source, serv->main_context);
		}

		g_main_loop_quit(serv->main_loop);
		g_thread_join(serv->thread);

		g_main_loop_unref(serv->main_loop);
		g_main_context_unref(serv->main_context);

		maki_user_unref(serv->user);

		g_key_file_free(serv->key_file);

		g_free(serv->support.prefix.prefixes);
		g_free(serv->support.prefix.modes);
		g_free(serv->support.chantypes);
		g_free(serv->support.chanmodes);
		g_hash_table_destroy(serv->logs);
		g_hash_table_destroy(serv->channels);
		g_hash_table_destroy(serv->users);
		sashimi_free(serv->connection);
		g_free(serv->name);
		g_free(serv);
	}
}

/* This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal. */
gboolean maki_server_connect (makiServer* serv)
{
	gchar* address;
	gboolean ret = TRUE;

	address = maki_server_config_get_string(serv, "server", "address");

	if (!serv->connected && !maki_config_is_empty(address))
	{
		GTimeVal timeval;
		makiInstance* inst = maki_instance_get_default();
		makiNetwork* net = maki_instance_network(inst);

		maki_network_update(net);

		sashimi_connect_callback(serv->connection, maki_server_connect_callback, serv);
		sashimi_read_callback(serv->connection, maki_in_callback, serv);
		sashimi_reconnect_callback(serv->connection, maki_server_reconnect_callback, serv);

		g_get_current_time(&timeval);
		maki_dbus_emit_connect(timeval.tv_sec, serv->name);

		if (!sashimi_connect(serv->connection, address, maki_server_config_get_integer(serv, "server", "port"), maki_server_config_get_boolean(serv, "server", "ssl")))
		{
			maki_server_reconnect_callback(serv);
			ret = FALSE;
		}
	}

	g_free(address);

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

		if (serv->sources.away != 0)
		{
			i_source_remove(serv->sources.away, serv->main_context);
			serv->sources.away = 0;
		}

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

		/* FIXME disconnect signal */

		return sashimi_disconnect(serv->connection);
	}

	return TRUE;
}

void maki_server_reset (makiServer* serv)
{
	makiInstance* inst = maki_instance_get_default();

	serv->reconnect.retries = maki_instance_config_get_integer(inst, "reconnect", "retries");
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
	gchar* user;
	gchar* name;
	GTimeVal timeval;
	makiServer* serv = data;

	if (serv->reconnect.source != 0)
	{
		i_source_remove(serv->reconnect.source, serv->main_context);
		serv->reconnect.source = 0;
	}

	initial_nick = maki_server_config_get_string(serv, "server", "nick");
	user = maki_server_config_get_string(serv, "server", "user");
	name = maki_server_config_get_string(serv, "server", "name");

	maki_user_set_nick(serv->user, initial_nick);

	maki_out_nick(serv, initial_nick);

	maki_server_send_printf(serv, "USER %s 0 * :%s", user, name);

	serv->connected = TRUE;

	g_get_current_time(&timeval);
	maki_dbus_emit_connected(timeval.tv_sec, serv->name);
	maki_dbus_emit_nick(timeval.tv_sec, serv->name, "", maki_user_nick(serv->user));

	g_free(initial_nick);
	g_free(user);
	g_free(name);

	if (serv->sources.away != 0)
	{
		i_source_remove(serv->sources.away, serv->main_context);
	}

	serv->sources.away = i_timeout_add_seconds(60, maki_server_away, serv, serv->main_context);
}

/* This function is called by sashimi if the connection drops.
 * It schedules maki_server_reconnect() to be called regularly. */
void maki_server_reconnect_callback (gpointer data)
{
	makiInstance* inst = maki_instance_get_default();
	makiServer* serv = data;

	/* Prevent maki_server_reconnect() from running twice. */
	if (serv->reconnect.source != 0)
	{
		return;
	}

	serv->reconnect.source = i_timeout_add_seconds(maki_instance_config_get_integer(inst, "reconnect", "timeout"), maki_server_reconnect, serv, serv->main_context);
}
