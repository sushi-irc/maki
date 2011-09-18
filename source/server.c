/*
 * Copyright (c) 2008-2011 Michael Kuhn
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <string.h>

#include <ilib.h>

#include "server.h"

#include "dbus.h"
#include "in.h"
#include "instance.h"
#include "log.h"
#include "maki.h"
#include "misc.h"
#include "out.h"

enum makiServerStatus
{
	MAKI_SERVER_STATUS_DISCONNECTED,
	MAKI_SERVER_STATUS_CONNECTING,
	MAKI_SERVER_STATUS_CONNECTED
};

typedef enum makiServerStatus makiServerStatus;

struct makiServerIdleOperation
{
	union
	{
		struct
		{
			makiServer* server;
		}
		connect;

		struct
		{
			makiServer* server;
			gchar* message;
		}
		disconnect;
	}
	u;
};

typedef struct makiServerIdleOperation makiServerIdleOperation;

struct maki_server
{
	makiInstance* instance;

	gchar* name;
	makiServerStatus status;
	gboolean logged_in;
	sashimiConnection* connection;
	GHashTable* channels;
	GHashTable* users;
	GHashTable* logs;

	makiUser* user;

	GKeyFile* key_file;

	struct
	{
		gint retries;
		guint source;
	}
	reconnect;

	struct
	{
		guint away;
	}
	sources;

	struct
	{
		gchar* chanmodes;
		gchar* chantypes;

		struct
		{
			gchar* modes;
			gchar* prefixes;
		}
		prefix;
	}
	support;

	GMainContext* main_context;
	GMainLoop* main_loop;
	GThread* thread;

	GMutex* mutex;

	gint ref_count;
};

static
void
maki_server_internal_log_valist (makiServer* serv, const gchar* name, const gchar* format, va_list args)
{
	gchar* file;
	gchar* file_tmp;
	gchar* file_format;
	gchar* tmp;
	makiLog* log;

	if (!maki_instance_config_get_boolean(serv->instance, "logging", "enabled"))
	{
		return;
	}

	file_format = maki_instance_config_get_string(serv->instance, "logging", "format");
	file_tmp = i_strreplace(file_format, "$n", name, 0);
	file = i_get_current_time_string(file_tmp);

	g_free(file_format);
	g_free(file_tmp);

	if (file == NULL)
	{
		return;
	}

	if ((log = g_hash_table_lookup(serv->logs, file)) == NULL)
	{
		if ((log = maki_log_new(serv->instance, serv->name, file)) == NULL)
		{
			g_free(file);
			return;
		}

		g_hash_table_insert(serv->logs, file, log);
	}
	else
	{
		g_free(file);
	}

	tmp = g_strdup_vprintf(format, args);
	maki_log_write(log, tmp);
	g_free(tmp);
}

static
void
maki_server_internal_log (makiServer* serv, const gchar* name, const gchar* format, ...)
{
	va_list args;

	va_start(args, format);
	maki_server_internal_log_valist(serv, name, format, args);
	va_end(args);
}

static
gboolean
maki_server_internal_sendf_valist (makiServer* serv, gchar const* format, va_list args)
{
	gboolean ret;
	gchar* buffer;

	buffer = g_strdup_vprintf(format, args);
	ret = sashimi_send(serv->connection, buffer);
	g_free(buffer);

	return ret;
}

static
gboolean
maki_server_internal_sendf (makiServer* serv, gchar const* format, ...)
{
	gboolean ret;
	va_list args;

	va_start(args, format);
	ret = maki_server_internal_sendf_valist(serv, format, args);
	va_end(args);

	return ret;
}

static
makiUser*
maki_server_internal_add_user (makiServer* serv, gchar const* nick)
{
	makiUser* ret;

	if ((ret = g_hash_table_lookup(serv->users, nick)) != NULL)
	{
		ret = maki_user_ref(ret);
	}
	else
	{
		ret = maki_user_new(nick);
		g_hash_table_insert(serv->users, g_strdup(nick), ret);
	}

	return ret;
}

static
gboolean
maki_server_internal_remove_user (makiServer* serv, gchar const* nick)
{
	makiUser* user;
	gboolean ret = FALSE;

	if ((user = g_hash_table_lookup(serv->users, nick)) != NULL)
	{
		if (maki_user_ref_count(user) == 1)
		{
			ret = g_hash_table_remove(serv->users, nick);
		}

		maki_user_unref(user);
	}

	return ret;
}

static
gboolean
maki_server_away (gpointer data)
{
	makiServer* serv = data;
	GHashTableIter iter;
	gpointer key, value;

	g_mutex_lock(serv->mutex);

	g_hash_table_iter_init(&iter, serv->channels);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		gchar const* chan_name = key;
		makiChannel* chan = value;

		if (maki_channel_joined(chan) && maki_channel_users_count(chan) <= 100)
		{
			maki_server_internal_sendf(serv, "WHO %s", chan_name);
		}
	}

	g_mutex_unlock(serv->mutex);

	return TRUE;
}

static
gpointer
maki_server_thread (gpointer data)
{
	makiServer* serv = data;

	g_main_context_push_thread_default(serv->main_context);
	g_main_loop_run(serv->main_loop);

	return NULL;
}

static gboolean maki_server_timeout_reconnect (gpointer);

/* This function is called by sashimi if the connection drops.
 * It schedules maki_server_reconnect() to be called regularly. */
