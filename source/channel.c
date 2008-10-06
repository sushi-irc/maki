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

struct maki_channel
{
	gboolean autojoin;
	gboolean joined;
	gchar* key;
	GHashTable* users;
	gchar* topic;
};

makiChannel* maki_channel_new (void)
{
	makiChannel* chan;

	chan = g_new(makiChannel, 1);

	chan->autojoin = FALSE;
	chan->joined = FALSE;
	chan->key = NULL;
	chan->users = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, maki_channel_user_free);
	chan->topic = NULL;

	return chan;
}

/* This function gets called when a channel is removed from the channels hash table. */
void maki_channel_free (gpointer data)
{
	makiChannel* chan = data;

	g_hash_table_destroy(chan->users);

	g_free(chan->topic);
	g_free(chan->key);

	g_free(chan);
}

gboolean maki_channel_autojoin (makiChannel* chan)
{
	return chan->autojoin;
}

void maki_channel_set_autojoin (makiChannel* chan, gboolean autojoin)
{
	chan->autojoin = autojoin;
}

gboolean maki_channel_joined (makiChannel* chan)
{
	return chan->joined;
}

void maki_channel_set_joined (makiChannel* chan, gboolean joined)
{
	chan->joined = joined;
}

const gchar* maki_channel_key (makiChannel* chan)
{
	return chan->key;
}

void maki_channel_set_key (makiChannel* chan, const gchar* key)
{
	g_free(chan->key);
	chan->key = g_strdup(key);
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

void maki_channel_add_user (makiChannel* chan, gchar* name, makiChannelUser* cuser)
{
	g_hash_table_replace(chan->users, name, cuser);
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
