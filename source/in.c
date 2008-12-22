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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static gint maki_prefix_position (makiServer* serv, gboolean is_prefix, gchar prefix)
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
			gchar* channel_key;

			channel_key = maki_channel_key(chan);
			maki_out_join(serv, chan_name, channel_key);
			g_free(channel_key);
		}
	}

	return FALSE;
}

static void maki_commands (makiServer* serv)
{
	gchar** commands;

	commands = maki_server_config_get_string_list(serv, "server", "commands");

	if (commands != NULL)
	{
		guint i;
		guint length;

		length = g_strv_length(commands);

		for (i = 0; i < length; i++)
		{
			maki_server_send(serv, commands[i]);
		}
	}

	g_strfreev(commands);
}

static void maki_in_dcc_send (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
{
	gchar* file_name;
	gsize file_name_len;

	if (!remaining)
	{
		return;
	}

	if ((file_name = maki_dcc_send_get_file_name(remaining, &file_name_len)) != NULL)
	{
		gchar** args;
		guint args_len;

		args = g_strsplit(remaining + file_name_len + 1, " ", 4);
		args_len = g_strv_length(args);

		if (args_len >= 2)
		{
			guint32 address;
			guint16 port;
			goffset file_size;
			gchar* token;
			makiDCCSendIn* dcc;

			file_size = 0;
			token = NULL;

			address = g_ascii_strtoull(args[0], NULL, 10);
			port = g_ascii_strtoull(args[1], NULL, 10);

			if (args_len > 2)
			{
				file_size = g_ascii_strtoull(args[2], NULL, 10);
			}

			if (args_len > 3)
			{
				token = args[3];
			}

			dcc = maki_dcc_send_in_new(serv, nick, file_name, address, port, file_size, token);

			if (maki_instance_config_get_boolean(serv->instance, "dcc", "accept_send"))
			{
				maki_dcc_send_in_accept(dcc);
			}
		}

		g_strfreev(args);

		g_free(file_name);
	}
}

static void maki_in_privmsg (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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

				maki_dbus_emit_action(timestamp, serv->server, nick, target, message + 7);
			}
			else
			{
				if (g_ascii_strcasecmp(target, serv->user->nick) == 0)
				{
					if (strncmp(message, "VERSION", 7) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001VERSION maki %s\001", nick, SUSHI_VERSION);
					}
					else if (strncmp(message, "PING", 4) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001%s\001", nick, message);
					}
					else if (strncmp(message, "DCC SEND ", 9) == 0)
					{
						maki_in_dcc_send(serv, timestamp, nick, message + 9);
					}
				}

				if (maki_is_channel(serv, target))
				{
					maki_log(serv, target, "=%s= %s", nick, message);
					maki_dbus_emit_ctcp(timestamp, serv->server, nick, target, message);
				}
				else
				{
					maki_log(serv, nick, "=%s= %s", nick, message);
					maki_dbus_emit_query_ctcp(timestamp, serv->server, nick, message);
				}
			}
		}
		else
		{
			if (maki_is_channel(serv, target))
			{
				maki_log(serv, target, "<%s> %s", nick, message);
				maki_dbus_emit_message(timestamp, serv->server, nick, target, message);
			}
			else
			{
				maki_log(serv, nick, "<%s> %s", nick, message);
				maki_dbus_emit_query(timestamp, serv->server, nick, message);
			}
		}
	}

	g_strfreev(tmp);
}

static void maki_in_join (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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

	chan = maki_server_get_channel(serv, channel);

	if (g_ascii_strcasecmp(nick, serv->user->nick) == 0)
	{
		if (chan != NULL)
		{
			maki_channel_remove_users(chan);
			maki_channel_set_joined(chan, TRUE);
		}
		else
		{
			chan = maki_channel_new(serv, channel);
			maki_channel_set_joined(chan, TRUE);

			maki_server_add_channel(serv, channel, chan);
		}

		maki_log(serv, channel, _("» You join."));
	}
	else
	{
		maki_log(serv, channel, _("» %s joins."), nick);
	}

	if (chan != NULL)
	{
		makiChannelUser* cuser;
		makiUser* user;

		user = i_cache_insert(serv->users, nick);
		cuser = maki_channel_user_new(user);
		maki_channel_add_user(chan, cuser->user->nick, cuser);
	}

	maki_dbus_emit_join(timestamp, serv->server, nick, channel);
}

