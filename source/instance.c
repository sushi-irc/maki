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

#include "instance.h"
#include "plugin.h"

#include "ilib.h"

#include <gmodule.h>

struct maki_instance
{
	GKeyFile* key_file;

	GHashTable* servers;

	GHashTable* directories;

	GModule* plugins[1];

	struct
	{
		guint64 id;
		GSList* list;
		GMutex* mutex;
	}
	dcc;

	makiNetwork* network;
};

makiInstance* maki_instance_get_default (void)
{
	static makiInstance* inst = NULL;

	if (G_UNLIKELY(inst == NULL))
	{
		inst = maki_instance_new();
	}

	return inst;
}

static makiDCCSend* maki_instance_get_dcc_send (makiInstance* inst, guint64 id)
{
	GSList* list;

	for (list = inst->dcc.list; list != NULL; list = list->next)
	{
		makiDCCSend* dcc = list->data;

		if (maki_dcc_send_id(dcc) == id)
		{
			return dcc;
		}
	}

	return NULL;
}

static void maki_instance_config_set_defaults (makiInstance* inst)
{
	if (!maki_instance_config_exists(inst, "dcc", "accept_chat"))
	{
		maki_instance_config_set_boolean(inst, "dcc", "accept_chat", FALSE);
	}

	if (!maki_instance_config_exists(inst, "dcc", "accept_resume"))
	{
		maki_instance_config_set_boolean(inst, "dcc", "accept_resume", FALSE);
	}

	if (!maki_instance_config_exists(inst, "dcc", "accept_send"))
	{
		maki_instance_config_set_boolean(inst, "dcc", "accept_send", FALSE);
	}

	if (!maki_instance_config_exists(inst, "dcc", "port_first"))
	{
		maki_instance_config_set_integer(inst, "dcc", "port_first", 1024);
	}

	if (!maki_instance_config_exists(inst, "dcc", "port_last"))
	{
		maki_instance_config_set_integer(inst, "dcc", "port_last", 65535);
	}

	if (!maki_instance_config_exists(inst, "directories", "downloads"))
	{
		gchar* value;

		value = g_build_filename(g_get_user_data_dir(), "sushi", "downloads", NULL);
		maki_instance_config_set_string(inst, "directories", "downloads", value);
		g_free(value);
	}

	if (!maki_instance_config_exists(inst, "directories", "logs"))
	{
		gchar* value;

		value = g_build_filename(g_get_user_data_dir(), "sushi", "logs", NULL);
		maki_instance_config_set_string(inst, "directories", "logs", value);
		g_free(value);
	}

	if (!maki_instance_config_exists(inst, "logging", "enabled"))
	{
		maki_instance_config_set_boolean(inst, "logging", "enabled", TRUE);
	}

	if (!maki_instance_config_exists(inst, "logging", "format"))
	{
		maki_instance_config_set_string(inst, "logging", "format", "$n/%Y-%m");
	}

	if (!maki_instance_config_exists(inst, "network", "stun"))
	{
		maki_instance_config_set_string(inst, "network", "stun", "");
	}

	if (!maki_instance_config_exists(inst, "reconnect", "retries"))
	{
		maki_instance_config_set_integer(inst, "reconnect", "retries", 3);
	}

	if (!maki_instance_config_exists(inst, "reconnect", "timeout"))
	{
		maki_instance_config_set_integer(inst, "reconnect", "timeout", 10);
	}
}

makiInstance* maki_instance_new (void)
{
	gchar* config_dir;
	gchar* config_file;
	gchar* servers_dir;
	makiInstance* inst;

	config_dir = g_build_filename(g_get_user_config_dir(), "sushi", NULL);
	servers_dir = g_build_filename(config_dir, "servers", NULL);

	if (g_mkdir_with_parents(config_dir, 0777) != 0
	    || g_mkdir_with_parents(servers_dir, 0777) != 0)
	{
		g_free(config_dir);
		g_free(servers_dir);
		return NULL;
	}

	inst = g_new(makiInstance, 1);

	inst->key_file = g_key_file_new();
	inst->directories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	inst->servers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, maki_server_unref);

	g_hash_table_insert(inst->directories, g_strdup("config"), config_dir);
	g_hash_table_insert(inst->directories, g_strdup("servers"), servers_dir);

	config_file = g_build_filename(config_dir, "maki", NULL);
	g_key_file_load_from_file(inst->key_file, config_file, G_KEY_FILE_NONE, NULL);
	g_free(config_file);

	maki_instance_config_set_defaults(inst);

	/* FIXME load after instance */
	inst->plugins[0] = maki_plugin_load("nm");

	inst->dcc.id = 0;
	inst->dcc.list = NULL;
	inst->dcc.mutex = g_mutex_new();

	inst->network = maki_network_new(inst);

	return inst;
}

