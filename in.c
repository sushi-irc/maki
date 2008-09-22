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

/* A convenience function to remove a colon before an argument.
 * It also checks for NULL. */
static gchar* maki_remove_colon (gchar* string)
{
	if (string != NULL && string[0] == ':')
	{
		return (string + 1);
	}

	return string;
}

static gboolean maki_mode_has_parameter (makiServer* serv, gchar sign, gchar mode)
{
	gint type;
	gchar* chanmode;

	if (serv->support.chanmodes == NULL)
	{
		return FALSE;
	}

	if (strchr(serv->support.prefix.modes, mode) != NULL)
	{
		return TRUE;
	}

	for (chanmode = serv->support.chanmodes, type = 0; *chanmode != '\0'; ++chanmode)
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

static gboolean maki_is_channel(makiServer* serv, const gchar* target)
{
	return (strchr(serv->support.chantypes, target[0]) != NULL);
}

gint maki_prefix_position (makiServer* serv, gboolean is_prefix, gchar prefix)
{
	guint pos = 0;
	gchar* str = (is_prefix) ? serv->support.prefix.prefixes : serv->support.prefix.modes;

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

/* This function gets called after a successful login.
 * It joins all configured channels. */
static gboolean maki_join (gpointer data)
{
	GHashTableIter iter;
	gpointer key, value;
	makiServer* serv = data;

	maki_server_channels_iter(serv, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		const gchar* chan_name = key;
		makiChannel* chan = value;

		if (maki_channel_autojoin(chan) || maki_channel_joined(chan))
		{
			maki_out_join(serv, chan_name, maki_channel_key(chan));
		}
	}

	return FALSE;
}

void maki_commands (makiServer* serv)
{
	if (serv->commands != NULL)
	{
		gchar** command;

		command = serv->commands;

		while (*command != NULL)
		{
			maki_server_send(serv, *command);
			++command;
		}
	}
}

void maki_in_privmsg (makiServer* serv, glong time, gchar* nick, gchar* remaining)
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
				if (maki_is_channel(serv, target))
				{
					maki_log(serv, target, "%s %s", nick, message + 7);
				}
				else
				{
					maki_log(serv, nick, "%s %s", nick, message + 7);
				}

				maki_dbus_emit_action(time, serv->server, nick, target, message + 7);
			}
			else
			{
				if (g_ascii_strcasecmp(target, serv->user->nick) == 0)
				{
					if (strncmp(message, "VERSION", 7) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001VERSION %s %s\001", nick, SUSHI_NAME, SUSHI_VERSION);
					}
					else if (strncmp(message, "PING", 4) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001%s\001", nick, message);
					}
				}

				if (maki_is_channel(serv, target))
				{
					maki_log(serv, target, "=%s= %s", nick, message);
					maki_dbus_emit_ctcp(time, serv->server, nick, target, message);
				}
				else
				{
					maki_log(serv, nick, "=%s= %s", nick, message);
					maki_dbus_emit_query_ctcp(time, serv->server, nick, message);
				}
			}
		}
		else
		{
			if (maki_is_channel(serv, target))
			{
				maki_log(serv, target, "<%s> %s", nick, message);
				maki_dbus_emit_message(time, serv->server, nick, target, message);
			}
			else
			{
				maki_log(serv, nick, "<%s> %s", nick, message);
				maki_dbus_emit_query(time, serv->server, nick, message);
			}
		}
	}

	g_strfreev(tmp);
}

void maki_in_join (makiServer* serv, glong time, gchar* nick, gchar* remaining)
{
	gchar* channel;
	makiChannel* chan;

	if (!remaining)
	{
		return;
	}

	channel = maki_remove_colon(remaining);
	/* XXX */
	g_strdelimit(channel, " ", '\0');

	if ((chan = maki_server_get_channel(serv, channel)) != NULL)
	{
		makiChannelUser* cuser;
		makiUser* user;

		user = maki_cache_insert(serv->users, nick);
		cuser = maki_channel_user_new(user);
		maki_channel_add_user(chan, cuser->user->nick, cuser);
	}

	if (g_ascii_strcasecmp(nick, serv->user->nick) == 0)
	{
		if (chan != NULL)
		{
			maki_channel_set_joined(chan, TRUE);
		}
		else
		{
			chan = maki_channel_new();
			maki_channel_set_joined(chan, TRUE);

			maki_server_add_channel(serv, channel, chan);
		}

		maki_log(serv, channel, _("» You join."));
	}
	else
	{
		maki_log(serv, channel, _("» %s joins."), nick);
	}

	maki_dbus_emit_join(time, serv->server, nick, channel);
}

