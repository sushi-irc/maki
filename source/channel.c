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

struct maki_channel
{
	makiServer* server;
	gchar* name;
	gboolean joined;
	GHashTable* users;
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
	chan->users = g_hash_table_new_full(i_ascii_str_case_hash, i_ascii_str_case_equal, g_free, maki_channel_user_free);
	chan->topic = NULL;

	maki_channel_set_defaults(chan);

	return chan;
}

/* This function gets called when a channel is removed from the channels hash table. */
void maki_channel_free (gpointer data)
{
	makiChannel* chan = data;

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

void maki_channel_add_user (makiChannel* chan, const gchar* name, makiChannelUser* cuser)
{
	g_hash_table_replace(chan->users, g_strdup(name), cuser);
}

makiChannelUser* maki_channel_get_user (makiChannel* chan, const gchar* name)
{
	return g_hash_table_lookup(chan->users, name);
}

void maki_channel_remove_user (makiChannel* chan, const gchar* name)
{
	g_hash_table_remove(chan->users, name);
}

void maki_channel_remove_users (makiChannel* chan)
{
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
