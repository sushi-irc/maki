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

void maki_user_destroy (gpointer data)
{
	struct maki_user* m_user = data;

	g_free(m_user->nick);
	g_free(m_user);
}

void maki_channel_destroy (gpointer data)
{
	struct maki_channel* m_chan = data;

	g_hash_table_destroy(m_chan->users);
	g_free(m_chan->name);
	g_free(m_chan);
}

void maki_connection_destroy (gpointer data)
{
	struct maki_connection* m_conn = data;

	g_hash_table_destroy(m_conn->channels);
	sashimi_disconnect(m_conn->connection);
	sashimi_free(m_conn->connection);
	g_free(m_conn->nick);
	g_free(m_conn->server);
	g_free(m_conn);
}

gchar* maki_remove_colon (gchar* string)
{
	if (string != NULL && string[0] == ':')
	{
		++string;
	}

	return string;
}
