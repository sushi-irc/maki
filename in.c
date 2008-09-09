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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "maki.h"

/**
 * A convenience function to remove a colon before an argument.
 * It also checks for NULL.
 */
gchar* maki_remove_colon (gchar* string)
{
	if (string != NULL && string[0] == ':')
	{
		++string;
	}

	return string;
}

gboolean maki_mode_has_parameter (struct maki_connection* conn, gchar sign, gchar mode)
{
	gint type;
	gchar* chanmode;

	if (conn->support.chanmodes == NULL)
	{
		return FALSE;
	}

	if (strchr(conn->support.prefix.modes, mode) != NULL)
	{
		return TRUE;
	}

	for (chanmode = conn->support.chanmodes, type = 0; *chanmode != '\0'; ++chanmode)
	{
		if (*chanmode == ',')
		{
			++type;
			continue;
		}

		if (*chanmode == mode)
		{
			if (type == 0 || type == 1)
			{
				return TRUE;
			}
			else if (type == 2)
			{
				return (sign == '+');
			}
			else if (type == 3)
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

gboolean maki_is_channel(struct maki_connection* conn, const gchar* target)
{
	return (strchr(conn->support.chantypes, target[0]) != NULL);
}

gint maki_prefix_position (struct maki_connection* conn, gboolean is_prefix, gchar prefix)
{
	guint pos = 0;
	gchar* str = (is_prefix) ? conn->support.prefix.prefixes : conn->support.prefix.modes;

	while (str[pos] != '\0')
	{
		if (str[pos] == prefix)
		{
			return pos;
		}

		pos++;
	}

	return -1;
}

/**
 * This function gets called after a successful login.
 * It joins all configured channels.
 */
gboolean maki_join (gpointer data)
{
	GList* list;
	GList* tmp;
	struct maki_connection* conn = data;

	list = g_hash_table_get_values(conn->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;

		if (chan->autojoin || chan->joined)
		{
			maki_out_join(conn, chan->name, chan->key);
		}
	}

	g_list_free(list);

	return FALSE;
}

void maki_commands (struct maki_connection* conn)
{
	if (conn->commands != NULL)
	{
		gchar** command;

		command = conn->commands;

		while (*command != NULL)
		{
			sashimi_send(conn->connection, *command);
			++command;
		}
	}
}

void maki_in_privmsg (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* target;
	gchar* message;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);
	target = tmp[0];
	message = maki_remove_colon(tmp[1]);

	if (target != NULL && message != NULL)
	{
		if (message[0] == '\001')
		{
			++message;

			if (message[0] && message[strlen(message) - 1] == '\001')
			{
				message[strlen(message) - 1] = '\0';
			}

			if (strncmp(message, "ACTION", 6) == 0 && strlen(message) > 6)
			{
				if (maki_is_channel(conn, target))
				{
					maki_log(conn, target, "%s %s", nick, message + 7);
				}
				else
				{
					maki_log(conn, nick, "%s %s", nick, message + 7);
				}

				maki_dbus_emit_action(time, conn->server, nick, target, message + 7);
			}
			else
			{
				if (g_ascii_strcasecmp(target, conn->user->nick) == 0)
				{
					if (strncmp(message, "VERSION", 7) == 0)
					{
						maki_send_printf(conn, "NOTICE %s :\001VERSION %s %s\001", nick, SUSHI_NAME, SUSHI_VERSION);
					}
					else if (strncmp(message, "PING", 4) == 0)
					{
						maki_send_printf(conn, "NOTICE %s :\001%s\001", nick, message);
					}
				}

				if (maki_is_channel(conn, target))
				{
					maki_log(conn, target, "=%s= %s", nick, message);
					maki_dbus_emit_ctcp(time, conn->server, nick, target, message);
				}
				else
				{
					maki_log(conn, nick, "=%s= %s", nick, message);
					maki_dbus_emit_query_ctcp(time, conn->server, nick, message);
				}
			}
		}
		else
		{
			if (maki_is_channel(conn, target))
			{
				maki_log(conn, target, "<%s> %s", nick, message);
				maki_dbus_emit_message(time, conn->server, nick, target, message);
			}
			else
			{
				maki_log(conn, nick, "<%s> %s", nick, message);
				maki_dbus_emit_query(time, conn->server, nick, message);
			}
		}
	}

	g_strfreev(tmp);
}

void maki_in_join (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gchar* channel;
	struct maki_channel* chan;

	if (!remaining)
	{
		return;
	}

	channel = (remaining[0] == ':') ? remaining + 1 : remaining;
	g_strdelimit(channel, " ", '\0');

	if ((chan = g_hash_table_lookup(conn->channels, channel)) != NULL)
	{
		struct maki_channel_user* cuser;
		struct maki_user* user;

		user = maki_cache_insert(conn->users, nick);
		cuser = maki_channel_user_new(user);
		g_hash_table_replace(chan->users, cuser->user->nick, cuser);
	}

	if (g_ascii_strcasecmp(nick, conn->user->nick) == 0)
	{
		if (chan != NULL)
		{
			chan->joined = TRUE;
		}
		else
		{
			chan = maki_channel_new(channel);
			chan->joined = TRUE;

			g_hash_table_replace(conn->channels, chan->name, chan);
		}

		maki_log(conn, channel, _("» You join."));
	}
	else
	{
		maki_log(conn, channel, _("» %s joins."), nick);
	}

	maki_dbus_emit_join(time, conn->server, nick, channel);
}

void maki_in_part (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gchar* message;
	struct maki_channel* chan;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) < 1)
	{
		g_strfreev(tmp);
		return;
	}

	channel = tmp[0];
	message = maki_remove_colon(tmp[1]);

	if ((chan = g_hash_table_lookup(conn->channels, channel)) != NULL)
	{
		g_hash_table_remove(chan->users, nick);
	}

	if (g_ascii_strcasecmp(nick, conn->user->nick) == 0)
	{
		if (chan != NULL)
		{
			chan->joined = FALSE;

			if (!chan->autojoin && chan->key == NULL)
			{
				g_hash_table_remove(conn->channels, channel);
			}
		}

		if (message != NULL)
		{
			maki_log(conn, channel, _("« You part (%s)."), message);
		}
		else
		{
			maki_log(conn, channel, _("« You part."));
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_log(conn, channel, _("« %s parts (%s)."), nick, message);
		}
		else
		{
			maki_log(conn, channel, _("« %s parts."), nick);
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_part(time, conn->server, nick, channel, message);
	}
	else
	{
		maki_dbus_emit_part(time, conn->server, nick, channel, "");
	}

	g_strfreev(tmp);
}

void maki_in_quit (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	GList* list;
	GList* tmp;

	list = g_hash_table_get_values(conn->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;

		if (!chan->joined)
		{
			continue;
		}

		if (g_hash_table_lookup(chan->users, nick) != NULL)
		{
			if (remaining)
			{
				maki_log(conn, chan->name, _("« %s quits (%s)."), nick, maki_remove_colon(remaining));
			}
			else
			{
				maki_log(conn, chan->name, _("« %s quits."), nick);
			}
		}

		g_hash_table_remove(chan->users, nick);
	}

	g_list_free(list);

	if (remaining)
	{
		maki_dbus_emit_quit(time, conn->server, nick, maki_remove_colon(remaining));
	}
	else
	{
		maki_dbus_emit_quit(time, conn->server, nick, "");
	}
}

void maki_in_kick (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gchar* who;
	gchar* message;
	struct maki_channel* chan;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 3);

	if (g_strv_length(tmp) < 2)
	{
		g_strfreev(tmp);
		return;
	}

	channel = tmp[0];
	who = tmp[1];
	message = maki_remove_colon(tmp[2]);

	if ((chan = g_hash_table_lookup(conn->channels, channel)) != NULL)
	{
		g_hash_table_remove(chan->users, who);
	}

	if (g_ascii_strcasecmp(who, conn->user->nick) == 0)
	{
		if (chan != NULL)
		{
			chan->joined = FALSE;

			if (!chan->autojoin && chan->key == NULL)
			{
				g_hash_table_remove(conn->channels, channel);
			}
		}

		if (message != NULL)
		{
			maki_log(conn, channel, _("« %s kicks you (%s)."), nick, message);
		}
		else
		{
			maki_log(conn, channel, _("« %s kicks you."), nick);
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_log(conn, channel, _("« %s kicks %s (%s)."), nick, who, message);
		}
		else
		{
			maki_log(conn, channel, _("« %s kicks %s."), nick, who);
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_kick(time, conn->server, nick, channel, who, message);
	}
	else
	{
		maki_dbus_emit_kick(time, conn->server, nick, channel, who, "");
	}

	g_strfreev(tmp);
}

void maki_in_nick (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gboolean own = FALSE;
	gchar* new_nick;
	GList* list;
	GList* tmp;

	if (!remaining)
	{
		return;
	}

	new_nick = maki_remove_colon(remaining);

	if (g_ascii_strcasecmp(nick, conn->user->nick) == 0)
	{
		struct maki_user* user;

		user = maki_cache_insert(conn->users, new_nick);
		maki_user_copy(conn->user, user);
		maki_cache_remove(conn->users, conn->user->nick);
		conn->user = user;

		if (g_ascii_strcasecmp(conn->user->nick, conn->initial_nick) == 0)
		{
			maki_out_nickserv(conn);
		}

		own = TRUE;
	}

	list = g_hash_table_get_values(conn->channels);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_channel* chan = tmp->data;
		struct maki_channel_user* cuser;

		if (!chan->joined)
		{
			continue;
		}

		if ((cuser = g_hash_table_lookup(chan->users, nick)) != NULL)
		{
			struct maki_channel_user* tmp;
			struct maki_user* user;

			user = maki_cache_insert(conn->users, new_nick);
			maki_user_copy(cuser->user, user);

			tmp = maki_channel_user_new(user);
			maki_channel_user_copy(cuser, tmp);

			g_hash_table_remove(chan->users, nick);
			g_hash_table_replace(chan->users, tmp->user->nick, tmp);

			if (own)
			{
				maki_log(conn, chan->name, _("• You are now known as %s."), new_nick);
			}
			else
			{
				maki_log(conn, chan->name, _("• %s is now known as %s."), nick, new_nick);
			}
		}
	}

	g_list_free(list);

	maki_dbus_emit_nick(time, conn->server, nick, new_nick);
}

