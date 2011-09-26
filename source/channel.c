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

#include <ilib.h>

#include "channel.h"

#include "server.h"
#include "user.h"

struct maki_channel
{
	makiServer* server;
	gchar* name;
	gboolean joined;
	GHashTable* users;
	GHashTable* user_prefixes;
	gchar* topic;
};

static void maki_channel_set_defaults (makiChannel* chan)
{
	if (!maki_server_config_exists(chan->server, chan->name, "autojoin"))
	{
		maki_server_config_set_boolean(chan->server, chan->name, "autojoin", FALSE);
	}
}

makiChannel* maki_channel_new (makiServer* serv, const gchar* name)
{
	makiChannel* chan;

	chan = g_new(makiChannel, 1);

	chan->server = serv;
	chan->name = g_strdup(name);
	chan->joined = FALSE;
	chan->users = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, NULL);
	chan->user_prefixes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
	chan->topic = NULL;

	maki_channel_set_defaults(chan);

	return chan;
}

/* This function gets called when a channel is removed from the channels hash table. */
void maki_channel_free (gpointer data)
{
	makiChannel* chan = data;

	maki_channel_remove_users(chan);

	g_hash_table_destroy(chan->user_prefixes);
	g_hash_table_destroy(chan->users);

	g_free(chan->topic);
	g_free(chan->name);

	g_free(chan);
}

gboolean maki_channel_autojoin (makiChannel* chan)
{
	return maki_server_config_get_boolean(chan->server, chan->name, "autojoin");
}

void maki_channel_set_autojoin (makiChannel* chan, gboolean autojoin)
{
	maki_server_config_set_boolean(chan->server, chan->name, "autojoin", autojoin);
}

gboolean maki_channel_joined (makiChannel* chan)
{
	return chan->joined;
}

void maki_channel_set_joined (makiChannel* chan, gboolean joined)
{
	chan->joined = joined;
}

gchar* maki_channel_key (makiChannel* chan)
{
	return maki_server_config_get_string(chan->server, chan->name, "key");
}

void maki_channel_set_key (makiChannel* chan, const gchar* key)
{
	maki_server_config_set_string(chan->server, chan->name, "key", key);
}

const gchar* maki_channel_topic (makiChannel* chan)
{
	return chan->topic;
}

void maki_channel_set_topic (makiChannel* chan, const gchar* topic)
{
	g_free(chan->topic);
	chan->topic = g_strdup(topic);
}

makiUser* maki_channel_add_user (makiChannel* chan, const gchar* name)
{
	makiUser* user;

	user = maki_server_add_user(chan->server, name);
	g_hash_table_insert(chan->users, g_strdup(maki_user_nick(user)), user);
	g_hash_table_insert(chan->user_prefixes, user, GUINT_TO_POINTER(0));

	return user;
}

makiUser* maki_channel_get_user (makiChannel* chan, const gchar* name)
{
	return g_hash_table_lookup(chan->users, name);
}

makiUser* maki_channel_rename_user (makiChannel* chan, const gchar* old_nick, const gchar* new_nick)
{
	makiUser* user = NULL;

	if ((user = g_hash_table_lookup(chan->users, old_nick)) != NULL)
	{
		g_hash_table_insert(chan->users, g_strdup(new_nick), user);
		g_hash_table_remove(chan->users, old_nick);
	}

	return user;
}

void maki_channel_remove_user (makiChannel* chan, const gchar* name)
{
	makiUser* user;

	if ((user = g_hash_table_lookup(chan->users, name)) != NULL)
	{
		g_hash_table_remove(chan->user_prefixes, user);
		g_hash_table_remove(chan->users, name);
		maki_server_remove_user(chan->server, maki_user_nick(user));
	}
}

void maki_channel_remove_users (makiChannel* chan)
{
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init(&iter, chan->users);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiUser* user = value;

		maki_server_remove_user(chan->server, maki_user_nick(user));
	}

	g_hash_table_remove_all(chan->user_prefixes);
	g_hash_table_remove_all(chan->users);
}

guint maki_channel_users_count (makiChannel* chan)
{
	return g_hash_table_size(chan->users);
}

void maki_channel_users_iter (makiChannel* chan, GHashTableIter* iter)
{
	g_hash_table_iter_init(iter, chan->users);
}

gboolean
maki_channel_get_user_prefix (makiChannel* chan, makiUser* user, guint pos)
{
	gpointer v;
	guint prefix = 0;

	if (g_hash_table_lookup_extended(chan->user_prefixes, user, NULL, &v))
	{
		prefix = GPOINTER_TO_UINT(v);
	}

	return (prefix & (1 << pos));
}

void
maki_channel_set_user_prefix (makiChannel* chan, makiUser* user, guint pos, gboolean set)
{
	gpointer v;

	if (g_hash_table_lookup_extended(chan->user_prefixes, user, NULL, &v))
	{
		guint prefix;

		prefix = GPOINTER_TO_UINT(v);

		if (set)
		{
			prefix |= (1 << pos);
		}
		else
		{
			prefix &= ~(1 << pos);
		}

		g_hash_table_insert(chan->user_prefixes, user, GUINT_TO_POINTER(prefix));
	}
}

void
maki_channel_set_user_prefix_override (makiChannel* chan, makiUser* user, guint prefix)
{
	gpointer v;

	if (g_hash_table_lookup_extended(chan->user_prefixes, user, NULL, &v))
	{
		g_hash_table_insert(chan->user_prefixes, user, GUINT_TO_POINTER(prefix));
	}
}