static void maki_in_part (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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
			gchar* key;

			key = maki_channel_key(chan);

			maki_channel_set_joined(chan, FALSE);
			maki_channel_remove_users(chan);

			if (!maki_channel_autojoin(chan) && key == NULL)
			{
				maki_server_remove_channel(serv, channel);
			}

			g_free(key);
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
		maki_dbus_emit_part(timestamp, serv->server, nick, channel, message);
	}
	else
	{
		maki_dbus_emit_part(timestamp, serv->server, nick, channel, "");
	}

	g_strfreev(tmp);
}

static void maki_in_quit (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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
		maki_dbus_emit_quit(timestamp, serv->server, nick, maki_remove_colon(remaining));
	}
	else
	{
		maki_dbus_emit_quit(timestamp, serv->server, nick, "");
	}
}

static void maki_in_kick (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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
			gchar* key;

			key = maki_channel_key(chan);

			maki_channel_set_joined(chan, FALSE);

			if (!maki_channel_autojoin(chan) && key == NULL)
			{
				maki_server_remove_channel(serv, channel);
			}

			g_free(key);
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
		maki_dbus_emit_kick(timestamp, serv->server, nick, channel, who, message);
	}
	else
	{
		maki_dbus_emit_kick(timestamp, serv->server, nick, channel, who, "");
	}

	g_strfreev(tmp);
}

static void maki_in_nick (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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
		gchar* initial_nick;
		makiUser* user;

		user = i_cache_insert(serv->users, new_nick);
		maki_user_copy(serv->user, user);
		i_cache_remove(serv->users, serv->user->nick);
		serv->user = user;

		initial_nick = maki_server_config_get_string(serv, "server", "nick");

		if (!maki_config_is_empty(initial_nick) && g_ascii_strcasecmp(serv->user->nick, initial_nick) == 0)
		{
			maki_out_nickserv(serv);
		}

		g_free(initial_nick);

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

			user = i_cache_insert(serv->users, new_nick);
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

	maki_dbus_emit_nick(timestamp, serv->server, nick, new_nick);
}

static void maki_in_notice (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining)
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
			maki_dbus_emit_notice(timestamp, serv->server, nick, target, message);
		}
		else
		{
			maki_log(serv, nick, "-%s- %s", nick, message);
			maki_dbus_emit_query_notice(timestamp, serv->server, nick, message);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_mode (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gboolean own;
	gchar** modes;
	gchar** tmp;
	gchar* target;

	if (!remaining)
	{
		return;
	}

	own = (g_ascii_strcasecmp(nick, serv->user->nick) == 0);

	if (is_numeric)
	{
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2);
	target = tmp[0];

	if (g_strv_length(tmp) < 2)
	{
		g_strfreev(tmp);
		return;
	}

	modes = g_strsplit(maki_remove_colon(tmp[1]), " ", 0);

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

				maki_dbus_emit_mode(timestamp, serv->server, nick, target, buffer, modes[i]);
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

				maki_dbus_emit_mode(timestamp, serv->server, nick, target, buffer, "");
			}
		}
	}

	g_strfreev(tmp);
	g_strfreev(modes);
}

static void maki_in_invite (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining, gboolean is_numeric)
{
	gchar** tmp;
	gchar* channel;
	gchar* who;

	if (!remaining)
	{
		return;
	}

	if (is_numeric)
	{
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) < 2)
	{
		g_strfreev(tmp);
		return;
	}

	who = tmp[0];
	channel = maki_remove_colon(tmp[1]);

	if (is_numeric)
	{
		maki_log(serv, channel, _("• You successfully invite %s."), who);
	}
	else
	{
		maki_log(serv, channel, _("• %s invites %s."), nick, who);
	}

	maki_dbus_emit_invite(timestamp, serv->server, nick, channel, who);

	g_strfreev(tmp);
}

static void maki_in_topic (makiServer* serv, glong timestamp, gchar* nick, gchar* remaining, gboolean is_numeric)
{
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
		nick = "";
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) < 2)
	{
		g_strfreev(tmp);
		return;
	}

	channel = tmp[0];
	topic = maki_remove_colon(tmp[1]);

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

	maki_dbus_emit_topic(timestamp, serv->server, nick, channel, topic);

	g_strfreev(tmp);
}