void maki_in_notice (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* target;
	gchar* message;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);
	target = tmp[0];
	message = maki_remove_colon(tmp[1]);

	if (target != NULL && message != NULL)
	{
		if (maki_is_channel(conn, target))
		{
			maki_log(conn, target, "-%s- %s", nick, message);
			maki_dbus_emit_notice(time, conn->server, nick, target, message);
		}
		else
		{
			maki_log(conn, nick, "-%s- %s", nick, message);
			maki_dbus_emit_query_notice(time, conn->server, nick, message);
		}
	}

	g_strfreev(tmp);
}

void maki_in_mode (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gboolean own;
	gint offset = 0;
	gchar** tmp;
	gchar* target;

	if (!remaining)
	{
		return;
	}

	own = (g_ascii_strcasecmp(nick, conn->user->nick) == 0);

	if (is_numeric)
	{
		offset = 1;
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2 + offset);
	target = tmp[offset];

	if (tmp[0] != NULL && target != NULL && tmp[1 + offset] != NULL)
	{
		gchar** modes;

		modes = g_strsplit(maki_remove_colon(tmp[1 + offset]), " ", 0);

		if (modes[0] != NULL)
		{
			gint i;
			guint length;
			gchar sign = '+';
			gchar buffer[3];
			gchar* mode;

			length = g_strv_length(modes);

			for (mode = modes[0], i = 1; *mode != '\0'; ++mode)
			{
				if (*mode == '+' || *mode == '-')
				{
					sign = *mode;
					continue;
				}

				buffer[0] = sign;
				buffer[1] = *mode;
				buffer[2] = '\0';

				if (maki_mode_has_parameter(conn, sign, *mode) && i < length)
				{
					gint pos;

					if ((pos = maki_prefix_position(conn, FALSE, *mode)) >= 0)
					{
						struct maki_channel* chan;
						struct maki_channel_user* cuser;

						if ((chan = g_hash_table_lookup(conn->channels, target)) != NULL
								&& (cuser = g_hash_table_lookup(chan->users, modes[i])) != NULL)
						{
							if (sign == '+')
							{
								cuser->prefix |= (1 << pos);
							}
							else
							{
								cuser->prefix &= ~(1 << pos);
							}
						}
					}

					if (is_numeric)
					{
						maki_log(conn, target, _("• Mode: %s %s"), buffer, modes[i]);
					}
					else
					{
						if (own)
						{
							maki_log(conn, target, _("• You set mode: %s %s"), buffer, modes[i]);
						}
						else
						{
							maki_log(conn, target, _("• %s sets mode: %s %s"), nick, buffer, modes[i]);
						}
					}

					maki_dbus_emit_mode(time, conn->server, nick, target, buffer, modes[i]);
					++i;
				}
				else
				{
					if (is_numeric)
					{
						maki_log(conn, target, _("• Mode: %s"), buffer);
					}
					else
					{
						if (own)
						{
							maki_log(conn, target, _("• You set mode: %s"), buffer);
						}
						else
						{
							maki_log(conn, target, _("• %s sets mode: %s"), nick, buffer);
						}
					}

					maki_dbus_emit_mode(time, conn->server, nick, target, buffer, "");
				}
			}
		}

		g_strfreev(modes);
	}

	g_strfreev(tmp);
}