gboolean maki_instance_config_get_boolean (makiInstance* inst, const gchar* group, const gchar* key)
{
	return g_key_file_get_boolean(inst->key_file, group, key, NULL);
}

void maki_instance_config_set_boolean (makiInstance* inst, const gchar* group, const gchar* key, gboolean value)
{
	gchar* path;

	g_key_file_set_boolean(inst->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(inst, "config"), "maki", NULL);
	i_key_file_to_file(inst->key_file, path, NULL, NULL);
	g_free(path);
}

gint maki_instance_config_get_integer (makiInstance* inst, const gchar* group, const gchar* key)
{
	return g_key_file_get_integer(inst->key_file, group, key, NULL);
}

void maki_instance_config_set_integer (makiInstance* inst, const gchar* group, const gchar* key, gint value)
{
	gchar* path;

	g_key_file_set_integer(inst->key_file, group, key, value);

	path = g_build_filename(maki_instance_directory(inst, "config"), "maki", NULL);
	i_key_file_to_file(inst->key_file, path, NULL, NULL);
	g_free(path);
}

gchar* maki_instance_config_get_string (makiInstance* inst, const gchar* group, const gchar* key)
{
	return g_key_file_get_string(inst->key_file, group, key, NULL);
}

void maki_instance_config_set_string (makiInstance* inst, const gchar* group, const gchar* key, const gchar* string)
{
	gchar* path;

	g_key_file_set_string(inst->key_file, group, key, string);

	path = g_build_filename(maki_instance_directory(inst, "config"), "maki", NULL);
	i_key_file_to_file(inst->key_file, path, NULL, NULL);
	g_free(path);
}

gboolean maki_instance_config_exists (makiInstance* inst, const gchar* group, const gchar* key)
{
	return g_key_file_has_key(inst->key_file, group, key, NULL);
}

const gchar* maki_instance_directory (makiInstance* inst, const gchar* directory)
{
	return g_hash_table_lookup(inst->directories, directory);
}

makiServer* maki_instance_add_server (makiInstance* inst, const gchar* name, makiServer* serv)
{
	g_hash_table_insert(inst->servers, g_strdup(name), serv);

	return serv;
}

makiServer* maki_instance_get_server (makiInstance* inst, const gchar* name)
{
	return g_hash_table_lookup(inst->servers, name);
}

gboolean maki_instance_remove_server (makiInstance* inst, const gchar* name)
{
	return g_hash_table_remove(inst->servers, name);
}

gboolean maki_instance_rename_server (makiInstance* inst, const gchar* old_name, const gchar* new_name)
{
	makiServer* serv;

	if ((serv = g_hash_table_lookup(inst->servers, new_name)) != NULL)
	{
		return FALSE;
	}

	if ((serv = g_hash_table_lookup(inst->servers, old_name)) == NULL)
	{
		return FALSE;
	}

	g_hash_table_insert(inst->servers, g_strdup(new_name), maki_server_ref(serv));
	g_hash_table_remove(inst->servers, old_name);

	return TRUE;
}

guint maki_instance_servers_count (makiInstance* inst)
{
	return g_hash_table_size(inst->servers);
}

void maki_instance_servers_iter (makiInstance* inst, GHashTableIter* iter)
{
	g_hash_table_iter_init(iter, inst->servers);
}

guint64 maki_instance_get_dcc_send_id (makiInstance* inst)
{
	guint64 id;

	g_mutex_lock(inst->dcc.mutex);

	inst->dcc.id++;
	id = inst->dcc.id;

	g_mutex_unlock(inst->dcc.mutex);

	return id;
}

void maki_instance_add_dcc_send (makiInstance* inst, makiDCCSend* dcc)
{
	g_mutex_lock(inst->dcc.mutex);

	inst->dcc.list = g_slist_prepend(inst->dcc.list, dcc);

	g_mutex_unlock(inst->dcc.mutex);
}

gboolean maki_instance_accept_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc = NULL;

	g_mutex_lock(inst->dcc.mutex);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		maki_dcc_send_accept(dcc);
	}

	g_mutex_unlock(inst->dcc.mutex);

	return (dcc != NULL);
}

gboolean maki_instance_resume_accept_dcc_send (makiInstance* inst, gchar* file_name, guint16 port, goffset position, guint32 token, gboolean is_incoming)
{
	gboolean ret = FALSE;
	GSList* list;

	g_mutex_lock(inst->dcc.mutex);

	for (list = inst->dcc.list; list != NULL; list = list->next)
	{
		makiDCCSend* dcc = list->data;

		if (maki_dcc_send_resume_accept(dcc, file_name, port, position, token, is_incoming))
		{
			ret = TRUE;
			break;
		}
	}

	g_mutex_unlock(inst->dcc.mutex);

	return ret;
}