static
void
maki_server_on_disconnect (gpointer data)
{
	makiServer* serv = data;

	g_mutex_lock(serv->mutex);

	/* Prevent maki_server_reconnect() from running twice. */
	if (serv->reconnect.source == 0)
	{
		serv->reconnect.source = i_timeout_add_seconds(maki_instance_config_get_integer(serv->instance, "reconnect", "timeout"), maki_server_timeout_reconnect, serv, serv->main_context);
	}

	g_mutex_unlock(serv->mutex);
}

static
void
maki_server_internal_disconnect (makiServer* serv, gchar const* message)
{
	sashimi_connect_callback(serv->connection, NULL, NULL);
	sashimi_disconnect_callback(serv->connection, NULL, NULL);
	sashimi_read_callback(serv->connection, NULL, NULL);

	if (serv->sources.away != 0)
	{
		i_source_remove(serv->sources.away, serv->main_context);
		serv->sources.away = 0;
	}

	if (message != NULL)
	{
		GHashTableIter iter;
		gpointer key, value;

		if (message[0] != '\0')
		{
			maki_server_internal_sendf(serv, "QUIT :%s", message);
		}
		else
		{
			sashimi_send(serv->connection, "QUIT");
		}

		g_hash_table_iter_init(&iter, serv->channels);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			const gchar* chan_name = key;
			makiChannel* chan = value;

			if (maki_channel_joined(chan))
			{
				if (message[0] != '\0')
				{
					maki_server_internal_log(serv, chan_name, _("« You quit (%s)."), message);
				}
				else
				{
					maki_server_internal_log(serv, chan_name, _("« You quit."));
				}
			}

			maki_channel_set_joined(chan, FALSE);
		}

		maki_dbus_emit_quit(serv->name, maki_user_from(serv->user), message);
	}

	serv->status = MAKI_SERVER_STATUS_DISCONNECTED;
	serv->logged_in = FALSE;

	/* FIXME disconnect signal */

	sashimi_disconnect(serv->connection);
}

static
gboolean
maki_server_idle_disconnect (gpointer data)
{
	makiServerIdleOperation* op = data;
	makiServer* serv = op->u.disconnect.server;
	gchar* message = op->u.disconnect.message;

	g_mutex_lock(serv->mutex);
	maki_server_internal_disconnect(serv, message);
	g_mutex_unlock(serv->mutex);

	g_free(op->u.disconnect.message);
	g_free(op);

	return FALSE;
}

static
void
maki_server_on_connect (gpointer data)
{
	gchar* user;
	gchar* name;
	gchar* nick;
	makiServer* serv = data;

	g_mutex_lock(serv->mutex);

	if (serv->reconnect.source != 0)
	{
		i_source_remove(serv->reconnect.source, serv->main_context);
		serv->reconnect.source = 0;
	}

	nick = g_key_file_get_string(serv->key_file, "server", "nick", NULL);
	user = g_key_file_get_string(serv->key_file, "server", "user", NULL);
	name = g_key_file_get_string(serv->key_file, "server", "name", NULL);

	maki_server_internal_remove_user(serv, maki_user_nick(serv->user));
	serv->user = maki_server_internal_add_user(serv, nick);

	maki_server_internal_sendf(serv, "NICK %s", nick);
	maki_server_internal_sendf(serv, "USER %s 0 * :%s", user, name);

	serv->status = MAKI_SERVER_STATUS_CONNECTED;

	maki_dbus_emit_connected(serv->name);
	maki_dbus_emit_nick(serv->name, "", maki_user_nick(serv->user));

	g_free(nick);
	g_free(user);
	g_free(name);

	if (serv->sources.away != 0)
	{
		i_source_remove(serv->sources.away, serv->main_context);
	}

	serv->sources.away = i_timeout_add_seconds(60, maki_server_away, serv, serv->main_context);

	g_mutex_unlock(serv->mutex);
}

