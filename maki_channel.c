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

struct maki_channel* maki_channel_new (const gchar* name)
{
	struct maki_channel* m_chan;

	m_chan = g_new(struct maki_channel, 1);

	m_chan->name = g_strdup(name);
	m_chan->autojoin = FALSE;
	m_chan->joined = FALSE;
	m_chan->key = NULL;
	m_chan->users = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_channel_user_free);
	m_chan->topic = NULL;

	return m_chan;
}

/**
 * This function gets called when a channel is removed from the channels hash table.
 */
void maki_channel_free (gpointer data)
{
	struct maki_channel* m_chan = data;

	g_hash_table_destroy(m_chan->users);

	g_free(m_chan->topic);
	g_free(m_chan->key);
	g_free(m_chan->name);

	g_free(m_chan);
}