void maki_in_invite (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gint offset = 0;
	gchar** tmp;
	gchar* channel;
	gchar* who;

	if (!remaining)
	{
		return;
	}

	if (is_numeric)
	{
		offset = 1;
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2 + offset);

	if (g_strv_length(tmp) < 2 + offset)
	{
		g_strfreev(tmp);
		return;
	}

	who = tmp[offset];
	channel = maki_remove_colon(tmp[1 + offset]);

	if (is_numeric)
	{
		maki_log(conn, channel, _("• You successfully invite %s."), who);
	}
	else
	{
		maki_log(conn, channel, _("• %s invites %s."), nick, who);
	}

	maki_dbus_emit_invite(time, conn->server, nick, channel, who);

	g_strfreev(tmp);
}

void maki_in_topic (struct maki_connection* conn, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gint offset = 0;
	gchar** tmp;
	gchar* channel;
	gchar* topic;
	struct maki_channel* chan;

	if (!remaining)
	{
		return;
	}

	if (is_numeric)
	{
		offset = 1;
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2 + offset);

	if (g_strv_length(tmp) < 2 + offset)
	{
		g_strfreev(tmp);
		return;
	}

	channel = tmp[offset];
	topic = maki_remove_colon(tmp[1 + offset]);

	if ((chan = g_hash_table_lookup(conn->channels, channel)) != NULL)
	{
		g_free(chan->topic);
		chan->topic = g_strdup(topic);
	}

	if (is_numeric)
	{
		maki_log(conn, channel, _("• Topic: %s"), topic);
	}
	else
	{
		if (g_ascii_strcasecmp(nick, conn->user->nick) == 0)
		{
			maki_log(conn, channel, _("• You change the topic: %s"), topic);
		}
		else
		{
			maki_log(conn, channel, _("• %s changes the topic: %s"), nick, topic);
		}
	}

	maki_dbus_emit_topic(time, conn->server, nick, channel, topic);

	g_strfreev(tmp);
}

