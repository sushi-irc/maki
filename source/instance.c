/*
 * Copyright (c) 2008-2012 Michael Kuhn
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
#include <glib/gstdio.h>
#include <gmodule.h>

#include <string.h>

#include <ilib.h>

#include "instance.h"

#include "dbus.h"
#include "plugin.h"

struct maki_instance
{
	makiNetwork* network;

	GKeyFile* key_file;

	GHashTable* servers;
	GHashTable* directories;
	GHashTable* plugins;

	struct
	{
		guint64 id;
		GSList* list;
	}
	dcc;

	GMainContext* main_context;
	GMainLoop* main_loop;
	GThread* thread;

	struct
	{
		GMutex* config;
		GMutex* dcc;
		GMutex* instance;
		GMutex* servers;
	}
	mutex;
};

static
void
maki_instance_load_plugins (makiInstance* inst)
{
	gchar** plugins;
	guint plugins_len;
	guint i;

	plugins = g_key_file_get_keys(inst->key_file, "plugins", NULL, NULL);

	if (plugins == NULL)
	{
		return;
	}

	plugins_len = g_strv_length(plugins);

	for (i = 0; i < plugins_len; i++)
	{
		GModule* module;

		if (!g_key_file_get_boolean(inst->key_file, "plugins", plugins[i], NULL))
		{
			continue;
		}

		if ((module = maki_plugin_load(plugins[i])) != NULL)
		{
			g_hash_table_insert(inst->plugins, g_strdup(plugins[i]), module);
		}
	}

	g_strfreev(plugins);
}

static
gpointer
maki_instance_thread (gpointer data)
{
	makiInstance* inst = data;

	g_main_context_push_thread_default(inst->main_context);

	maki_instance_load_plugins(inst);

	g_main_loop_run(inst->main_loop);

	return NULL;
}

static
makiDCCSend*
maki_instance_get_dcc_send (makiInstance* inst, guint64 id)
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

static
void
maki_instance_config_save (makiInstance* inst)
{
	gchar* path;

	path = g_build_filename(g_hash_table_lookup(inst->directories, "config"), "maki", NULL);
	i_key_file_to_file(inst->key_file, path, NULL, NULL);
	g_free(path);
}

static
void
maki_instance_config_set_defaults (makiInstance* inst)
{
	if (!g_key_file_has_key(inst->key_file, "dcc", "accept_chat", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "dcc", "accept_chat", FALSE);
	}

	if (!g_key_file_has_key(inst->key_file, "dcc", "accept_resume", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "dcc", "accept_resume", FALSE);
	}

	if (!g_key_file_has_key(inst->key_file, "dcc", "accept_send", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "dcc", "accept_send", FALSE);
	}

	if (!g_key_file_has_key(inst->key_file, "dcc", "port_first", NULL))
	{
		g_key_file_set_integer(inst->key_file, "dcc", "port_first", 1024);
	}

	if (!g_key_file_has_key(inst->key_file, "dcc", "port_last", NULL))
	{
		g_key_file_set_integer(inst->key_file, "dcc", "port_last", 65535);
	}

	if (!g_key_file_has_key(inst->key_file, "directories", "downloads", NULL))
	{
		gchar* value;

		value = g_build_filename(g_get_user_data_dir(), "sushi", "downloads", NULL);
		g_key_file_set_string(inst->key_file, "directories", "downloads", value);
		g_free(value);
	}

	if (!g_key_file_has_key(inst->key_file, "directories", "logs", NULL))
	{
		gchar* value;

		value = g_build_filename(g_get_user_data_dir(), "sushi", "logs", NULL);
		g_key_file_set_string(inst->key_file, "directories", "logs", value);
		g_free(value);
	}

	if (!g_key_file_has_key(inst->key_file, "logging", "enabled", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "logging", "enabled", TRUE);
	}

	if (!g_key_file_has_key(inst->key_file, "logging", "format", NULL))
	{
		g_key_file_set_string(inst->key_file, "logging", "format", "$n/%Y-%m");
	}

	if (!g_key_file_has_key(inst->key_file, "network", "stun", NULL))
	{
		g_key_file_set_string(inst->key_file, "network", "stun", "");
	}

	if (!g_key_file_has_key(inst->key_file, "reconnect", "retries", NULL))
	{
		g_key_file_set_integer(inst->key_file, "reconnect", "retries", 3);
	}

	if (!g_key_file_has_key(inst->key_file, "reconnect", "timeout", NULL))
	{
		g_key_file_set_integer(inst->key_file, "reconnect", "timeout", 10);
	}

	if (!g_key_file_has_key(inst->key_file, "plugins", "sleep", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "plugins", "sleep", TRUE);
	}

	if (!g_key_file_has_key(inst->key_file, "plugins", "upnp", NULL))
	{
		g_key_file_set_boolean(inst->key_file, "plugins", "upnp", TRUE);
	}

	maki_instance_config_save(inst);
}

makiInstance*
maki_instance_get_default (void)
{
	static makiInstance* inst = NULL;

	if (G_UNLIKELY(inst == NULL))
	{
		inst = maki_instance_new();
	}

	return inst;
}

makiInstance*
maki_instance_new (void)
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

	inst->main_context = g_main_context_new();
	inst->main_loop = g_main_loop_new(inst->main_context, FALSE);

	inst->key_file = g_key_file_new();

	inst->directories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	inst->servers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, maki_server_unref);
	inst->plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, maki_plugin_unload);

	g_hash_table_insert(inst->directories, g_strdup("config"), config_dir);
	g_hash_table_insert(inst->directories, g_strdup("servers"), servers_dir);

	config_file = g_build_filename(config_dir, "maki", NULL);
	g_key_file_load_from_file(inst->key_file, config_file, G_KEY_FILE_NONE, NULL);
	g_free(config_file);

	maki_instance_config_set_defaults(inst);

	inst->dcc.id = 0;
	inst->dcc.list = NULL;

	inst->mutex.config = g_mutex_new();
	inst->mutex.dcc = g_mutex_new();
	inst->mutex.instance = g_mutex_new();
	inst->mutex.servers = g_mutex_new();

	inst->network = maki_network_new(inst);

	inst->thread = g_thread_create(maki_instance_thread, inst, TRUE, NULL);

	return inst;
}

void
maki_instance_free (makiInstance* inst)
{
	GSList* list;

	g_hash_table_destroy(inst->plugins);
	g_hash_table_destroy(inst->servers);

	for (list = inst->dcc.list; list != NULL; list = list->next)
	{
		makiDCCSend* dcc = list->data;

		maki_dbus_emit_dcc_send(maki_dcc_send_id(dcc), "", "", "", 0, 0, 0, 0);
		maki_dcc_send_free(dcc);
	}

	g_slist_free(inst->dcc.list);

	maki_network_free(inst->network);

	g_main_loop_quit(inst->main_loop);
	g_thread_join(inst->thread);

	g_main_loop_unref(inst->main_loop);
	g_main_context_unref(inst->main_context);

	g_key_file_free(inst->key_file);

	g_hash_table_destroy(inst->directories);

	g_mutex_free(inst->mutex.config);
	g_mutex_free(inst->mutex.dcc);
	g_mutex_free(inst->mutex.instance);
	g_mutex_free(inst->mutex.servers);

	g_free(inst);
}

gboolean
maki_instance_config_get_boolean (makiInstance* inst, gchar const* group, gchar const* key)
{
	gboolean ret;

	g_mutex_lock(inst->mutex.config);
	ret = g_key_file_get_boolean(inst->key_file, group, key, NULL);
	g_mutex_unlock(inst->mutex.config);

	return ret;
}

void
maki_instance_config_set_boolean (makiInstance* inst, gchar const* group, gchar const* key, gboolean value)
{
	g_mutex_lock(inst->mutex.config);
	g_key_file_set_boolean(inst->key_file, group, key, value);
	maki_instance_config_save(inst);
	g_mutex_unlock(inst->mutex.config);
}

gint
maki_instance_config_get_integer (makiInstance* inst, gchar const* group, gchar const* key)
{
	gint ret;

	g_mutex_lock(inst->mutex.config);
	ret = g_key_file_get_integer(inst->key_file, group, key, NULL);
	g_mutex_unlock(inst->mutex.config);

	return ret;
}

void
maki_instance_config_set_integer (makiInstance* inst, gchar const* group, gchar const* key, gint value)
{
	g_mutex_lock(inst->mutex.config);
	g_key_file_set_integer(inst->key_file, group, key, value);
	maki_instance_config_save(inst);
	g_mutex_unlock(inst->mutex.config);
}

gchar*
maki_instance_config_get_string (makiInstance* inst, gchar const* group, gchar const* key)
{
	gchar* ret;

	g_mutex_lock(inst->mutex.config);
	ret = g_key_file_get_string(inst->key_file, group, key, NULL);
	g_mutex_unlock(inst->mutex.config);

	return ret;
}

void
maki_instance_config_set_string (makiInstance* inst, gchar const* group, gchar const* key, gchar const* string)
{
	g_mutex_lock(inst->mutex.config);
	g_key_file_set_string(inst->key_file, group, key, string);
	maki_instance_config_save(inst);
	g_mutex_unlock(inst->mutex.config);
}

gchar**
maki_instance_config_get_keys (makiInstance* inst, gchar const* group)
{
	gchar** ret;

	g_mutex_lock(inst->mutex.config);
	ret = g_key_file_get_keys(inst->key_file, group, NULL, NULL);
	g_mutex_unlock(inst->mutex.config);

	return ret;
}

gboolean
maki_instance_config_exists (makiInstance* inst, gchar const* group, gchar const* key)
{
	gboolean ret;

	g_mutex_lock(inst->mutex.config);
	ret = g_key_file_has_key(inst->key_file, group, key, NULL);
	g_mutex_unlock(inst->mutex.config);

	return ret;
}

GMainContext*
maki_instance_main_context (makiInstance* inst)
{
	GMainContext* ret;

	g_mutex_lock(inst->mutex.instance);
	ret = inst->main_context;
	g_mutex_unlock(inst->mutex.instance);

	return ret;
}

makiNetwork*
maki_instance_network (makiInstance* inst)
{
	makiNetwork* ret;

	g_mutex_lock(inst->mutex.instance);
	ret = inst->network;
	g_mutex_unlock(inst->mutex.instance);

	return ret;
}

gchar const*
maki_instance_directory (makiInstance* inst, gchar const* directory)
{
	gchar const* ret;

	g_mutex_lock(inst->mutex.instance);
	ret = g_hash_table_lookup(inst->directories, directory);
	g_mutex_unlock(inst->mutex.instance);

	return ret;
}

void
maki_instance_add_server (makiInstance* inst, gchar const* name, makiServer* serv)
{
	g_mutex_lock(inst->mutex.servers);
	g_hash_table_insert(inst->servers, g_strdup(name), serv);
	g_mutex_unlock(inst->mutex.servers);
}

makiServer*
maki_instance_get_server (makiInstance* inst, gchar const* name)
{
	makiServer* ret;

	g_mutex_lock(inst->mutex.servers);
	ret = g_hash_table_lookup(inst->servers, name);
	g_mutex_unlock(inst->mutex.servers);

	return ret;
}

gboolean
maki_instance_remove_server (makiInstance* inst, gchar const* name)
{
	gboolean ret;

	g_mutex_lock(inst->mutex.servers);
	ret = g_hash_table_remove(inst->servers, name);
	g_mutex_unlock(inst->mutex.servers);

	return ret;
}

gboolean
maki_instance_rename_server (makiInstance* inst, gchar const* old_name, gchar const* new_name)
{
	makiServer* serv;
	gboolean ret = TRUE;

	g_mutex_lock(inst->mutex.servers);

	if (g_hash_table_lookup(inst->servers, new_name) != NULL)
	{
		ret = FALSE;
		goto end;
	}

	if ((serv = g_hash_table_lookup(inst->servers, old_name)) == NULL)
	{
		ret = FALSE;
		goto end;
	}

	g_hash_table_insert(inst->servers, g_strdup(new_name), maki_server_ref(serv));
	g_hash_table_remove(inst->servers, old_name);

end:
	g_mutex_unlock(inst->mutex.servers);

	return ret;
}

guint
maki_instance_servers_count (makiInstance* inst)
{
	guint ret;

	g_mutex_lock(inst->mutex.servers);
	ret = g_hash_table_size(inst->servers);
	g_mutex_unlock(inst->mutex.servers);

	return ret;
}

void
maki_instance_servers_iter (makiInstance* inst, GHashTableIter* iter)
{
	g_mutex_lock(inst->mutex.servers);
	g_hash_table_iter_init(iter, inst->servers);
	g_mutex_unlock(inst->mutex.servers);
}

guint64
maki_instance_get_dcc_send_id (makiInstance* inst)
{
	guint64 id;

	g_mutex_lock(inst->mutex.dcc);
	inst->dcc.id++;
	id = inst->dcc.id;
	g_mutex_unlock(inst->mutex.dcc);

	return id;
}

void
maki_instance_add_dcc_send (makiInstance* inst, makiDCCSend* dcc)
{
	g_mutex_lock(inst->mutex.dcc);
	inst->dcc.list = g_slist_prepend(inst->dcc.list, dcc);
	g_mutex_unlock(inst->mutex.dcc);
}

gboolean
maki_instance_accept_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc;

	g_mutex_lock(inst->mutex.dcc);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		maki_dcc_send_accept(dcc);
	}

	g_mutex_unlock(inst->mutex.dcc);

	return (dcc != NULL);
}

gboolean
maki_instance_resume_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc;

	g_mutex_lock(inst->mutex.dcc);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		maki_dcc_send_resume(dcc);
	}

	g_mutex_unlock(inst->mutex.dcc);

	return (dcc != NULL);
}

gboolean
maki_instance_resume_accept_dcc_send (makiInstance* inst, gchar* file_name, guint16 port, goffset position, guint32 token, gboolean is_incoming)
{
	gboolean ret = FALSE;
	GSList* list;

	g_mutex_lock(inst->mutex.dcc);

	for (list = inst->dcc.list; list != NULL; list = list->next)
	{
		makiDCCSend* dcc = list->data;

		if (maki_dcc_send_resume_accept(dcc, file_name, port, position, token, is_incoming))
		{
			ret = TRUE;
			break;
		}
	}

	g_mutex_unlock(inst->mutex.dcc);

	return ret;
}

gboolean
maki_instance_remove_dcc_send (makiInstance* inst, guint64 id)
{
	makiDCCSend* dcc;

	g_mutex_lock(inst->mutex.dcc);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		inst->dcc.list = g_slist_remove(inst->dcc.list, dcc);

		maki_dbus_emit_dcc_send(maki_dcc_send_id(dcc), "", "", "", 0, 0, 0, 0);
		maki_dcc_send_free(dcc);
	}

	g_mutex_unlock(inst->mutex.dcc);

	return (dcc != NULL);
}

guint
maki_instance_dcc_sends_count (makiInstance* inst)
{
	guint ret;

	g_mutex_lock(inst->mutex.dcc);
	ret = g_slist_length(inst->dcc.list);
	g_mutex_unlock(inst->mutex.dcc);

	return ret;
}

gchar*
maki_instance_dcc_send_get (makiInstance* inst, guint64 id, gchar const* key)
{
	makiDCCSend* dcc;
	gchar* value = NULL;

	g_mutex_lock(inst->mutex.dcc);

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

	g_mutex_unlock(inst->mutex.dcc);

	return value;
}

gboolean
maki_instance_dcc_send_set (makiInstance* inst, guint64 id, gchar const* key, gchar const* value)
{
	makiDCCSend* dcc;
	gboolean ret = FALSE;

	g_mutex_lock(inst->mutex.dcc);

	if ((dcc = maki_instance_get_dcc_send(inst, id)) != NULL)
	{
		if (strcmp(key, "directory") == 0)
		{
			gchar const* path;
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

	g_mutex_unlock(inst->mutex.dcc);

	return ret;
}

void
maki_instance_dcc_sends_xxx (makiInstance* inst, GArray** ids, gchar*** servers, gchar*** froms, gchar*** filenames, GArray** sizes, GArray** progresses, GArray** speeds, GArray** statuses)
{
	guint i;
	GSList* list;

	g_mutex_lock(inst->mutex.dcc);

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

	g_mutex_unlock(inst->mutex.dcc);
}

gboolean
maki_instance_plugin_method (makiInstance* inst, gchar const* plugin, gchar const* method, gpointer* symbol)
{
	GModule* module;
	gboolean ret;

	g_mutex_lock(inst->mutex.instance);

	if ((module = g_hash_table_lookup(inst->plugins, plugin)) == NULL)
	{
		ret = FALSE;
		goto end;
	}

	ret = g_module_symbol(module, method, symbol);

end:
	g_mutex_unlock(inst->mutex.instance);

	return ret;
}
