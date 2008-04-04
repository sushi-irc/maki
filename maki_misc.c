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
#include "maki_cache.h"

/**
 * This function is a wrapper around sashimi_connect().
 * It handles the initial login with NICK and USER and emits the connect signal.
 */
gint maki_connect (struct maki_connection* m_conn)
{
	gint ret;

	if ((ret = sashimi_connect(m_conn->connection)) == 0)
	{
		gchar* buffer;
		GTimeVal time;

		m_conn->reconnect = TRUE;
		m_conn->retries = m_conn->maki->config.reconnect.retries;

		g_free(m_conn->nick);
		m_conn->nick = g_strdup(m_conn->initial_nick);

		buffer = g_strdup_printf("NICK %s", m_conn->initial_nick);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		buffer = g_strdup_printf("USER %s 0 * :%s", m_conn->initial_nick, m_conn->name);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		g_get_current_time(&time);
		maki_dbus_emit_connect(m_conn->maki->bus, time.tv_sec, m_conn->server);
	}

	return ret;
}

/**
 * This function is a wrapper around sashimi_disconnect().
 */
gint maki_disconnect (struct maki_connection* m_conn)
{
	gint ret;

	m_conn->connected = FALSE;
	ret = sashimi_disconnect(m_conn->connection);

	return ret;
}

/**
 * This function gets called by the quit method after a delay.
 */
gboolean maki_disconnect_timeout (gpointer data)
{
	struct maki_connection* m_conn = data;

	maki_disconnect(m_conn);

	return FALSE;
}

gpointer maki_user_new (gpointer key, gpointer data)
{
	gchar* nick = key;
	struct maki_connection* connection = data;
	struct maki_user* m_user;

	m_user = g_new(struct maki_user, 1);
	m_user->connection = connection;
	m_user->nick = nick;

	return m_user;
}

/**
 * This function gets called when a user is removed from the users hash table.
 */
void maki_user_free (gpointer value)
{
	struct maki_user* m_user = value;

	g_free(m_user);
}

struct maki_channel_user* maki_channel_user_new (struct maki_user* m_user)
{
	struct maki_channel_user* m_cuser;

	m_cuser = g_new(struct maki_channel_user, 1);
	m_cuser->user = m_user;
	m_cuser->prefix = 0;

	return m_cuser;
}

void maki_channel_user_free (gpointer data)
{
	struct maki_channel_user* m_cuser = data;

	maki_cache_remove(m_cuser->user->connection->users, m_cuser->user->nick);

	g_free(m_cuser);
}

struct maki_channel* maki_channel_new (const gchar* name)
{
	struct maki_channel* m_chan;

	m_chan = g_new(struct maki_channel, 1);

	m_chan->name = g_strdup(name);
	m_chan->autojoin = FALSE;
	m_chan->joined = FALSE;
	m_chan->key = NULL;
	m_chan->users = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_channel_user_free);

	return m_chan;
}

/**
 * This function gets called when a channel is removed from the channels hash table.
 */
void maki_channel_free (gpointer data)
{
	struct maki_channel* m_chan = data;

	g_hash_table_destroy(m_chan->users);
	g_free(m_chan->key);
	g_free(m_chan->name);
	g_free(m_chan);
}

struct maki_connection* maki_connection_new (struct maki* maki, const gchar* server, const gchar* address, gushort port, const gchar* nick, const gchar* name)
{
	struct maki_connection* m_conn;

	m_conn = g_new(struct maki_connection, 1);
	m_conn->maki = maki;
	m_conn->server = g_strdup(server);
	m_conn->initial_nick = g_strdup(nick);
	m_conn->nick = g_strdup(nick);
	m_conn->name = g_strdup(name);
	m_conn->autoconnect = FALSE;
	m_conn->connected = FALSE;
	m_conn->reconnect = FALSE;
	m_conn->retries = maki->config.reconnect.retries;
	m_conn->connection = sashimi_new(address, port, maki->message_queue, m_conn);
	m_conn->channels = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_channel_free);
	m_conn->users = maki_cache_new(maki_user_new, maki_user_free, m_conn);

	m_conn->nickserv.password = NULL;
	m_conn->commands = NULL;
	m_conn->ignores = NULL;
	m_conn->support.chanmodes = NULL;
	m_conn->support.prefix.modes = g_strdup("ov");
	m_conn->support.prefix.prefixes = g_strdup("@+");

	return m_conn;
}

/**
 * This function gets called when a connection is removed from the connections hash table.
 */
void maki_connection_free (gpointer data)
{
	struct maki_connection* m_conn = data;

	g_free(m_conn->support.prefix.prefixes);
	g_free(m_conn->support.prefix.modes);
	g_free(m_conn->support.chanmodes);
	g_strfreev(m_conn->ignores);
	g_strfreev(m_conn->commands);
	g_free(m_conn->nickserv.password);
	g_hash_table_destroy(m_conn->channels);
	maki_cache_free(m_conn->users);
	maki_disconnect(m_conn);
	sashimi_free(m_conn->connection);
	g_free(m_conn->name);
	g_free(m_conn->nick);
	g_free(m_conn->initial_nick);
	g_free(m_conn->server);
	g_free(m_conn);
}
