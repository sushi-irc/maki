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

#include <glib/gstdio.h>

struct maki_instance
{
	makiConfig* config;

	GHashTable* servers;

	GHashTable* directories;
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

makiInstance* maki_instance_new (void)
{
	gchar* config_dir;
	gchar* servers_dir;
	makiInstance* inst;

	inst = g_new(makiInstance, 1);

	config_dir = g_build_filename(g_get_user_config_dir(), "sushi", NULL);
	servers_dir = g_build_filename(g_get_user_config_dir(), "sushi", "servers", NULL);

	if (g_mkdir_with_parents(config_dir, S_IRUSR | S_IWUSR | S_IXUSR) != 0
	    || g_mkdir_with_parents(servers_dir, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		g_free(config_dir);
		g_free(servers_dir);
		g_free(inst);
		return NULL;
	}

	inst->directories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	inst->servers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_server_free);

	g_hash_table_insert(inst->directories, g_strdup("config"), config_dir);
	g_hash_table_insert(inst->directories, g_strdup("servers"), servers_dir);

	inst->config = maki_config_new(inst);

	return inst;
}

makiConfig* maki_instance_config (makiInstance* inst)
{
	return inst->config;
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
	g_hash_table_destroy(inst->servers);

	g_hash_table_destroy(inst->directories);

	maki_config_free(inst->config);

	g_free(inst);
}