void maki_in_part (makiServer* serv, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gchar* message;
	makiChannel* chan;

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

	if ((chan = maki_server_get_channel(serv, channel)) != NULL)
	{
		maki_channel_remove_user(chan, nick);
	}

	if (g_ascii_strcasecmp(nick, serv->user->nick) == 0)
	{
		if (chan != NULL)
		{
			maki_channel_set_joined(chan, FALSE);

			if (!maki_channel_autojoin(chan) && maki_channel_key(chan) == NULL)
			{
				maki_server_remove_channel(serv, channel);
			}
		}

		if (message != NULL)
		{
			maki_log(serv, channel, _("« You part (%s)."), message);
		}
		else
		{
			maki_log(serv, channel, _("« You part."));
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_log(serv, channel, _("« %s parts (%s)."), nick, message);
		}
		else
		{
			maki_log(serv, channel, _("« %s parts."), nick);
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_part(time, serv->server, nick, channel, message);
	}
	else
	{
		maki_dbus_emit_part(time, serv->server, nick, channel, "");
	}

	g_strfreev(tmp);
}

void maki_in_quit (makiServer* serv, glong time, gchar* nick, gchar* remaining)
{
	GHashTableIter iter;
	gpointer key, value;

	maki_server_channels_iter(serv, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		const gchar* chan_name = key;
		makiChannel* chan = value;

		if (!maki_channel_joined(chan))
		{
			continue;
		}

		if (maki_channel_get_user(chan, nick) != NULL)
		{
			if (remaining)
			{
				maki_log(serv, chan_name, _("« %s quits (%s)."), nick, maki_remove_colon(remaining));
			}
			else
			{
				maki_log(serv, chan_name, _("« %s quits."), nick);
			}
		}

		maki_channel_remove_user(chan, nick);
	}

	if (remaining)
	{
		maki_dbus_emit_quit(time, serv->server, nick, maki_remove_colon(remaining));
	}
	else
	{
		maki_dbus_emit_quit(time, serv->server, nick, "");
	}
}

void maki_in_kick (makiServer* serv, glong time, gchar* nick, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gchar* who;
	gchar* message;
	makiChannel* chan;

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

	if ((chan = maki_server_get_channel(serv, channel)) != NULL)
	{
		maki_channel_remove_user(chan, who);
	}

	if (g_ascii_strcasecmp(who, serv->user->nick) == 0)
	{
		if (chan != NULL)
		{
			maki_channel_set_joined(chan, FALSE);

			if (!maki_channel_autojoin(chan) && maki_channel_key(chan) == NULL)
			{
				maki_server_remove_channel(serv, channel);
			}
		}

		if (message != NULL)
		{
			maki_log(serv, channel, _("« %s kicks you (%s)."), nick, message);
		}
		else
		{
			maki_log(serv, channel, _("« %s kicks you."), nick);
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_log(serv, channel, _("« %s kicks %s (%s)."), nick, who, message);
		}
		else
		{
			maki_log(serv, channel, _("« %s kicks %s."), nick, who);
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_kick(time, serv->server, nick, channel, who, message);
	}
	else
	{
		maki_dbus_emit_kick(time, serv->server, nick, channel, who, "");
	}

	g_strfreev(tmp);
}

void maki_in_nick (makiServer* serv, glong time, gchar* nick, gchar* remaining)
{
	gboolean own = FALSE;
	gchar* new_nick;
	GHashTableIter iter;
	gpointer key, value;

	if (!remaining)
	{
		return;
	}

	new_nick = maki_remove_colon(remaining);

	if (g_ascii_strcasecmp(nick, serv->user->nick) == 0)
	{
		makiUser* user;

		user = maki_cache_insert(serv->users, new_nick);
		maki_user_copy(serv->user, user);
		maki_cache_remove(serv->users, serv->user->nick);
		serv->user = user;

		if (g_ascii_strcasecmp(serv->user->nick, serv->initial_nick) == 0)
		{
			maki_out_nickserv(serv);
		}

		own = TRUE;
	}

	maki_server_channels_iter(serv, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		const gchar* chan_name = key;
		makiChannel* chan = value;
		makiChannelUser* cuser;

		if (!maki_channel_joined(chan))
		{
			continue;
		}

		if ((cuser = maki_channel_get_user(chan, nick)) != NULL)
		{
			makiChannelUser* tmp;
			makiUser* user;

			user = maki_cache_insert(serv->users, new_nick);
			maki_user_copy(cuser->user, user);

			tmp = maki_channel_user_new(user);
			maki_channel_user_copy(cuser, tmp);

			maki_channel_remove_user(chan, nick);
			maki_channel_add_user(chan, tmp->user->nick, tmp);

			if (own)
			{
				maki_log(serv, chan_name, _("• You are now known as %s."), new_nick);
			}
			else
			{
				maki_log(serv, chan_name, _("• %s is now known as %s."), nick, new_nick);
			}
		}
	}

	maki_dbus_emit_nick(time, serv->server, nick, new_nick);
}

void maki_in_notice (makiServer* serv, glong time, gchar* nick, gchar* remaining)
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
		if (maki_is_channel(serv, target))
		{
			maki_log(serv, target, "-%s- %s", nick, message);
			maki_dbus_emit_notice(time, serv->server, nick, target, message);
		}
		else
		{
			maki_log(serv, nick, "-%s- %s", nick, message);
			maki_dbus_emit_query_notice(time, serv->server, nick, message);
		}
	}

	g_strfreev(tmp);
}