static void maki_in_rpl_namreply (makiServer* serv, glong timestamp, gchar* remaining, gboolean is_end)
{
	gchar** tmp;
	gint i;
	guint length;
	makiChannel* chan;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 0);
	length = g_strv_length(tmp);

	if (is_end)
	{
		if (length < 1)
		{
			g_strfreev(tmp);
			return;
		}

		maki_dbus_emit_names(timestamp, serv->server, "", tmp[0]);
	}
	else
	{
		if (length < 3)
		{
			g_strfreev(tmp);
			return;
		}

		if ((chan = maki_server_get_channel(serv, tmp[1])) != NULL)
		{
			for (i = 2; i < length; ++i)
			{
				gchar* nick = maki_remove_colon(tmp[i]);
				guint prefix = 0;
				gint pos;
				makiChannelUser* cuser;
				makiUser* user;

				while ((pos = maki_prefix_position(serv, TRUE, *nick)) >= 0)
				{
					prefix |= (1 << pos);
					++nick;
				}

				user = i_cache_insert(serv->users, nick);
				cuser = maki_channel_user_new(user);
				maki_channel_add_user(chan, cuser->user->nick, cuser);
				cuser->prefix = prefix;

				maki_dbus_emit_names(timestamp, serv->server, nick, tmp[1]);
			}
		}
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_away (makiServer* serv, glong timestamp, gchar* remaining)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) >= 2)
	{
		maki_dbus_emit_away_message(timestamp, serv->server, tmp[0], maki_remove_colon(tmp[1]));
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_isupport (makiServer* serv, glong timestamp, gchar* remaining)
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

	for (i = 0; i < length; ++i)
	{
		gchar** support;

		if (tmp[i][0] == ':')
		{
			break;
		}

		support = g_strsplit(tmp[i], "=", 2);

		if (g_strv_length(support) < 2)
		{
			g_strfreev(support);
			continue;
		}

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

		g_strfreev(support);
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_motd (makiServer* serv, glong timestamp, gchar* remaining, gboolean is_end)
{
	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_motd(timestamp, serv->server, "");
	}
	else
	{
		maki_dbus_emit_motd(timestamp, serv->server, maki_remove_colon(remaining));
	}
}

static void maki_in_rpl_list (makiServer* serv, glong timestamp, gchar* remaining, gboolean is_end)
{
	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_list(timestamp, serv->server, "", -1, "");
	}
	else
	{
		gchar** tmp;

		tmp = g_strsplit(remaining, " ", 3);

		if (g_strv_length(tmp) == 3)
		{
			maki_dbus_emit_list(timestamp, serv->server, tmp[0], atol(tmp[1]), maki_remove_colon(tmp[2]));
		}

		g_strfreev(tmp);
	}
}