gboolean maki_instance_resume_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc = NULL;

	g_mutex_lock(inst->dcc.mutex);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		maki_dcc_send_resume(dcc);
	}

	g_mutex_unlock(inst->dcc.mutex);

	return (dcc != NULL);
}

gboolean maki_instance_remove_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc = NULL;

	g_mutex_lock(inst->dcc.mutex);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		GTimeVal timeval;

		inst->dcc.list = g_slist_remove(inst->dcc.list, dcc);

		g_get_current_time(&timeval);
		maki_dbus_emit_dcc_send(timeval.tv_sec, maki_dcc_send_id(dcc), "", "", "", 0, 0, 0, 0);

		maki_dcc_send_free(dcc);
	}

	g_mutex_unlock(inst->dcc.mutex);

	return (dcc != NULL);
}

guint maki_instance_dcc_sends_count (makiInstance* inst)
{
	guint ret;

	g_mutex_lock(inst->dcc.mutex);

	ret = g_slist_length(inst->dcc.list);

	g_mutex_unlock(inst->dcc.mutex);

	return ret;
}

gchar* maki_instance_dcc_send_get (makiInstance* inst, guint64 id, const gchar* key)
{
	makiDCCSend* dcc = NULL;
	gchar* value = NULL;

	g_mutex_lock(inst->dcc.mutex);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		if (strcmp(key, "directory") == 0)
		{
			value = g_path_get_dirname(maki_dcc_send_path(dcc));
		}
		else if (strcmp(key, "path") == 0)
		{
			value = g_strdup(maki_dcc_send_path(dcc));
		}
	}

	g_mutex_unlock(inst->dcc.mutex);

	return value;
}

gboolean maki_instance_dcc_send_set (makiInstance* inst, guint64 id, const gchar* key, const gchar* value)
{
	makiDCCSend* dcc = NULL;
	gboolean ret = FALSE;

	g_mutex_lock(inst->dcc.mutex);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		if (strcmp(key, "directory") == 0)
		{
			const gchar* path;
			gchar* basename;
			gchar* new_path;

			path = maki_dcc_send_path(dcc);
			basename = g_path_get_basename(path);
			new_path = g_build_filename(value, basename, NULL);

			ret = maki_dcc_send_set_path(dcc, new_path);

			g_free(basename);
			g_free(new_path);
		}
		else if (strcmp(key, "path") == 0)
		{
			ret = maki_dcc_send_set_path(dcc, value);
		}
	}

	g_mutex_unlock(inst->dcc.mutex);

	return ret;
}

void maki_instance_dcc_sends_xxx (makiInstance* inst, GArray** ids, gchar*** servers, gchar*** froms, gchar*** filenames, GArray** sizes, GArray** progresses, GArray** speeds, GArray** statuses)
{
	guint i;
	GSList* list;

	g_mutex_lock(inst->dcc.mutex);

	for (list = inst->dcc.list, i = 0; list != NULL; list = list->next, i++)
	{
		guint64 id;
		guint64 size;
		guint64 progress;
		guint64 speed;
		guint64 status;
		makiDCCSend* dcc = list->data;

		id = maki_dcc_send_id(dcc);
		size = maki_dcc_send_size(dcc);
		progress = maki_dcc_send_progress(dcc);
		speed = maki_dcc_send_speed(dcc);
		status = maki_dcc_send_status(dcc);

		*ids = g_array_append_val(*ids, id);
		(*servers)[i] = g_strdup(maki_server_name(maki_dcc_send_server(dcc)));
		(*froms)[i] = g_strdup(maki_user_from(maki_dcc_send_user(dcc)));
		(*filenames)[i] = maki_dcc_send_filename(dcc);
		*sizes = g_array_append_val(*sizes, size);
		*progresses = g_array_append_val(*progresses, progress);
		*speeds = g_array_append_val(*speeds, speed);
		*statuses = g_array_append_val(*statuses, status);
	}

	g_mutex_unlock(inst->dcc.mutex);
}

makiNetwork* maki_instance_network (makiInstance* inst)
{
	return inst->network;
}

void maki_instance_free (makiInstance* inst)
{
	GSList* list;

	for (list = inst->dcc.list; list != NULL; list = list->next)
	{
		GTimeVal timeval;
		makiDCCSend* dcc = list->data;

		g_get_current_time(&timeval);

		maki_dbus_emit_dcc_send(timeval.tv_sec, maki_dcc_send_id(dcc), "", "", "", 0, 0, 0, 0);

		maki_dcc_send_free(dcc);
	}

	g_slist_free(inst->dcc.list);
	g_mutex_free(inst->dcc.mutex);

	if (inst->plugins[0] != NULL)
	{
		maki_plugin_unload(inst->plugins[0]);
	}

	maki_network_free(inst->network);

	g_key_file_free(inst->key_file);

	g_hash_table_destroy(inst->servers);
	g_hash_table_destroy(inst->directories);

	g_free(inst);
}
