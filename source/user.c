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

#include "user.h"

#include "instance.h"

struct maki_user
{
	makiServer* server;

	gchar* from;
	gchar* nick;
	gchar* user;
	gchar* host;
	gboolean away;
	gchar* away_message;

	guint ref_count;
};

static
void
maki_user_set_from (makiUser* user)
{
	g_free(user->from);

	if (user->user != NULL && user->host != NULL)
	{
		user->from = g_strdup_printf("%s!%s@%s", user->nick, user->user, user->host);
	}
	else
	{
		user->from = g_strdup(user->nick);
	}
}

makiUser*
maki_user_new (gchar const* nick)
{
	makiUser* user;

	user = g_new(makiUser, 1);
	user->from = NULL;
	user->nick = g_strdup(nick);
	user->user = NULL;
	user->host = NULL;
	user->away = FALSE;
	user->away_message = NULL;

	user->ref_count = 1;

	maki_user_set_from(user);

	return user;
}

makiUser*
maki_user_ref (makiUser* user)
{
	user->ref_count++;

	return user;
}

void
maki_user_unref (gpointer value)
{
	makiUser* user = value;

	user->ref_count--;

	if (user->ref_count == 0)
	{
		g_free(user->from);
		g_free(user->nick);
		g_free(user->user);
		g_free(user->host);
		g_free(user->away_message);
		g_free(user);
	}
}

guint
maki_user_ref_count (makiUser* user)
{
	return user->ref_count;
}

gchar const*
maki_user_from (makiUser* user)
{
	return user->from;
}

gchar const*
maki_user_nick (makiUser* user)
{
	return user->nick;
}

void
maki_user_set_nick (makiUser* user, gchar const* nick)
{
	g_free(user->nick);
	user->nick = g_strdup(nick);
	maki_user_set_from(user);
}

void
maki_user_set_user (makiUser* user, gchar const* usr)
{
	g_free(user->user);
	user->user = g_strdup(usr);
	maki_user_set_from(user);
}

void
maki_user_set_host (makiUser* user, gchar const* host)
{
	g_free(user->host);
	user->host = g_strdup(host);
	maki_user_set_from(user);
}

gboolean
maki_user_away (makiUser* user)
{
	return user->away;
}

void
maki_user_set_away (makiUser* user, gboolean away)
{
	user->away = away;
}

gchar const*
maki_user_away_message (makiUser* user)
{
	return user->away_message;
}

void
maki_user_set_away_message (makiUser* user, gchar const* away_message)
{
	g_free(user->away_message);
	user->away_message = g_strdup(away_message);
}
