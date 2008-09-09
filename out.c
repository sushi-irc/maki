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

void maki_out_away (struct maki_connection* conn, const gchar* message)
{
	maki_send_printf(conn, "AWAY :%s", message);
}

void maki_out_join (struct maki_connection* conn, const gchar* channel, const gchar* key)
{
	if (key != NULL && key[0])
	{
		maki_send_printf(conn, "JOIN %s %s", channel, key);
	}
	else
	{
		maki_send_printf(conn, "JOIN %s", channel);
	}
}

void maki_out_nick (struct maki_connection* conn, const gchar* nick)
{
	maki_send_printf(conn, "NICK %s", nick);
}

void maki_out_nickserv (struct maki_connection* conn)
{
	if (conn->nickserv.password != NULL)
	{
		if (g_ascii_strcasecmp(conn->user->nick, conn->initial_nick) != 0)
		{
			if (conn->nickserv.ghost)
			{
				maki_send_printf(conn, "PRIVMSG NickServ :GHOST %s %s", conn->initial_nick, conn->nickserv.password);
			}

			maki_out_nick(conn, conn->initial_nick);
		}

		maki_send_printf(conn, "PRIVMSG NickServ :IDENTIFY %s", conn->nickserv.password);
	}
}

static void maki_out_privmsg_internal (struct maki_connection* conn, const gchar* target, const gchar* message, gboolean queue)
{
	gchar* buffer;
	GTimeVal time;

	g_get_current_time(&time);

	buffer = g_strdup_printf("PRIVMSG %s :%s", target, message);

	if (queue)
	{
		sashimi_queue(conn->connection, buffer);
	}
	else
	{
		sashimi_send_or_queue(conn->connection, buffer);
	}

	g_free(buffer);

	maki_log(conn, target, "<%s> %s", conn->user->nick, message);
	maki_dbus_emit_own_message(time.tv_sec, conn->server, target, message);
}

void maki_out_privmsg (struct maki_connection* conn, const gchar* target, gchar* message, gboolean queue)
{
	gchar tmp = '\0';
	gint length = 512;

	/* :nickname!username@hostname PRIVMSG target :message\r\n */
	length -= 1; /* : */
	length -= strlen(conn->user->nick); /* nickname */
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

		maki_out_privmsg_internal(conn, target, message, queue);

		message[i] = tmp;
		message += i;
	}

	maki_out_privmsg_internal(conn, target, message, queue);
}

void maki_out_quit (struct maki_connection* conn, const gchar* message)
{
	GTimeVal time;

	GList* list;
	GList* tmp;

	g_get_current_time(&time);

	maki_send_printf(conn, "QUIT :%s", message);

	list = g_hash_table_get_values(conn->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;

		if (chan->joined)
		{
			maki_log(conn, chan->name, "« You quit (%s).", message);
		}
	}

	g_list_free(list);

	maki_dbus_emit_quit(time.tv_sec, conn->server, conn->user->nick, message);
}