static void maki_in_rpl_banlist (makiServer* serv, glong timestamp, gchar* remaining, gboolean is_end)
{
	guint length;
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 4);
	length = g_strv_length(tmp);

	if (is_end)
	{
		if (length >= 1)
		{
			maki_dbus_emit_banlist(timestamp, serv->server, tmp[0], "", "", -1);
		}
	}
	else
	{
		if (length == 4)
		{
			maki_dbus_emit_banlist(timestamp, serv->server, tmp[0], tmp[1], tmp[2], atol(tmp[3]));
		}
		else if (length == 2)
		{
			/* This is what the RFC specifies. */
			maki_dbus_emit_banlist(timestamp, serv->server, tmp[0], tmp[1], "", 0);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_whois (makiServer* serv, glong timestamp, gchar* remaining, gboolean is_end)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) >= 2)
	{
		if (is_end)
		{
			maki_dbus_emit_whois(timestamp, serv->server, tmp[0], "");
		}
		else
		{
			maki_dbus_emit_whois(timestamp, serv->server, tmp[0], tmp[1]);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_err_nosuch (makiServer* serv, glong timestamp, gchar* remaining, gint numeric)
{
	gchar type[2];
	gchar** tmp;

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

	switch (numeric)
	{
		/* ERR_NOSUCHNICK */
		case 401:
			type[0] = 'n';
			break;
		/* ERR_NOSUCHSERVER */
		case 402:
			type[0] = 's';
			break;
		/* ERR_NOSUCHCHANNEL */
		case 403:
			type[0] = 'c';
			break;
		default:
			g_warn_if_reached();
			break;
	}

	type[1] = '\0';

	maki_dbus_emit_no_such(timestamp, serv->server, tmp[0], type);

	g_strfreev(tmp);
}

static void maki_in_err_cannot_join (makiServer* serv, glong timestamp, gchar* remaining, gint numeric)
{
	gchar reason[2];
	gchar** tmp;

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

	switch (numeric)
	{
		/* ERR_CHANNELISFULL */
		case 471:
			reason[0] = 'l';
			break;
		/* ERR_INVITEONLYCHAN */
		case 473:
			reason[0] = 'i';
			break;
		/* ERR_BANNEDFROMCHAN */
		case 474:
			reason[0] = 'b';
			break;
		/* ERR_BADCHANNELKEY */
		case 475:
			reason[0] = 'k';
			break;
		default:
			g_warn_if_reached();
			break;
	}

	reason[1] = '\0';

	maki_dbus_emit_cannot_join(timestamp, serv->server, tmp[0], reason);

	g_strfreev(tmp);
}

/* This function receives and handles all messages from sashimi. */
void maki_in_callback (const gchar* message, gpointer data)
{
	makiServer* serv = data;
	GTimeVal timeval;

	/* Check for valid UTF-8, because strange crashes can occur otherwise. */
	if (!g_utf8_validate(message, -1, NULL))
	{
		gchar* tmp;

		/* If the message is not in UTF-8 we will just assume that it is in ISO-8859-1. */
		if ((tmp = g_convert_with_fallback(message, -1, "UTF-8", "ISO-8859-1", "?", NULL, NULL, NULL)) == NULL)
		{
			return;
		}

		maki_in_callback(tmp, serv);
		g_free(tmp);
		return;
	}

	g_get_current_time(&timeval);

	maki_debug("IN: [%ld/%s] %s\n", timeval.tv_sec, serv->server, message);

	if (G_LIKELY(message[0] == ':'))
	{
		gchar** parts;
		gchar** from;
		gchar** ignores;
		gchar* from_nick;
		gchar* type;
		gchar* remaining;

		parts = g_strsplit(message + 1, " ", 3);

		if (g_strv_length(parts) < 2)
		{
			g_strfreev(parts);
			return;
		}

		/* FIXME */
		ignores = maki_server_config_get_string_list(serv, "server", "ignores");

		if (ignores != NULL)
		{
			gint i;
			guint length;

			length = g_strv_length(ignores);

			for (i = 0; i < length; ++i)
			{
				if (g_pattern_match_simple(ignores[i], parts[0]))
				{
					g_strfreev(parts);
					g_strfreev(ignores);
					return;
				}
			}
		}

		g_strfreev(ignores);

		from = g_strsplit_set(parts[0], "!@", 3);

		if (g_strv_length(from) < 1)
		{
			g_strfreev(parts);
			g_strfreev(from);
			return;
		}

		from_nick = from[0];
		type = parts[1];
		remaining = parts[2];

		if (g_ascii_isdigit(type[0]) && g_ascii_isdigit(type[1]) && g_ascii_isdigit(type[2]))
		{
			gint numeric;

			numeric = 100 * g_ascii_digit_value(type[0]) + 10 * g_ascii_digit_value(type[1]) + g_ascii_digit_value(type[2]);

			while (*remaining != '\0')
			{
				if (*remaining == ' ')
				{
					remaining++;
					break;
				}

				remaining++;
			}

			switch (numeric)
			{
				/* RPL_CHANNELMODEIS */
				case 324:
					maki_in_mode(serv, timeval.tv_sec, from_nick, remaining, TRUE);
					break;
				/* RPL_INVITING */
				case 341:
					maki_in_invite(serv, timeval.tv_sec, from_nick, remaining, TRUE);
					break;
				/* RPL_NAMREPLY */
				case 353:
				/* RPL_ENDOFNAMES */
				case 366:
					maki_in_rpl_namreply(serv, timeval.tv_sec, remaining, (numeric == 366));
					break;
				/* RPL_UNAWAY */
				case 305:
					serv->user->away = FALSE;
					g_free(serv->user->away_message);
					serv->user->away_message = NULL;
					maki_dbus_emit_back(timeval.tv_sec, serv->server);
					break;
				/* RPL_NOWAWAY */
				case 306:
					serv->user->away = TRUE;
					maki_dbus_emit_away(timeval.tv_sec, serv->server);
					break;
				/* RPL_AWAY */
				case 301:
					maki_in_rpl_away(serv, timeval.tv_sec, remaining);
					break;
				/* RPL_ENDOFMOTD */
				case 376:
				/* ERR_NOMOTD */
				case 422:
					serv->logged_in = TRUE;
					maki_out_nickserv(serv);
					g_timeout_add_seconds(3, maki_join, serv);
					maki_commands(serv);

					if (serv->user->away && serv->user->away_message != NULL)
					{
						maki_out_away(serv, serv->user->away_message);
					}

					maki_in_rpl_motd(serv, timeval.tv_sec, remaining, TRUE);
					break;
				/* ERR_NICKNAMEINUSE */
				case 433:
					if (!serv->logged_in)
					{
						gchar* nick;
						makiUser* user;

						nick = g_strconcat(serv->user->nick, "_", NULL);

						maki_dbus_emit_nick(timeval.tv_sec, serv->server, serv->user->nick, nick);

						user = i_cache_insert(serv->users, nick);
						maki_user_copy(serv->user, user);
						i_cache_remove(serv->users, serv->user->nick);
						serv->user = user;
						g_free(nick);

						maki_out_nick(serv, serv->user->nick);
					}
					/* FIXME else */
					break;
				/* ERR_NOSUCHNICK */
				case 401:
				/* ERR_NOSUCHSERVER */
				case 402:
				/* ERR_NOSUCHCHANNEL */
				case 403:
					maki_in_err_nosuch(serv, timeval.tv_sec, remaining, numeric);
					break;
				/* RPL_MOTD */
				case 372:
					maki_in_rpl_motd(serv, timeval.tv_sec, remaining, FALSE);
					break;
				/* RPL_TOPIC */
				case 332:
					maki_in_topic(serv, timeval.tv_sec, from_nick, remaining, TRUE);
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
					maki_in_rpl_whois(serv, timeval.tv_sec, remaining, (numeric == 318));
					break;
				/* RPL_ISUPPORT */
				case 5:
					maki_in_rpl_isupport(serv, timeval.tv_sec, remaining);
					break;
				/* RPL_LIST */
				case 322:
				/* RPL_LISTEND */
				case 323:
					maki_in_rpl_list(serv, timeval.tv_sec, remaining, (numeric == 323));
					break;
				/* RPL_BANLIST */
				case 367:
				/* RPL_ENDOFBANLIST */
				case 368:
					maki_in_rpl_banlist(serv, timeval.tv_sec, remaining, (numeric == 368));
					break;
				/* RPL_YOUREOPER */
				case 381:
					maki_dbus_emit_oper(timeval.tv_sec, serv->server);
					break;
				/* ERR_CHANNELISFULL */
				case 471:
				/* ERR_INVITEONLYCHAN */
				case 473:
				/* ERR_BANNEDFROMCHAN */
				case 474:
				/* ERR_BADCHANNELKEY */
				case 475:
					maki_in_err_cannot_join(serv, timeval.tv_sec, remaining, numeric);
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
				maki_in_privmsg(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "JOIN", 4) == 0)
			{
				maki_in_join(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "PART", 4) == 0)
			{
				maki_in_part(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "QUIT", 4) == 0)
			{
				maki_in_quit(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "KICK", 4) == 0)
			{
				maki_in_kick(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "NICK", 4) == 0)
			{
				maki_in_nick(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "NOTICE", 6) == 0)
			{
				maki_in_notice(serv, timeval.tv_sec, from_nick, remaining);
			}
			else if (strncmp(type, "MODE", 4) == 0)
			{
				maki_in_mode(serv, timeval.tv_sec, from_nick, remaining, FALSE);
			}
			else if (strncmp(type, "INVITE", 6) == 0)
			{
				maki_in_invite(serv, timeval.tv_sec, from_nick, remaining, FALSE);
			}
			else if (strncmp(type, "TOPIC", 5) == 0)
			{
				maki_in_topic(serv, timeval.tv_sec, from_nick, remaining, FALSE);
			}
			else
			{
				maki_debug("WARN: Unhandled message type '%s'\n", type);
			}
		}

		g_strfreev(parts);
		g_strfreev(from);
	}
}
