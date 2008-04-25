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

#include <string.h>

#include "maki.h"

void maki_out_join (struct maki* maki, struct maki_connection* m_conn, const gchar* channel, const gchar* key)
{
	gchar* buffer;

	if (key != NULL && key[0])
	{
		buffer = g_strdup_printf("JOIN %s %s", channel, key);
	}
	else
	{
		buffer = g_strdup_printf("JOIN %s", channel);
	}

	sashimi_send(m_conn->connection, buffer);
	g_free(buffer);
}

void maki_out_nick (struct maki* maki, struct maki_connection* m_conn, const gchar* nick)
{
	gchar* buffer;

	buffer = g_strdup_printf("NICK %s", nick);
	sashimi_send(m_conn->connection, buffer);
	g_free(buffer);
}

void maki_out_nickserv (struct maki* maki, struct maki_connection* m_conn)
{
	if (m_conn->nickserv.password != NULL)
	{
		gchar* buffer;

		if (strcmp(m_conn->user->nick, m_conn->initial_nick) != 0)
		{
			buffer = g_strdup_printf("PRIVMSG NickServ :GHOST %s %s", m_conn->initial_nick, m_conn->nickserv.password);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);

			maki_out_nick(maki, m_conn, m_conn->initial_nick);
		}

		buffer = g_strdup_printf("PRIVMSG NickServ :IDENTIFY %s", m_conn->nickserv.password);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}
}

void maki_out_privmsg (struct maki* maki, struct maki_connection* m_conn, const gchar* target, const gchar* message, gboolean queue)
{
	gchar* buffer;
	GTimeVal time;

	g_get_current_time(&time);

	buffer = g_strdup_printf("PRIVMSG %s :%s", target, message);

	if (queue)
	{
		sashimi_queue(m_conn->connection, buffer);
	}
	else
	{
		sashimi_send(m_conn->connection, buffer);
	}

	g_free(buffer);

	if (maki_is_channel(m_conn, target))
	{
		maki_dbus_emit_own_message(maki->bus, time.tv_sec, m_conn->server, target, message);
	}
	else
	{
		maki_dbus_emit_own_query(maki->bus, time.tv_sec, m_conn->server, target, message);
	}

	maki_log(m_conn, target, "<%s> %s", m_conn->user->nick, message);
}

void maki_out_privmsg_split (struct maki* maki, struct maki_connection* m_conn, const gchar* target, gchar* message, gboolean queue)
{
	gchar tmp = '\0';
	gint length = 512;

	/* :nickname!username@hostname PRIVMSG target :message\r\n */
	length -= 1; /* : */
	length -= strlen(m_conn->user->nick); /* nickname */
	length -= 1; /* ! */
	length -= 9; /* username */
	length -= 1; /* @ */
	length -= 63; /* hostname */
	length -= 1; /* " " */
	length -= 7; /* PRIVMSG */
	length -= 1; /* " " */
	length -= strlen(target); /* target */
	length -= 1; /* " " */
	length -= 1; /* : */
	length -= 2; /* \r\n */

	while (strlen(message) > length)
	{
		gint i = 0;
		gint skip;

		queue = TRUE;

		while (TRUE)
		{
			skip = g_utf8_skip[((guchar*)message)[i]];

			if (i + skip >= length)
			{
				break;
			}

			i += skip;
		}

		tmp = message[i];
		message[i] = '\0';

		maki_out_privmsg(maki, m_conn, target, message, queue);

		message[i] = tmp;
		message += i;
	}

	maki_out_privmsg(maki, m_conn, target, message, queue);
}

void maki_out_quit (struct maki* maki, struct maki_connection* m_conn, const gchar* message)
{
	gchar* buffer;
	GTimeVal time;

	g_get_current_time(&time);

	buffer = g_strdup_printf("QUIT :%s", message);
	sashimi_send(m_conn->connection, buffer);
	g_free(buffer);

	maki_dbus_emit_quit(maki->bus, time.tv_sec, m_conn->server, m_conn->user->nick, message);
}
