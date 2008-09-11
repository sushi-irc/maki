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

void maki_out_away (makiServer* serv, const gchar* message)
{
	maki_send_printf(serv, "AWAY :%s", message);
}

void maki_out_join (makiServer* serv, const gchar* channel, const gchar* key)
{
	if (key != NULL && key[0])
	{
		maki_send_printf(serv, "JOIN %s %s", channel, key);
	}
	else
	{
		maki_send_printf(serv, "JOIN %s", channel);
	}
}

void maki_out_nick (makiServer* serv, const gchar* nick)
{
	maki_send_printf(serv, "NICK %s", nick);
}

void maki_out_nickserv (makiServer* serv)
{
	if (serv->nickserv.password != NULL)
	{
		if (g_ascii_strcasecmp(serv->user->nick, serv->initial_nick) != 0)
		{
			if (serv->nickserv.ghost)
			{
				maki_send_printf(serv, "PRIVMSG NickServ :GHOST %s %s", serv->initial_nick, serv->nickserv.password);
			}

			maki_out_nick(serv, serv->initial_nick);
		}

		maki_send_printf(serv, "PRIVMSG NickServ :IDENTIFY %s", serv->nickserv.password);
	}
}

static void maki_out_privmsg_internal (makiServer* serv, const gchar* target, const gchar* message, gboolean queue)
{
	gchar* buffer;
	GTimeVal time;

	g_get_current_time(&time);

	buffer = g_strdup_printf("PRIVMSG %s :%s", target, message);

	if (queue)
	{
		sashimi_queue(serv->connection, buffer);
	}
	else
	{
		sashimi_send_or_queue(serv->connection, buffer);
	}

	g_free(buffer);

	maki_log(serv, target, "<%s> %s", serv->user->nick, message);
	maki_dbus_emit_own_message(time.tv_sec, serv->server, target, message);
}

void maki_out_privmsg (makiServer* serv, const gchar* target, gchar* message, gboolean queue)
{
	gchar tmp = '\0';
	gint length = 512;

	/* :nickname!username@hostname PRIVMSG target :message\r\n */
	length -= 1; /* : */
	length -= strlen(serv->user->nick); /* nickname */
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

		maki_out_privmsg_internal(serv, target, message, queue);

		message[i] = tmp;
		message += i;
	}

	maki_out_privmsg_internal(serv, target, message, queue);
}

void maki_out_quit (makiServer* serv, const gchar* message)
{
	GTimeVal time;

	GList* list;
	GList* tmp;

	g_get_current_time(&time);

	maki_send_printf(serv, "QUIT :%s", message);

	list = g_hash_table_get_values(serv->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		makiChannel* chan = tmp->data;

		if (chan->joined)
		{
			maki_log(serv, chan->name, _("« You quit (%s)."), message);
		}
	}

	g_list_free(list);

	maki_dbus_emit_quit(time.tv_sec, serv->server, serv->user->nick, message);
}
