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

struct maki_instance
{
	GKeyFile* key_file;

	GHashTable* servers;

	GHashTable* directories;
};

makiInstance* maki_instance_get_default (void)
{
	static makiInstance* inst = NULL;

	if (G_UNLIKELY(inst == NULL))
	{
		inst = maki_instance_new(opt_config);
	}

	return inst;
}

static void maki_instance_config_set_defaults (makiInstance* inst)
{
	if (!maki_instance_config_exists(inst, "dcc", "accept_chat"))
	{
		maki_instance_config_set_boolean(inst, "dcc", "accept_chat", FALSE);
	}

	if (!maki_instance_config_exists(inst, "dcc", "accept_send"))
	{
		maki_instance_config_set_boolean(inst, "dcc", "accept_send", FALSE);
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

makiInstance* maki_instance_new (const gchar* config)
{
	gchar* config_dir;
	gchar* config_file;
	gchar* servers_dir;
	makiInstance* inst;

	if (config != NULL)
	{
		config_dir = g_strdup(config);
	}
	else
	{
		config_dir = g_build_filename(g_get_user_config_dir(), "sushi", NULL);
	}

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
	inst->servers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_server_free);

	g_hash_table_insert(inst->directories, g_strdup("config"), config_dir);
	g_hash_table_insert(inst->directories, g_strdup("servers"), servers_dir);

	config_file = g_build_filename(config_dir, "maki", NULL);
	g_key_file_load_from_file(inst->key_file, config_file, G_KEY_FILE_NONE, NULL);
	g_free(config_file);

	maki_instance_config_set_defaults(inst);

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

/* FIXME abstract more */
GHashTable* maki_instance_servers (makiInstance* inst)
{
	return inst->servers;
}

void maki_instance_free (makiInstance* inst)
{
	g_key_file_free(inst->key_file);

	g_hash_table_destroy(inst->servers);
	g_hash_table_destroy(inst->directories);

	g_free(inst);
}