static
gboolean
maki_server_internal_connect (makiServer* serv)
{
	gboolean ret;
	makiNetwork* net = maki_instance_network(serv->instance);
	gboolean ssl;
	gchar* address;
	gint port;

	maki_network_update(net);

	sashimi_connect_callback(serv->connection, maki_server_on_connect, serv);
	sashimi_disconnect_callback(serv->connection, maki_server_on_disconnect, serv);
	sashimi_read_callback(serv->connection, maki_in_callback, serv);

	maki_dbus_emit_connect(serv->name);

	address = g_key_file_get_string(serv->key_file, "server", "address", NULL);
	port = g_key_file_get_integer(serv->key_file, "server", "port", NULL);
	ssl = g_key_file_get_boolean(serv->key_file, "server", "ssl", NULL);

	ret = sashimi_connect(serv->connection, address, port, ssl);

	g_free(address);

	return ret;
}

static
gboolean
maki_server_idle_connect (gpointer data)
{
	makiServerIdleOperation* op = data;
	makiServer* serv = op->u.connect.server;

	g_mutex_lock(serv->mutex);
	maki_server_internal_connect(serv);
	g_mutex_unlock(serv->mutex);

	g_free(op);

	return FALSE;
}

/* This function handles unexpected reconnects. */
static
gboolean
maki_server_timeout_reconnect (gpointer data)
{
	gboolean ret;
	makiServer* serv = data;

	g_mutex_lock(serv->mutex);

	ret = (serv->reconnect.retries > 0);
	maki_server_internal_disconnect(serv, NULL);

	if (ret)
	{
		serv->reconnect.retries--;

		if (maki_server_internal_connect(serv))
		{
			ret = FALSE;
			serv->reconnect.source = 0;
		}
	}
	else
	{
		/* Finally give up. */
		serv->reconnect.source = 0;
	}

	g_mutex_unlock(serv->mutex);

	return ret;
}

static
void
maki_server_config_save (makiServer* serv)
{
	gchar* path;

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), serv->name, NULL);
	i_key_file_to_file(serv->key_file, path, NULL, NULL);
	g_free(path);
}

static
void
maki_server_config_set_defaults (makiServer* serv)
{
	if (!g_key_file_has_key(serv->key_file, "server", "autoconnect", NULL))
	{
		g_key_file_set_boolean(serv->key_file, "server", "autoconnect", FALSE);
	}

	if (!g_key_file_has_key(serv->key_file, "server", "port", NULL))
	{
		g_key_file_set_integer(serv->key_file, "server", "port", 6667);
	}

	if (!g_key_file_has_key(serv->key_file, "server", "ssl", NULL))
	{
		g_key_file_set_boolean(serv->key_file, "server", "ssl", FALSE);
	}

	if (!g_key_file_has_key(serv->key_file, "server", "nick", NULL))
	{
		g_key_file_set_string(serv->key_file, "server", "nick", g_get_user_name());
	}

	if (!g_key_file_has_key(serv->key_file, "server", "user", NULL))
	{
		g_key_file_set_string(serv->key_file, "server", "user", g_get_user_name());
	}

	if (!g_key_file_has_key(serv->key_file, "server", "name", NULL))
	{
		g_key_file_set_string(serv->key_file, "server", "name", g_get_real_name());
	}

	if (!g_key_file_has_key(serv->key_file, "server", "nickserv", NULL))
	{
		g_key_file_set_string(serv->key_file, "server", "nickserv", "");
	}

	if (!g_key_file_has_key(serv->key_file, "server", "nickserv_ghost", NULL))
	{
		g_key_file_set_boolean(serv->key_file, "server", "nickserv_ghost", FALSE);
	}

	maki_server_config_save(serv);

	/*
		commands = g_key_file_get_string_list(key_file, "server", "commands", NULL, NULL);
		ignores = g_key_file_get_string_list(key_file, "server", "ignores", NULL, NULL);
	*/
}