void maki_in_mode (makiServer* serv, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gboolean own;
	gint offset = 0;
	gchar** tmp;
	gchar* target;

	if (!remaining)
	{
		return;
	}

	own = (g_ascii_strcasecmp(nick, serv->user->nick) == 0);

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

				if (maki_mode_has_parameter(serv, sign, *mode) && i < length)
				{
					gint pos;

					if ((pos = maki_prefix_position(serv, FALSE, *mode)) >= 0)
					{
						makiChannel* chan;
						makiChannelUser* cuser;

						if ((chan = maki_server_get_channel(serv, target)) != NULL
								&& (cuser = maki_channel_get_user(chan, modes[i])) != NULL)
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
						maki_log(serv, target, _("• Mode: %s %s"), buffer, modes[i]);
					}
					else
					{
						if (own)
						{
							maki_log(serv, target, _("• You set mode: %s %s"), buffer, modes[i]);
						}
						else
						{
							maki_log(serv, target, _("• %s sets mode: %s %s"), nick, buffer, modes[i]);
						}
					}

					maki_dbus_emit_mode(time, serv->server, nick, target, buffer, modes[i]);
					++i;
				}
				else
				{
					if (is_numeric)
					{
						maki_log(serv, target, _("• Mode: %s"), buffer);
					}
					else
					{
						if (own)
						{
							maki_log(serv, target, _("• You set mode: %s"), buffer);
						}
						else
						{
							maki_log(serv, target, _("• %s sets mode: %s"), nick, buffer);
						}
					}

					maki_dbus_emit_mode(time, serv->server, nick, target, buffer, "");
				}
			}
		}

		g_strfreev(modes);
	}

	g_strfreev(tmp);
}

void maki_in_invite (makiServer* serv, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
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
		maki_log(serv, channel, _("• You successfully invite %s."), who);
	}
	else
	{
		maki_log(serv, channel, _("• %s invites %s."), nick, who);
	}

	maki_dbus_emit_invite(time, serv->server, nick, channel, who);

	g_strfreev(tmp);
}