void maki_in_rpl_namreply (struct maki_connection* conn, glong time, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gint i;
	guint length;
	struct maki_channel* chan;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 0);
	channel = tmp[2];
	length = g_strv_length(tmp);

	if (length > 3 && (chan = g_hash_table_lookup(conn->channels, channel)) != NULL)
	{
		for (i = 3; i < length; ++i)
		{
			gchar* nick = maki_remove_colon(tmp[i]);
			guint prefix = 0;
			gint pos;
			struct maki_channel_user* cuser;
			struct maki_user* user;

			if ((pos = maki_prefix_position(conn, TRUE, *nick)) >= 0)
			{
				prefix |= (1 << pos);
				++nick;
			}

			user = maki_cache_insert(conn->users, nick);
			cuser = maki_channel_user_new(user);
			g_hash_table_replace(chan->users, cuser->user->nick, cuser);
			cuser->prefix = prefix;
		}
	}

	g_strfreev(tmp);
}

void maki_in_rpl_away (struct maki_connection* conn, glong time, gchar* remaining)
{
	gchar** tmp;
	gchar* nick;
	gchar* message;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 3);

	if (g_strv_length(tmp) < 3)
	{
		g_strfreev(tmp);
		return;
	}

	nick = tmp[1];
	message = maki_remove_colon(tmp[2]);

	maki_dbus_emit_away_message(time, conn->server, nick, message);

	g_strfreev(tmp);
}