makiServer*
maki_server_new (gchar const* name)
{
	gchar* nick;
	gchar* path;
	gchar** group;
	gchar** groups;
	makiServer* serv;

	g_return_val_if_fail(name != NULL, NULL);

	serv = g_new(makiServer, 1);
	serv->instance = maki_instance_get_default();
	serv->name = g_strdup(name);
	serv->key_file = g_key_file_new();
	serv->status = MAKI_SERVER_STATUS_DISCONNECTED;
	serv->logged_in = FALSE;
	serv->reconnect.source = 0;
	serv->reconnect.retries = maki_instance_config_get_integer(serv->instance, "reconnect" ,"retries");
	serv->sources.away = 0;
	serv->main_context = g_main_context_new();
	serv->main_loop = g_main_loop_new(serv->main_context, FALSE);
	serv->connection = sashimi_new(serv->main_context);
	serv->channels = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, maki_channel_free);
	serv->users = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, NULL);
	serv->logs = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, maki_log_free);

	path = g_build_filename(maki_instance_directory(serv->instance, "servers"), name, NULL);
	g_key_file_load_from_file(serv->key_file, path, G_KEY_FILE_NONE, NULL);
	g_free(path);

	maki_server_config_set_defaults(serv);

	nick = g_key_file_get_string(serv->key_file, "server", "nick", NULL);
	serv->user = maki_server_internal_add_user(serv, nick);
	g_free(nick);

	serv->support.chanmodes = NULL;
	serv->support.chantypes = g_strdup("#&");
	serv->support.prefix.modes = g_strdup("ov");
	serv->support.prefix.prefixes = g_strdup("@+");

	serv->ref_count = 1;

	sashimi_timeout(serv->connection, 60);

	serv->mutex = g_mutex_new();

	groups = g_key_file_get_groups(serv->key_file, NULL);

	for (group = groups; *group != NULL; group++)
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

makiServer*
maki_server_ref (makiServer* serv)
{
	g_return_val_if_fail(serv != NULL, NULL);

	g_atomic_int_inc(&(serv->ref_count));

	return serv;
}

