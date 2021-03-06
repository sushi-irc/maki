/*
 * Copyright (c) 2008-2012 Michael Kuhn
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
#include <glib/gi18n.h>

#include <string.h>

#include "out.h"

#include "dbus.h"
#include "misc.h"

void maki_out_away (makiServer* serv, const gchar* message)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(message != NULL);

	maki_server_send_printf(serv, "AWAY :%s", message);
}

void maki_out_join (makiServer* serv, const gchar* channel, const gchar* key)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(channel != NULL);

	if (key != NULL && key[0])
	{
		maki_server_send_printf(serv, "JOIN %s %s", channel, key);
	}
	else
	{
		maki_server_send_printf(serv, "JOIN %s", channel);
	}
}

void maki_out_nick (makiServer* serv, const gchar* nick)
{
	g_return_if_fail(serv != NULL);
	g_return_if_fail(nick != NULL);

	maki_server_send_printf(serv, "NICK %s", nick);
}

void maki_out_nickserv (makiServer* serv)
{
	gchar* initial_nick;
	gchar* nickserv_password;

	g_return_if_fail(serv != NULL);

	initial_nick = maki_server_config_get_string(serv, "server", "nick");
	nickserv_password = maki_server_config_get_string(serv, "server", "nickserv");

	if (!maki_config_is_empty(initial_nick) && !maki_config_is_empty(nickserv_password))
	{
		if (g_ascii_strcasecmp(maki_user_nick(maki_server_user(serv)), initial_nick) != 0)
		{
			if (maki_server_config_get_boolean(serv, "server", "nickserv_ghost"))
			{
				maki_server_send_printf(serv, "PRIVMSG NickServ :GHOST %s %s", initial_nick, nickserv_password);
			}

			maki_out_nick(serv, initial_nick);
		}

		maki_server_send_printf(serv, "PRIVMSG NickServ :IDENTIFY %s", nickserv_password);
	}

	g_free(initial_nick);
	g_free(nickserv_password);
}

static void maki_out_privmsg_internal (makiServer* serv, const gchar* target, const gchar* message, gboolean queue)
{
	gchar* buffer;

	g_return_if_fail(serv != NULL);
	g_return_if_fail(target != NULL);
	g_return_if_fail(message != NULL);

	buffer = g_strdup_printf("PRIVMSG %s :%s", target, message);
	maki_server_queue(serv, buffer, queue);
	g_free(buffer);

	maki_server_log(serv, target, "<%s> %s", maki_user_nick(maki_server_user(serv)), message);
	maki_dbus_emit_message(maki_server_name(serv), maki_user_from(maki_server_user(serv)), target, message);
}

void maki_out_privmsg (makiServer* serv, const gchar* target, const gchar* message, gboolean queue)
{
	gsize length = 512;

	g_return_if_fail(serv != NULL);
	g_return_if_fail(target != NULL);
	g_return_if_fail(message != NULL);

	/* :nickname!username@hostname PRIVMSG target :message\r\n */
	length -= 1; /* : */
	length -= strlen(maki_user_nick(maki_server_user(serv))); /* nickname */
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
		gchar* tmp;
		gsize i;
		gsize skip;

		i = 0;
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

		tmp = g_strndup(message, i);
		maki_out_privmsg_internal(serv, target, tmp, queue);
		message += i;
		g_free(tmp);
	}

	maki_out_privmsg_internal(serv, target, message, queue);
}