void maki_in_rpl_isupport (struct maki_connection* conn, glong time, gchar* remaining)
{
	gint i;
	guint length;
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 0);
	length = g_strv_length(tmp);

	for (i = 1; i < length; ++i)
	{
		gchar** support;

		if (tmp[i][0] == ':')
		{
			break;
		}

		support = g_strsplit(tmp[i], "=", 2);

		if (support[0] != NULL && support[1] != NULL)
		{
			if (strncmp(support[0], "CHANMODES", 9) == 0)
			{
				g_free(conn->support.chanmodes);
				conn->support.chanmodes = g_strdup(support[1]);
			}
			else if (strncmp(support[0], "CHANTYPES", 9) == 0)
			{
				g_free(conn->support.chantypes);
				conn->support.chantypes = g_strdup(support[1]);
			}
			else if (strncmp(support[0], "PREFIX", 6) == 0)
			{
				gchar* paren;

				paren = strchr(support[1], ')');

				if (support[1][0] == '(' && paren != NULL)
				{
					*paren = '\0';
					g_free(conn->support.prefix.modes);
					g_free(conn->support.prefix.prefixes);
					conn->support.prefix.modes = g_strdup(support[1] + 1);
					conn->support.prefix.prefixes = g_strdup(paren + 1);
				}
			}
		}

		g_strfreev(support);
	}

	g_strfreev(tmp);
}

void maki_in_rpl_motd (struct maki_connection* conn, glong time, gchar* remaining)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (tmp[0] != NULL && tmp[1] != NULL)
	{
		maki_dbus_emit_motd(time, conn->server, maki_remove_colon(tmp[1]));
	}

	g_strfreev(tmp);
}

void maki_in_rpl_list (struct maki_connection* conn, glong time, gchar* remaining, gboolean is_end)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_list(time, conn->server, "", -1, "");
		return;
	}

	tmp = g_strsplit(remaining, " ", 4);

	if (g_strv_length(tmp) == 4)
	{
		maki_dbus_emit_list(time, conn->server, tmp[1], atol(tmp[2]), maki_remove_colon(tmp[3]));
	}

	g_strfreev(tmp);
}

void maki_in_rpl_banlist (struct maki_connection* conn, glong time, gchar* remaining, gboolean is_end)
{
	gchar** tmp;
	guint length;

	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		tmp = g_strsplit(remaining, " ", 3);

		if (g_strv_length(tmp) >= 2)
		{
			maki_dbus_emit_banlist(time, conn->server, tmp[1], "", "", -1);
		}

		g_strfreev(tmp);

		return;
	}

	tmp = g_strsplit(remaining, " ", 5);
	length = g_strv_length(tmp);

	if (length == 5)
	{
		maki_dbus_emit_banlist(time, conn->server, tmp[1], tmp[2], tmp[3], atol(tmp[4]));
	}
	else if (length == 3)
	{
		/* This is what the RFC specifies. */
		maki_dbus_emit_banlist(time, conn->server, tmp[1], tmp[2], "", 0);
	}

	g_strfreev(tmp);
}