/* This function gets called when a server is removed from the servers hash table. */
void
maki_server_unref (gpointer data)
{
	makiServer* serv = data;

	g_return_if_fail(serv != NULL);

	if (g_atomic_int_dec_and_test(&(serv->ref_count)))
	{
		maki_server_disconnect(serv, NULL);

		g_mutex_free(serv->mutex);

		if (serv->reconnect.source != 0)
		{
			i_source_remove(serv->reconnect.source, serv->main_context);
		}

		g_main_loop_quit(serv->main_loop);
		g_thread_join(serv->thread);

		g_main_loop_unref(serv->main_loop);
		g_main_context_unref(serv->main_context);

		maki_server_internal_remove_user(serv, maki_user_nick(serv->user));

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

gboolean
maki_server_config_get_boolean (makiServer* serv, gchar const* group, gchar const* key)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(group != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_boolean(serv->key_file, group, key, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_config_set_boolean (makiServer* serv, gchar const* group, gchar const* key, gboolean value)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(group != NULL);
	g_return_if_fail(key != NULL);

	g_mutex_lock(serv->mutex);
	g_key_file_set_boolean(serv->key_file, group, key, value);
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);
}

gint
maki_server_config_get_integer (makiServer* serv, gchar const* group, gchar const* key)
{
	gint ret;

	g_return_val_if_fail(serv != NULL, -1);
	g_return_val_if_fail(group != NULL, -1);
	g_return_val_if_fail(key != NULL, -1);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_integer(serv->key_file, group, key, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_config_set_integer (makiServer* serv, gchar const* group, gchar const* key, gint value)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(group != NULL);
	g_return_if_fail(key != NULL);

	g_mutex_lock(serv->mutex);
	g_key_file_set_integer(serv->key_file, group, key, value);
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);
}

gchar*
maki_server_config_get_string (makiServer* serv, gchar const* group, gchar const* key)
{
	gchar* ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(group != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_string(serv->key_file, group, key, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_config_set_string (makiServer* serv, gchar const* group, gchar const* key, gchar const* string)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(group != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(string != NULL);

	g_mutex_lock(serv->mutex);
	g_key_file_set_string(serv->key_file, group, key, string);
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);
}

gchar**
maki_server_config_get_string_list (makiServer* serv, gchar const* group, gchar const* key)
{
	gchar** ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(group != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_string_list(serv->key_file, group, key, NULL, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_config_set_string_list (makiServer* serv, gchar const* group, gchar const* key, gchar** list)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(group != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(list != NULL);

	g_mutex_lock(serv->mutex);
	g_key_file_set_string_list(serv->key_file, group, key, (gchar const* const*)list, g_strv_length(list));
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);
}

gboolean
maki_server_config_remove_key (makiServer* serv, gchar const* group, gchar const* key)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(group != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_remove_key(serv->key_file, group, key, NULL);
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_config_remove_group (makiServer* serv, gchar const* group)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(group != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_remove_group(serv->key_file, group, NULL);
	maki_server_config_save(serv);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gchar**
maki_server_config_get_keys (makiServer* serv, gchar const* group)
{
	gchar** ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(group != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_keys(serv->key_file, group, NULL, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gchar**
maki_server_config_get_groups (makiServer* serv)
{
	gchar** ret;

	g_return_val_if_fail(serv != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret =  g_key_file_get_groups(serv->key_file, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_config_exists (makiServer* serv, gchar const* group, gchar const* key)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(group != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_has_key(serv->key_file, group, key, NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gchar const*
maki_server_name (makiServer* serv)
{
	gchar const* ret;

	g_return_val_if_fail(serv != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = serv->name;
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_autoconnect (makiServer* serv)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_key_file_get_boolean(serv->key_file, "server", "autoconnect", NULL);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_connected (makiServer* serv)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = (serv->status == MAKI_SERVER_STATUS_CONNECTED);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_logged_in (makiServer* serv)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = serv->logged_in;
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_set_logged_in (makiServer* serv, gboolean logged_in)
{
	g_return_if_fail(serv != NULL);

	g_mutex_lock(serv->mutex);
	serv->logged_in = logged_in;
	g_mutex_unlock(serv->mutex);
}

makiUser*
maki_server_user (makiServer* serv)
{
	makiUser* ret;

	g_return_val_if_fail(serv != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = serv->user;
	g_mutex_unlock(serv->mutex);

	return ret;
}

gchar const*
maki_server_support (makiServer* serv, makiServerSupport support)
{
	gchar const* ret = NULL;

	g_return_val_if_fail(serv != NULL, NULL);

	g_mutex_lock(serv->mutex);

	switch (support)
	{
		case MAKI_SERVER_SUPPORT_CHANMODES:
			ret = serv->support.chanmodes;
			break;
		case MAKI_SERVER_SUPPORT_CHANTYPES:
			ret = serv->support.chantypes;
			break;
		case MAKI_SERVER_SUPPORT_PREFIX_MODES:
			ret = serv->support.prefix.modes;
			break;
		case MAKI_SERVER_SUPPORT_PREFIX_PREFIXES:
			ret = serv->support.prefix.prefixes;
			break;
		default:
			g_warn_if_reached();
	}

	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_set_support (makiServer* serv, makiServerSupport support, gchar const* value)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(value != NULL);

	g_mutex_lock(serv->mutex);

	switch (support)
	{
		case MAKI_SERVER_SUPPORT_CHANMODES:
			g_free(serv->support.chanmodes);
			serv->support.chanmodes = g_strdup(value);
			break;
		case MAKI_SERVER_SUPPORT_CHANTYPES:
			g_free(serv->support.chantypes);
			serv->support.chantypes = g_strdup(value);
			break;
		case MAKI_SERVER_SUPPORT_PREFIX_MODES:
			g_free(serv->support.prefix.modes);
			serv->support.prefix.modes = g_strdup(value);
			break;
		case MAKI_SERVER_SUPPORT_PREFIX_PREFIXES:
			g_free(serv->support.prefix.prefixes);
			serv->support.prefix.prefixes = g_strdup(value);
			break;
		default:
			g_warn_if_reached();
	}

	g_mutex_unlock(serv->mutex);
}

makiUser*
maki_server_add_user (makiServer* serv, gchar const* nick)
{
	makiUser* ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(nick != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = maki_server_internal_add_user(serv, nick);
	g_mutex_unlock(serv->mutex);

	return ret;
}

makiUser*
maki_server_get_user (makiServer* serv, gchar const* nick)
{
	makiUser* ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(nick != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = g_hash_table_lookup(serv->users, nick);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_remove_user (makiServer* serv, gchar const* nick)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(nick != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = maki_server_internal_remove_user(serv, nick);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_rename_user (makiServer* serv, gchar const* old_nick, gchar const* new_nick)
{
	makiUser* user;
	gchar* tmp = NULL;
	gboolean ret = TRUE;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(old_nick != NULL, FALSE);
	g_return_val_if_fail(new_nick != NULL, FALSE);

	g_mutex_lock(serv->mutex);

	if (g_hash_table_lookup(serv->users, new_nick) != NULL)
	{
		ret = FALSE;
		goto end;
	}

	if ((user = g_hash_table_lookup(serv->users, old_nick)) == NULL)
	{
		ret = FALSE;
		goto end;
	}

	/* old_nick usually points to maki_user_nick(user). */
	tmp = g_strdup(old_nick);

	maki_user_set_nick(user, new_nick);
	/* old_nick is probably invalid from here on! */
	g_hash_table_insert(serv->users, g_strdup(new_nick), user);
	g_hash_table_remove(serv->users, tmp);

end:
	g_free(tmp);

	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_add_channel (makiServer* serv, gchar const* name, makiChannel* chan)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(name != NULL);
	g_return_if_fail(chan != NULL);

	g_mutex_lock(serv->mutex);
	g_hash_table_insert(serv->channels, g_strdup(name), chan);
	g_mutex_unlock(serv->mutex);
}

makiChannel*
maki_server_get_channel (makiServer* serv, gchar const* name)
{
	makiChannel* ret;

	g_return_val_if_fail(serv != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	g_mutex_lock(serv->mutex);
	ret = g_hash_table_lookup(serv->channels, name);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_remove_channel (makiServer* serv, gchar const* name)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = g_hash_table_remove(serv->channels, name);
	g_mutex_unlock(serv->mutex);

	return ret;
}

guint
maki_server_channels_count (makiServer* serv)
{
	guint ret;

	g_return_val_if_fail(serv != NULL, 0);

	g_mutex_lock(serv->mutex);
	ret = g_hash_table_size(serv->channels);
	g_mutex_unlock(serv->mutex);

	return ret;
}

void
maki_server_channels_iter (makiServer* serv, GHashTableIter* iter)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(iter != NULL);

	g_mutex_lock(serv->mutex);
	g_hash_table_iter_init(iter, serv->channels);
	g_mutex_unlock(serv->mutex);
}

void
maki_server_log (makiServer* serv, const gchar* name, const gchar* format, ...)
{
	va_list args;

	g_return_if_fail(serv != NULL);
	g_return_if_fail(name != NULL);
	g_return_if_fail(format != NULL);

	g_mutex_lock(serv->mutex);
	va_start(args, format);
	maki_server_internal_log_valist(serv, name, format, args);
	va_end(args);
	g_mutex_unlock(serv->mutex);
}

gboolean
maki_server_queue (makiServer* serv, gchar const* message, gboolean force)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(serv->mutex);

	if (force)
	{
		ret = sashimi_queue(serv->connection, message);
	}
	else
	{
		ret = sashimi_send_or_queue(serv->connection, message);
	}

	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_send (makiServer* serv, gchar const* message)
{
	gboolean ret;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	ret = sashimi_send(serv->connection, message);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_send_printf (makiServer* serv, gchar const* format, ...)
{
	gboolean ret;
	va_list args;

	g_return_val_if_fail(serv != NULL, FALSE);
	g_return_val_if_fail(format != NULL, FALSE);

	g_mutex_lock(serv->mutex);
	va_start(args, format);
	ret = maki_server_internal_sendf_valist(serv, format, args);
	va_end(args);
	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_connect (makiServer* serv)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(serv != NULL, FALSE);

	g_mutex_lock(serv->mutex);

	if (serv->status == MAKI_SERVER_STATUS_DISCONNECTED)
	{
		makiServerIdleOperation* op;

		op = g_new(makiServerIdleOperation, 1);
		op->u.connect.server = serv;

		serv->status = MAKI_SERVER_STATUS_CONNECTING;
		serv->reconnect.retries = maki_instance_config_get_integer(serv->instance, "reconnect", "retries");

		i_idle_add(maki_server_idle_connect, op, serv->main_context);
		ret = TRUE;
	}

	g_mutex_unlock(serv->mutex);

	return ret;
}

gboolean
maki_server_disconnect (makiServer* serv, gchar const* message)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(serv != NULL, FALSE);

	g_mutex_lock(serv->mutex);

	if (serv->status != MAKI_SERVER_STATUS_DISCONNECTED)
	{
		makiServerIdleOperation* op;

		op = g_new(makiServerIdleOperation, 1);
		op->u.disconnect.server = serv;
		op->u.disconnect.message = g_strdup(message);

		i_idle_add(maki_server_idle_disconnect, op, serv->main_context);
		ret = TRUE;
	}

	g_mutex_unlock(serv->mutex);

	return ret;
}