void maki_in_topic (makiServer* serv, glong time, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gint offset = 0;
	gchar** tmp;
	gchar* channel;
	gchar* topic;
	makiChannel* chan;

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

	if ((chan = maki_server_get_channel(serv, channel)) != NULL)
	{
		maki_channel_set_topic(chan, topic);
	}

	if (is_numeric)
	{
		maki_log(serv, channel, _("• Topic: %s"), topic);
	}
	else
	{
		if (g_ascii_strcasecmp(nick, serv->user->nick) == 0)
		{
			maki_log(serv, channel, _("• You change the topic: %s"), topic);
		}
		else
		{
			maki_log(serv, channel, _("• %s changes the topic: %s"), nick, topic);
		}
	}

	maki_dbus_emit_topic(time, serv->server, nick, channel, topic);

	g_strfreev(tmp);
}

void maki_in_rpl_namreply (makiServer* serv, glong time, gchar* remaining)
{
	gchar** tmp;
	gchar* channel;
	gint i;
	guint length;
	makiChannel* chan;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 0);
	channel = tmp[2];
	length = g_strv_length(tmp);

	if (length > 3 && (chan = maki_server_get_channel(serv, channel)) != NULL)
	{
		for (i = 3; i < length; ++i)
		{
			gchar* nick = maki_remove_colon(tmp[i]);
			guint prefix = 0;
			gint pos;
			makiChannelUser* cuser;
			makiUser* user;

			if ((pos = maki_prefix_position(serv, TRUE, *nick)) >= 0)
			{
				prefix |= (1 << pos);
				++nick;
			}

			user = maki_cache_insert(serv->users, nick);
			cuser = maki_channel_user_new(user);
			maki_channel_add_user(chan, cuser->user->nick, cuser);
			cuser->prefix = prefix;
		}
	}

	g_strfreev(tmp);
}

void maki_in_rpl_away (makiServer* serv, glong time, gchar* remaining)
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

	maki_dbus_emit_away_message(time, serv->server, nick, message);

	g_strfreev(tmp);
}

void maki_in_rpl_isupport (makiServer* serv, glong time, gchar* remaining)
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
				g_free(serv->support.chanmodes);
				serv->support.chanmodes = g_strdup(support[1]);
			}
			else if (strncmp(support[0], "CHANTYPES", 9) == 0)
			{
				g_free(serv->support.chantypes);
				serv->support.chantypes = g_strdup(support[1]);
			}
			else if (strncmp(support[0], "PREFIX", 6) == 0)
			{
				gchar* paren;

				paren = strchr(support[1], ')');

				if (support[1][0] == '(' && paren != NULL)
				{
					*paren = '\0';
					g_free(serv->support.prefix.modes);
					g_free(serv->support.prefix.prefixes);
					serv->support.prefix.modes = g_strdup(support[1] + 1);
					serv->support.prefix.prefixes = g_strdup(paren + 1);
				}
			}
		}

		g_strfreev(support);
	}

	g_strfreev(tmp);
}

void maki_in_rpl_motd (makiServer* serv, glong time, gchar* remaining)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (tmp[0] != NULL && tmp[1] != NULL)
	{
		maki_dbus_emit_motd(time, serv->server, maki_remove_colon(tmp[1]));
	}

	g_strfreev(tmp);
}

void maki_in_rpl_list (makiServer* serv, glong time, gchar* remaining, gboolean is_end)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_list(time, serv->server, "", -1, "");
		return;
	}

	tmp = g_strsplit(remaining, " ", 4);

	if (g_strv_length(tmp) == 4)
	{
		maki_dbus_emit_list(time, serv->server, tmp[1], atol(tmp[2]), maki_remove_colon(tmp[3]));
	}

	g_strfreev(tmp);
}

void maki_in_rpl_banlist (makiServer* serv, glong time, gchar* remaining, gboolean is_end)
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
			maki_dbus_emit_banlist(time, serv->server, tmp[1], "", "", -1);
		}

		g_strfreev(tmp);

		return;
	}

	tmp = g_strsplit(remaining, " ", 5);
	length = g_strv_length(tmp);

	if (length == 5)
	{
		maki_dbus_emit_banlist(time, serv->server, tmp[1], tmp[2], tmp[3], atol(tmp[4]));
	}
	else if (length == 3)
	{
		/* This is what the RFC specifies. */
		maki_dbus_emit_banlist(time, serv->server, tmp[1], tmp[2], "", 0);
	}

	g_strfreev(tmp);
}