void maki_in_rpl_whois (struct maki_connection* conn, glong time, gchar* remaining, gboolean is_end)
{
	gchar** tmp;
	guint length;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 3);
	length = g_strv_length(tmp);

	if (length < 3)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_whois(time, conn->server, tmp[1], "");

		g_strfreev(tmp);

		return;
	}

	maki_dbus_emit_whois(time, conn->server, tmp[1], tmp[2]);

	g_strfreev(tmp);
}

void maki_in_err_nosuchnick (struct maki_connection* conn, glong time, gchar* remaining)
{
	gchar** tmp;
	guint length;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 3);
	length = g_strv_length(tmp);

	if (length < 2)
	{
		g_strfreev(tmp);
		return;
	}

	maki_dbus_emit_invalid_target(time, conn->server, tmp[1]);

	g_strfreev(tmp);
}

/**
 * This function is run in its own thread.
 * It receives and handles all messages from sashimi.
 */
gpointer maki_in_runner (gpointer data)
{
	struct maki* m = data;

	for (;;)
	{
		gchar* message;
		GTimeVal time;
		struct maki_connection* conn;
		struct sashimi_message* s_msg;

		s_msg = g_async_queue_pop(m->message_queue);

		if (G_UNLIKELY(s_msg->message == NULL && s_msg->data == NULL))
		{
			sashimi_message_free(s_msg);
			g_thread_exit(NULL);
		}

		conn = s_msg->data;
		message = s_msg->message;

		/* Check for valid UTF-8, because strange crashes can occur otherwise. */
		if (!g_utf8_validate(message, -1, NULL))
		{
			gchar* tmp;

			/* If the message is not in UTF-8 we will just assume that it is in ISO-8859-1. */
			if ((tmp = g_convert_with_fallback(message, -1, "UTF-8", "ISO-8859-1", "?", NULL, NULL, NULL)) != NULL)
			{
				g_free(message);
				s_msg->message = message = tmp;
			}
			else
			{
				sashimi_message_free(s_msg);
				continue;
			}
		}

		g_get_current_time(&time);

		maki_debug("IN: [%ld] %s %s\n", time.tv_sec, conn->server, message);

		if (G_LIKELY(message[0] == ':'))
		{
			gchar** parts;
			gchar** from;
			gchar* from_nick;
			gchar* type;
			gchar* remaining;

			parts = g_strsplit(message, " ", 3);
			from = g_strsplit_set(maki_remove_colon(parts[0]), "!@", 3);
			from_nick = from[0];
			type = parts[1];
			remaining = parts[2];

			if (conn->ignores != NULL)
			{
				gboolean match = FALSE;
				gint i;
				guint length;

				length = g_strv_length(conn->ignores);

				for (i = 0; i < length; ++i)
				{
					if (g_pattern_match_simple(conn->ignores[i], maki_remove_colon(parts[0])))
					{
						match = TRUE;
						break;
					}
				}

				if (match)
				{
					g_strfreev(from);
					g_strfreev(parts);
					sashimi_message_free(s_msg);
					continue;
				}
			}

			if (from && from_nick && type)
			{
				if (strncmp(type, "PRIVMSG", 7) == 0)
				{
					maki_in_privmsg(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "JOIN", 4) == 0)
				{
					maki_in_join(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "PART", 4) == 0)
				{
					maki_in_part(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "QUIT", 4) == 0)
				{
					maki_in_quit(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "KICK", 4) == 0)
				{
					maki_in_kick(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "NICK", 4) == 0)
				{
					maki_in_nick(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "NOTICE", 6) == 0)
				{
					maki_in_notice(conn, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "MODE", 4) == 0 || strncmp(type, IRC_RPL_CHANNELMODEIS, 3) == 0)
				{
					maki_in_mode(conn, time.tv_sec, from_nick, remaining, (type[0] != 'M'));
				}
				else if (strncmp(type, "INVITE", 6) == 0 || strncmp(type, IRC_RPL_INVITING, 3) == 0)
				{
					maki_in_invite(conn, time.tv_sec, from_nick, remaining, (type[0] != 'I'));
				}
				else if (strncmp(type, IRC_RPL_NAMREPLY, 3) == 0)
				{
					maki_in_rpl_namreply(conn, time.tv_sec, remaining);
				}
				else if (strncmp(type, IRC_RPL_UNAWAY, 3) == 0)
				{
					conn->user->away = FALSE;
					g_free(conn->user->away_message);
					conn->user->away_message = NULL;
					maki_dbus_emit_back(time.tv_sec, conn->server);
				}
				else if (strncmp(type, IRC_RPL_NOWAWAY, 3) == 0)
				{
					conn->user->away = TRUE;
					maki_dbus_emit_away(time.tv_sec, conn->server);
				}
				else if (strncmp(type, IRC_RPL_AWAY, 3) == 0)
				{
					maki_in_rpl_away(conn, time.tv_sec, remaining);
				}
				else if (strncmp(type, IRC_RPL_ENDOFMOTD, 3) == 0 || strncmp(type, IRC_ERR_NOMOTD, 3) == 0)
				{
					conn->connected = TRUE;
					maki_dbus_emit_connected(time.tv_sec, conn->server, conn->user->nick);
					maki_out_nickserv(conn);
					g_timeout_add_seconds(3, maki_join, conn);
					maki_commands(conn);

					if (conn->user->away && conn->user->away_message != NULL)
					{
						maki_out_away(conn, conn->user->away_message);
					}
				}
				else if (strncmp(type, IRC_ERR_NICKNAMEINUSE, 3) == 0)
				{
					if (!conn->connected)
					{
						gchar* nick;
						struct maki_user* user;

						nick = g_strconcat(conn->user->nick, "_", NULL);
						user = maki_cache_insert(conn->users, nick);
						maki_user_copy(conn->user, user);
						maki_cache_remove(conn->users, conn->user->nick);
						conn->user = user;
						g_free(nick);

						maki_out_nick(conn, conn->user->nick);
					}
				}
				else if (strncmp(type, IRC_ERR_NOSUCHNICK, 3) == 0)
				{
					maki_in_err_nosuchnick(conn, time.tv_sec, remaining);
				}
				else if (strncmp(type, IRC_RPL_MOTD, 3) == 0)
				{
					maki_in_rpl_motd(conn, time.tv_sec, remaining);
				}
				else if (strncmp(type, IRC_RPL_TOPIC, 3) == 0 || strncmp(type, "TOPIC", 5) == 0)
				{
					maki_in_topic(conn, time.tv_sec, from_nick, remaining, (type[0] != 'T'));
				}
				else if (strncmp(type, IRC_RPL_WHOISUSER, 3) == 0
				         || strncmp(type, IRC_RPL_WHOISSERVER, 3) == 0
				         || strncmp(type, IRC_RPL_WHOISOPERATOR, 3) == 0
				         || strncmp(type, IRC_RPL_WHOISIDLE, 3) == 0
				         || strncmp(type, IRC_RPL_ENDOFWHOIS, 3) == 0
				         || strncmp(type, IRC_RPL_WHOISCHANNELS, 3) == 0)
				{
					maki_in_rpl_whois(conn, time.tv_sec, remaining, (type[2] == '8'));
				}
				else if (strncmp(type, IRC_RPL_ISUPPORT, 3) == 0)
				{
					maki_in_rpl_isupport(conn, time.tv_sec, remaining);
				}
				else if (strncmp(type, IRC_RPL_LIST, 3) == 0 || strncmp(type, IRC_RPL_LISTEND, 3) == 0)
				{
					maki_in_rpl_list(conn, time.tv_sec, remaining, (type[2] == '3'));
				}
				else if (strncmp(type, IRC_RPL_BANLIST, 3) == 0 || strncmp(type, IRC_RPL_ENDOFBANLIST, 3) == 0)
				{
					maki_in_rpl_banlist(conn, time.tv_sec, remaining, (type[2] == '8'));
				}
				else if (strncmp(type, IRC_RPL_YOUREOPER, 3) == 0)
				{
					maki_dbus_emit_oper(time.tv_sec, conn->server);
				}
				else
				{
					maki_debug("WARN: Unhandled message type '%s'\n", type);
				}
			}

			g_strfreev(from);
			g_strfreev(parts);
		}

		sashimi_message_free(s_msg);
	}
}
