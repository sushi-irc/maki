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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "maki.h"

struct maki_config
{
	GHashTable* groups;
};

struct maki_config_group
{
	GHashTable* keys;
};

typedef struct maki_config_group makiConfigGroup;

static makiConfigGroup* maki_config_group_new (void)
{
	makiConfigGroup* grp;

	grp = g_new(makiConfigGroup, 1);
	grp->keys = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	return grp;
}

static void maki_config_group_free (gpointer data)
{
	makiConfigGroup* grp = data;

	g_hash_table_destroy(grp->keys);
	g_free(grp);
}

static void maki_config_set_from_key_file (GKeyFile* key_file, makiConfig* conf, const gchar* group, const gchar* key)
{
	gchar* value;
	GError* error = NULL;

	value = g_key_file_get_string(key_file, group, key, &error);

	if (error)
	{
		g_error_free(error);
	}
	else
	{
		makiConfigGroup* grp;

		if ((grp = g_hash_table_lookup(conf->groups, group)) != NULL)
		{
			g_hash_table_insert(grp->keys, g_strdup(key), value);
		}
	}
}

makiConfig* maki_config_new (struct maki* m)
{
	gchar* path;
	makiConfig* conf;
	GKeyFile* key_file;

	conf = g_new(makiConfig, 1);
	conf->groups = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, maki_config_group_free);

	/* Create groups and set default values. */
	{
		makiConfigGroup* group;

		group = maki_config_group_new();
		g_hash_table_insert(conf->groups, g_strdup("directories"), group);
		g_hash_table_insert(group->keys, g_strdup("logs"), g_build_filename(g_get_user_data_dir(), "sushi", "logs", NULL));

		group = maki_config_group_new();
		g_hash_table_insert(conf->groups, g_strdup("logging"), group);
		g_hash_table_insert(group->keys, g_strdup("enabled"), g_strdup("true"));

		group = maki_config_group_new();
		g_hash_table_insert(conf->groups, g_strdup("reconnect"), group);
		g_hash_table_insert(group->keys, g_strdup("retries"), g_strdup("3"));
		g_hash_table_insert(group->keys, g_strdup("timeout"), g_strdup("10"));
	}

	key_file = g_key_file_new();
	path = g_build_filename(m->directories.config, "maki", NULL);

	/* Read values from config file. */
	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		maki_config_set_from_key_file(key_file, conf, "directories", "logs");

		maki_config_set_from_key_file(key_file, conf, "logging", "enabled");

		maki_config_set_from_key_file(key_file, conf, "reconnect", "retries");
		maki_config_set_from_key_file(key_file, conf, "reconnect", "timeout");
	}

	g_key_file_free(key_file);
	g_free(path);

	{
		const gchar* logs;

		logs = maki_config_get(conf, "directories", "logs");

		if (!g_path_is_absolute(logs))
		{
			maki_config_set(conf, "directories", "logs", g_build_filename(g_get_home_dir(), logs, NULL));
		}

		g_mkdir_with_parents(maki_config_get(conf, "directories", "logs"), S_IRUSR | S_IWUSR | S_IXUSR);
	}

	return conf;
}

const gchar* maki_config_get (makiConfig* conf, const gchar* group, const gchar* key)
{
	makiConfigGroup* grp;

	if ((grp = g_hash_table_lookup(conf->groups, group)) != NULL)
	{
		return g_hash_table_lookup(grp->keys, key);
	}

	return NULL;
}

gint maki_config_get_int (makiConfig* conf, const gchar* group, const gchar* key)
{
	return atoi(maki_config_get(conf, group, key));
}

void maki_config_set (makiConfig* conf, const gchar* group, const gchar* key, const gchar* value)
{
	makiConfigGroup* grp;

	if ((grp = g_hash_table_lookup(conf->groups, group)) != NULL)
	{
		gchar* path;
		GKeyFile* key_file;
		struct maki* m = maki();

		g_hash_table_insert(grp->keys, g_strdup(key), g_strdup(value));

		key_file = g_key_file_new();
		path = g_build_filename(m->directories.config, "maki", NULL);

		g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL);
		g_key_file_set_string(key_file, group, key, value);
		maki_key_file_to_file(key_file, path);

		g_key_file_free(key_file);
		g_free(path);
	}
}

void maki_config_free (makiConfig* conf)
{
	g_hash_table_destroy(conf->groups);

	g_free(conf);
}