void maki_in_rpl_whois (makiServer* serv, glong time, gchar* remaining, gboolean is_end)
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
		maki_dbus_emit_whois(time, serv->server, tmp[1], "");

		g_strfreev(tmp);

		return;
	}

	maki_dbus_emit_whois(time, serv->server, tmp[1], tmp[2]);

	g_strfreev(tmp);
}

void maki_in_err_nosuchnick (makiServer* serv, glong time, gchar* remaining)
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

	maki_dbus_emit_invalid_target(time, serv->server, tmp[1]);

	g_strfreev(tmp);
}

/**
 * This function is run in its own thread.
 * It receives and handles all messages from sashimi.
 */
gpointer maki_in_runner (gpointer data)
{
	makiInstance* inst = data;

	for (;;)
	{
		gchar* message;
		GTimeVal time;
		makiServer* serv;
		sashimiMessage* msg;

		msg = g_async_queue_pop(maki_instance_queue(inst));

		if (G_UNLIKELY(msg->message == NULL && msg->data == NULL))
		{
			sashimi_message_free(msg);
			g_thread_exit(NULL);
		}

		serv = msg->data;
		message = msg->message;

		/* Check for valid UTF-8, because strange crashes can occur otherwise. */
		if (!g_utf8_validate(message, -1, NULL))
		{
			gchar* tmp;

			/* If the message is not in UTF-8 we will just assume that it is in ISO-8859-1. */
			if ((tmp = g_convert_with_fallback(message, -1, "UTF-8", "ISO-8859-1", "?", NULL, NULL, NULL)) == NULL)
			{
				sashimi_message_free(msg);
				continue;
			}

			g_free(message);
			msg->message = message = tmp;
		}

		g_get_current_time(&time);

		maki_debug("IN: [%ld] %s %s\n", time.tv_sec, serv->server, message);

		if (G_LIKELY(message[0] == ':'))
		{
			gchar** parts;
			gchar** from;
			gchar* from_nick;
			gchar* type;
			gchar* remaining;

			parts = g_strsplit(maki_remove_colon(message), " ", 3);

			if (g_strv_length(parts) < 2)
			{
				g_strfreev(parts);
				sashimi_message_free(msg);
				continue;
			}

			if (serv->ignores != NULL)
			{
				gboolean match = FALSE;
				gint i;
				guint length;

				length = g_strv_length(serv->ignores);

				for (i = 0; i < length; ++i)
				{
					if ((match = g_pattern_match_simple(serv->ignores[i], parts[0])))
					{
						break;
					}
				}

				if (match)
				{
					g_strfreev(parts);
					sashimi_message_free(msg);
					continue;
				}
			}

			from = g_strsplit_set(parts[0], "!@", 3);

			if (g_strv_length(from) < 1)
			{
				g_strfreev(parts);
				g_strfreev(from);
				sashimi_message_free(msg);
				continue;
			}

			from_nick = from[0];
			type = parts[1];
			remaining = parts[2];

			if (g_ascii_isdigit(type[0]) && g_ascii_isdigit(type[1]) && g_ascii_isdigit(type[2]))
			{
				gint numeric;

				numeric = 100 * g_ascii_digit_value(type[0]) + 10 * g_ascii_digit_value(type[1]) + g_ascii_digit_value(type[2]);

				switch (numeric)
				{
					/* RPL_CHANNELMODEIS */
					case 324:
						maki_in_mode(serv, time.tv_sec, from_nick, remaining, TRUE);
						break;
					/* RPL_INVITING */
					case 341:
						maki_in_invite(serv, time.tv_sec, from_nick, remaining, TRUE);
						break;
					/* RPL_NAMREPLY */
					case 353:
						maki_in_rpl_namreply(serv, time.tv_sec, remaining);
						break;
					/* RPL_UNAWAY */
					case 305:
						serv->user->away = FALSE;
						g_free(serv->user->away_message);
						serv->user->away_message = NULL;
						maki_dbus_emit_back(time.tv_sec, serv->server);
						break;
					/* RPL_NOWAWAY */
					case 306:
						serv->user->away = TRUE;
						maki_dbus_emit_away(time.tv_sec, serv->server);
						break;
					/* RPL_AWAY */
					case 301:
						maki_in_rpl_away(serv, time.tv_sec, remaining);
						break;
					/* RPL_ENDOFMOTD */
					case 376:
					/* ERR_NOMOTD */
					case 422:
						serv->connected = TRUE;
						maki_dbus_emit_connected(time.tv_sec, serv->server, serv->user->nick);
						maki_out_nickserv(serv);
						g_timeout_add_seconds(3, maki_join, serv);
						maki_commands(serv);

						if (serv->user->away && serv->user->away_message != NULL)
						{
							maki_out_away(serv, serv->user->away_message);
						}
						break;
					/* ERR_NICKNAMEINUSE */
					case 433:
						if (!serv->connected)
						{
							gchar* nick;
							makiUser* user;

							nick = g_strconcat(serv->user->nick, "_", NULL);
							user = maki_cache_insert(serv->users, nick);
							maki_user_copy(serv->user, user);
							maki_cache_remove(serv->users, serv->user->nick);
							serv->user = user;
							g_free(nick);

							maki_out_nick(serv, serv->user->nick);
						}
						break;
					/* ERR_NOSUCHNICK */
					case 401:
						maki_in_err_nosuchnick(serv, time.tv_sec, remaining);
						break;
					/* RPL_MOTD */
					case 372:
						maki_in_rpl_motd(serv, time.tv_sec, remaining);
						break;
					/* RPL_TOPIC */
					case 332:
						maki_in_topic(serv, time.tv_sec, from_nick, remaining, TRUE);
						break;
					/* RPL_WHOISUSER */
					case 311:
					/* RPL_WHOISSERVER */
					case 312:
					/* RPL_WHOISOPERATOR */
					case 313:
					/* RPL_WHOISIDLE */
					case 317:
					/* RPL_ENDOFWHOIS */
					case 318:
					/* RPL_WHOISCHANNELS */
					case 319:
						maki_in_rpl_whois(serv, time.tv_sec, remaining, (numeric == 318));
						break;
					/* RPL_ISUPPORT */
					case 5:
						maki_in_rpl_isupport(serv, time.tv_sec, remaining);
						break;
					/* RPL_LIST */
					case 322:
					/* RPL_LISTEND */
					case 323:
						maki_in_rpl_list(serv, time.tv_sec, remaining, (numeric == 323));
						break;
					/* RPL_BANLIST */
					case 367:
					/* RPL_ENDOFBANLIST */
					case 368:
						maki_in_rpl_banlist(serv, time.tv_sec, remaining, (numeric == 368));
						break;
					/* RPL_YOUREOPER */
					case 381:
						maki_dbus_emit_oper(time.tv_sec, serv->server);
						break;
					default:
						maki_debug("WARN: Unhandled numeric reply '%d'\n", numeric);
						break;
				}
			}
			else
			{
				if (strncmp(type, "PRIVMSG", 7) == 0)
				{
					maki_in_privmsg(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "JOIN", 4) == 0)
				{
					maki_in_join(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "PART", 4) == 0)
				{
					maki_in_part(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "QUIT", 4) == 0)
				{
					maki_in_quit(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "KICK", 4) == 0)
				{
					maki_in_kick(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "NICK", 4) == 0)
				{
					maki_in_nick(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "NOTICE", 6) == 0)
				{
					maki_in_notice(serv, time.tv_sec, from_nick, remaining);
				}
				else if (strncmp(type, "MODE", 4) == 0)
				{
					maki_in_mode(serv, time.tv_sec, from_nick, remaining, FALSE);
				}
				else if (strncmp(type, "INVITE", 6) == 0)
				{
					maki_in_invite(serv, time.tv_sec, from_nick, remaining, FALSE);
				}
				else if (strncmp(type, "TOPIC", 5) == 0)
				{
					maki_in_topic(serv, time.tv_sec, from_nick, remaining, FALSE);
				}
				else
				{
					maki_debug("WARN: Unhandled message type '%s'\n", type);
				}
			}

			g_strfreev(parts);
			g_strfreev(from);
		}

		sashimi_message_free(msg);
	}
}
