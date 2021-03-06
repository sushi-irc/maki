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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ilib.h>

#include "in.h"

#include "dbus.h"
#include "dcc_send.h"
#include "instance.h"
#include "maki.h"
#include "misc.h"
#include "out.h"
#include "server.h"

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

static gboolean
maki_mode_has_parameter (makiServer* serv, gchar sign, gchar mode)
{
	guint type;
	const gchar* chanmodes;

	if (strchr(maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_MODES), mode) != NULL)
	{
		return TRUE;
	}

	if ((chanmodes = maki_server_support(serv, MAKI_SERVER_SUPPORT_CHANMODES)) == NULL)
	{
		return FALSE;
	}

	for (type = 0; *chanmodes != '\0'; chanmodes++)
	{
		if (*chanmodes == ',')
		{
			type++;
			continue;
		}

		if (*chanmodes == mode)
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

static gboolean
maki_is_channel(makiServer* serv, const gchar* target)
{
	return (strchr(maki_server_support(serv, MAKI_SERVER_SUPPORT_CHANTYPES), target[0]) != NULL);
}

static gint
maki_prefix_position (makiServer* serv, gboolean is_prefix, gchar prefix)
{
	guint pos = 0;
	const gchar* str = (is_prefix) ? maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES) : maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_MODES);

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

static void maki_in_dcc_send (makiServer* serv, makiUser* user, gchar* remaining)
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
			goffset file_size = 0;
			guint32 token = 0;
			makiDCCSend* dcc;
			makiInstance* inst = maki_instance_get_default();

			address = g_ascii_strtoull(args[0], NULL, 10);
			port = g_ascii_strtoull(args[1], NULL, 10);

			if (args_len > 2)
			{
				file_size = g_ascii_strtoull(args[2], NULL, 10);
			}

			if (args_len > 3)
			{
				token = g_ascii_strtoull(args[3], NULL, 10);
			}

			dcc = maki_dcc_send_new_in(serv, user, file_name, address, port, file_size, token);

			maki_instance_add_dcc_send(inst, dcc);
		}

		g_strfreev(args);

		g_free(file_name);
	}
}

static void maki_in_dcc_resume_accept (makiServer* serv, makiUser* user, gchar* remaining, gboolean is_incoming)
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

		args = g_strsplit(remaining + file_name_len + 1, " ", 3);
		args_len = g_strv_length(args);

		if (args_len >= 2)
		{
			guint16 port;
			goffset position;
			guint32 token = 0;
			makiInstance* inst = maki_instance_get_default();

			port = g_ascii_strtoull(args[0], NULL, 10);
			position = g_ascii_strtoull(args[1], NULL, 10);

			if (args_len > 2)
			{
				token = g_ascii_strtoull(args[2], NULL, 10);
			}

			if (maki_instance_resume_accept_dcc_send(inst, file_name, port, position, token, is_incoming))
			{
				if (!is_incoming)
				{
					if (token > 0)
					{
						maki_server_send_printf(serv, "PRIVMSG %s :\001DCC ACCEPT %s %" G_GUINT16_FORMAT " %" G_GUINT64_FORMAT " %" G_GUINT32_FORMAT "\001", maki_user_nick(user), file_name, port, position, token);
					}
					else
					{
						maki_server_send_printf(serv, "PRIVMSG %s :\001DCC ACCEPT %s %" G_GUINT16_FORMAT " %" G_GUINT64_FORMAT "\001", maki_user_nick(user), file_name, port, position);
					}
				}
			}
		}

		g_strfreev(args);

		g_free(file_name);
	}
}

static void maki_in_privmsg (makiServer* serv, makiUser* user, gchar* remaining)
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
					maki_server_log(serv, target, "%s %s", maki_user_nick(user), message + 7);
				}
				else
				{
					maki_server_log(serv, maki_user_nick(user), "%s %s", maki_user_nick(user), message + 7);
				}

				maki_dbus_emit_action(maki_server_name(serv), maki_user_from(user), target, message + 7);
			}
			else
			{
				if (g_ascii_strcasecmp(target, maki_user_nick(maki_server_user(serv))) == 0)
				{
					if (strncmp(message, "VERSION", 7) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001VERSION maki %s\001", maki_user_nick(user), SUSHI_VERSION);
					}
					else if (strncmp(message, "PING", 4) == 0)
					{
						maki_server_send_printf(serv, "NOTICE %s :\001%s\001", maki_user_nick(user), message);
					}
					else if (strncmp(message, "DCC SEND ", 9) == 0)
					{
						maki_in_dcc_send(serv, user, message + 9);
					}
					else if (strncmp(message, "DCC RESUME ", 11) == 0)
					{
						maki_in_dcc_resume_accept(serv, user, message + 11, FALSE);
					}
					else if (strncmp(message, "DCC ACCEPT ", 11) == 0)
					{
						maki_in_dcc_resume_accept(serv, user, message + 11, TRUE);
					}
				}

				if (maki_is_channel(serv, target))
				{
					maki_server_log(serv, target, "=%s= %s", maki_user_nick(user), message);
				}
				else
				{
					maki_server_log(serv, maki_user_nick(user), "=%s= %s", maki_user_nick(user), message);
				}

				maki_dbus_emit_ctcp(maki_server_name(serv), maki_user_from(user), target, message);
			}
		}
		else
		{
			if (maki_is_channel(serv, target))
			{
				maki_server_log(serv, target, "<%s> %s", maki_user_nick(user), message);
			}
			else
			{
				maki_server_log(serv, maki_user_nick(user), "<%s> %s", maki_user_nick(user), message);
			}

			maki_dbus_emit_message(maki_server_name(serv), maki_user_from(user), target, message);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_join (makiServer* serv, makiUser* user, gchar* remaining)
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

	if (g_ascii_strcasecmp(maki_user_nick(user), maki_user_nick(maki_server_user(serv))) == 0)
	{
		if (chan != NULL)
		{
			maki_channel_set_joined(chan, TRUE);
		}
		else
		{
			chan = maki_channel_new(serv, channel);
			maki_channel_set_joined(chan, TRUE);

			maki_server_add_channel(serv, channel, chan);
		}

		maki_server_log(serv, channel, _("» You join."));
	}
	else
	{
		maki_server_log(serv, channel, _("» %s joins."), maki_user_nick(user));
	}

	if (chan != NULL)
	{
		maki_channel_add_user(chan, maki_user_nick(user));
	}

	maki_dbus_emit_join(maki_server_name(serv), maki_user_from(user), channel);
}

static void maki_in_part (makiServer* serv, makiUser* user, gchar* remaining)
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
		maki_channel_remove_user(chan, maki_user_nick(user));
	}

	if (g_ascii_strcasecmp(maki_user_nick(user), maki_user_nick(maki_server_user(serv))) == 0)
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
			maki_server_log(serv, channel, _("« You part (%s)."), message);
		}
		else
		{
			maki_server_log(serv, channel, _("« You part."));
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_server_log(serv, channel, _("« %s parts (%s)."), maki_user_nick(user), message);
		}
		else
		{
			maki_server_log(serv, channel, _("« %s parts."), maki_user_nick(user));
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_part(maki_server_name(serv), maki_user_from(user), channel, message);
	}
	else
	{
		maki_dbus_emit_part(maki_server_name(serv), maki_user_from(user), channel, "");
	}

	g_strfreev(tmp);
}

static void maki_in_quit (makiServer* serv, makiUser* user, gchar* remaining)
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

		if (maki_channel_get_user(chan, maki_user_nick(user)) != NULL)
		{
			if (remaining)
			{
				maki_server_log(serv, chan_name, _("« %s quits (%s)."), maki_user_nick(user), maki_remove_colon(remaining));
			}
			else
			{
				maki_server_log(serv, chan_name, _("« %s quits."), maki_user_nick(user));
			}
		}

		maki_channel_remove_user(chan, maki_user_nick(user));
	}

	if (remaining)
	{
		maki_dbus_emit_quit(maki_server_name(serv), maki_user_from(user), maki_remove_colon(remaining));
	}
	else
	{
		maki_dbus_emit_quit(maki_server_name(serv), maki_user_from(user), "");
	}
}

static void maki_in_kick (makiServer* serv, makiUser* user, gchar* remaining)
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

	if (g_ascii_strcasecmp(who, maki_user_nick(maki_server_user(serv))) == 0)
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
			maki_server_log(serv, channel, _("« %s kicks you (%s)."), maki_user_nick(user), message);
		}
		else
		{
			maki_server_log(serv, channel, _("« %s kicks you."), maki_user_nick(user));
		}
	}
	else
	{
		if (message != NULL)
		{
			maki_server_log(serv, channel, _("« %s kicks %s (%s)."), maki_user_nick(user), who, message);
		}
		else
		{
			maki_server_log(serv, channel, _("« %s kicks %s."), maki_user_nick(user), who);
		}
	}

	if (message != NULL)
	{
		maki_dbus_emit_kick(maki_server_name(serv), maki_user_from(user), channel, who, message);
	}
	else
	{
		maki_dbus_emit_kick(maki_server_name(serv), maki_user_from(user), channel, who, "");
	}

	g_strfreev(tmp);
}

static void maki_in_nick (makiServer* serv, makiUser* user, gchar* remaining)
{
	gboolean own;
	gchar* new_nick;
	GHashTableIter iter;
	gpointer key, value;

	if (!remaining)
	{
		return;
	}

	new_nick = maki_remove_colon(remaining);
	own = (g_ascii_strcasecmp(maki_user_nick(user), maki_user_nick(maki_server_user(serv))) == 0);

	maki_server_channels_iter(serv, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		const gchar* chan_name = key;
		makiChannel* chan = value;

		if (!maki_channel_joined(chan))
		{
			continue;
		}

		if (maki_channel_rename_user(chan, maki_user_nick(user), new_nick) != NULL)
		{
			if (own)
			{
				maki_server_log(serv, chan_name, _("• You are now known as %s."), new_nick);
			}
			else
			{
				maki_server_log(serv, chan_name, _("• %s is now known as %s."), maki_user_nick(user), new_nick);
			}
		}
	}

	maki_dbus_emit_nick(maki_server_name(serv), maki_user_from(user), new_nick);
	maki_server_rename_user(serv, maki_user_nick(user), new_nick);

	if (own)
	{
		gchar* initial_nick;

		/* FIXME redundant */
		maki_server_rename_user(serv, maki_user_nick(maki_server_user(serv)), new_nick);

		initial_nick = maki_server_config_get_string(serv, "server", "nick");

		if (!maki_config_is_empty(initial_nick) && g_ascii_strcasecmp(new_nick, initial_nick) == 0)
		{
			maki_out_nickserv(serv);
		}

		g_free(initial_nick);
	}
}

static void maki_in_notice (makiServer* serv, makiUser* user, gchar* remaining)
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
			maki_server_log(serv, target, "-%s- %s", maki_user_nick(user), message);
		}
		else
		{
			maki_server_log(serv, maki_user_nick(user), "-%s- %s", maki_user_nick(user), message);
		}

		maki_dbus_emit_notice(maki_server_name(serv), maki_user_from(user), target, message);
	}

	g_strfreev(tmp);
}

static void maki_in_mode (makiServer* serv, makiUser* user, gchar* remaining, gboolean is_numeric)
{
	gboolean own;
	gchar** modes;
	gchar** tmp;
	gchar* target;

	if (!remaining)
	{
		return;
	}

	own = (g_ascii_strcasecmp(maki_user_nick(user), maki_user_nick(maki_server_user(serv))) == 0);

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
		guint i;
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
					makiUser* muser;

					if ((chan = maki_server_get_channel(serv, target)) != NULL
					    && (muser = maki_channel_get_user(chan, modes[i])) != NULL)
					{
						maki_channel_set_user_prefix(chan, muser, pos, (sign == '+'));
					}
				}

				if (is_numeric)
				{
					maki_server_log(serv, target, _("• Mode: %s %s"), buffer, modes[i]);

					maki_dbus_emit_mode(maki_server_name(serv), "", target, buffer, modes[i]);
				}
				else
				{
					if (own)
					{
						maki_server_log(serv, target, _("• You set mode: %s %s"), buffer, modes[i]);
					}
					else
					{
						maki_server_log(serv, target, _("• %s sets mode: %s %s"), maki_user_nick(user), buffer, modes[i]);
					}

					maki_dbus_emit_mode(maki_server_name(serv), maki_user_from(user), target, buffer, modes[i]);
				}

				++i;
			}
			else
			{
				if (is_numeric)
				{
					maki_server_log(serv, target, _("• Mode: %s"), buffer);

					maki_dbus_emit_mode(maki_server_name(serv), "", target, buffer, "");
				}
				else
				{
					if (own)
					{
						maki_server_log(serv, target, _("• You set mode: %s"), buffer);
					}
					else
					{
						maki_server_log(serv, target, _("• %s sets mode: %s"), maki_user_nick(user), buffer);
					}

					maki_dbus_emit_mode(maki_server_name(serv), maki_user_from(user), target, buffer, "");
				}
			}
		}
	}

	g_strfreev(tmp);
	g_strfreev(modes);
}

static void maki_in_invite (makiServer* serv, makiUser* user, gchar* remaining, gboolean is_numeric)
{
	gchar** tmp;
	gchar* channel;
	gchar* who;

	if (!remaining)
	{
		return;
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
		maki_server_log(serv, channel, _("• You successfully invite %s."), who);

		maki_dbus_emit_invite(maki_server_name(serv), "", channel, who);
	}
	else
	{
		maki_server_log(serv, channel, _("• %s invites %s."), maki_user_nick(user), who);

		maki_dbus_emit_invite(maki_server_name(serv), maki_user_from(user), channel, who);
	}

	g_strfreev(tmp);
}

static void maki_in_topic (makiServer* serv, makiUser* user, gchar* remaining, gboolean is_numeric)
{
	gchar** tmp;
	gchar* channel;
	gchar* topic;
	makiChannel* chan;

	if (!remaining)
	{
		return;
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
		maki_server_log(serv, channel, _("• Topic: %s"), topic);

		maki_dbus_emit_topic(maki_server_name(serv), "", channel, topic);
	}
	else
	{
		if (g_ascii_strcasecmp(maki_user_nick(user), maki_user_nick(maki_server_user(serv))) == 0)
		{
			maki_server_log(serv, channel, _("• You change the topic: %s"), topic);
		}
		else
		{
			maki_server_log(serv, channel, _("• %s changes the topic: %s"), maki_user_nick(user), topic);
		}

		maki_dbus_emit_topic(maki_server_name(serv), maki_user_from(user), channel, topic);
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_namreply (makiServer* serv, gchar* remaining, gboolean is_end)
{
	gchar** nicks;
	gchar** prefixes;
	gchar** tmp;
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

		nicks = g_new(gchar*, 1);

		nicks[0] = NULL;

		maki_dbus_emit_names(maki_server_name(serv), tmp[0], nicks, nicks);

		g_free(nicks);
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
			guint i;
			guint j;

			nicks = g_new(gchar*, length - 1);
			prefixes = g_new(gchar*, length - 1);

			nicks[length - 2] = prefixes[length - 2] = NULL;

			for (i = 2, j = 0; i < length; i++, j++)
			{
				gchar* nick = maki_remove_colon(tmp[i]);
				gchar prefix_str[2];
				guint prefix = 0;
				gint pos;
				makiUser* user;

				prefix_str[0] = '\0';
				prefix_str[1] = '\0';

				while ((pos = maki_prefix_position(serv, TRUE, *nick)) >= 0)
				{
					if (prefix_str[0] == '\0')
					{
						prefix_str[0] = maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES)[pos];
					}

					prefix |= (1 << pos);
					nick++;
				}

				user = maki_channel_add_user(chan, nick);
				maki_channel_set_user_prefix_override(chan, user, prefix);

				nicks[j] = nick;
				prefixes[j] = g_strdup(prefix_str);
			}

			maki_dbus_emit_names(maki_server_name(serv), tmp[1], nicks, prefixes);

			g_strfreev(prefixes);
			g_free(nicks);
		}
	}

	g_strfreev(tmp);
}

/* FIXME handle more stuff */
static void maki_in_rpl_whoreply (makiServer* serv, gchar* remaining, gboolean is_end)
{
	gchar** tmp;
	guint length;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 8);
	length = g_strv_length(tmp);

	if (is_end)
	{
	}
	else
	{
		makiUser* user;

		if (length < 8)
		{
			g_strfreev(tmp);
			return;
		}

		if ((user = maki_server_get_user(serv, tmp[4])) != NULL)
		{
			gboolean away;

			away = (tmp[5][0] == 'G');

			if (maki_user_away(user) != away)
			{
				maki_user_set_away(user, away);

				maki_dbus_emit_user_away(maki_server_name(serv), maki_user_from(user), maki_user_away(user));
			}
		}
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_away (makiServer* serv, gchar* remaining)
{
	gchar** tmp;

	if (!remaining)
	{
		return;
	}

	tmp = g_strsplit(remaining, " ", 2);

	if (g_strv_length(tmp) >= 2)
	{
		maki_dbus_emit_away_message(maki_server_name(serv), tmp[0], maki_remove_colon(tmp[1]));
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_isupport (makiServer* serv, gchar* remaining)
{
	guint i;
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
			maki_server_set_support(serv, MAKI_SERVER_SUPPORT_CHANMODES, support[1]);
		}
		else if (strncmp(support[0], "CHANTYPES", 9) == 0)
		{
			maki_server_set_support(serv, MAKI_SERVER_SUPPORT_CHANTYPES, support[1]);
		}
		else if (strncmp(support[0], "PREFIX", 6) == 0)
		{
			gchar* paren;

			paren = strchr(support[1], ')');

			if (support[1][0] == '(' && paren != NULL)
			{
				*paren = '\0';
				maki_server_set_support(serv, MAKI_SERVER_SUPPORT_PREFIX_MODES, support[1] + 1);
				maki_server_set_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES, paren + 1);
			}
		}

		g_strfreev(support);
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_motd (makiServer* serv, gchar* remaining, gboolean is_end)
{
	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_motd(maki_server_name(serv), "");
	}
	else
	{
		maki_dbus_emit_motd(maki_server_name(serv), maki_remove_colon(remaining));
	}
}

static void maki_in_rpl_list (makiServer* serv, gchar* remaining, gboolean is_end)
{
	if (!remaining)
	{
		return;
	}

	if (is_end)
	{
		maki_dbus_emit_list(maki_server_name(serv), "", -1, "");
	}
	else
	{
		gchar** tmp;

		tmp = g_strsplit(remaining, " ", 3);

		if (g_strv_length(tmp) == 3)
		{
			maki_dbus_emit_list(maki_server_name(serv), tmp[0], atol(tmp[1]), maki_remove_colon(tmp[2]));
		}

		g_strfreev(tmp);
	}
}

static void maki_in_rpl_banlist (makiServer* serv, gchar* remaining, gboolean is_end)
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
			maki_dbus_emit_banlist(maki_server_name(serv), tmp[0], "", "", -1);
		}
	}
	else
	{
		if (length == 4)
		{
			maki_dbus_emit_banlist(maki_server_name(serv), tmp[0], tmp[1], tmp[2], atol(tmp[3]));
		}
		else if (length == 2)
		{
			/* This is what the RFC specifies. */
			maki_dbus_emit_banlist(maki_server_name(serv), tmp[0], tmp[1], "", 0);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_rpl_whois (makiServer* serv, gchar* remaining, gboolean is_end)
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
			maki_dbus_emit_whois(maki_server_name(serv), tmp[0], "");
		}
		else
		{
			maki_dbus_emit_whois(maki_server_name(serv), tmp[0], tmp[1]);
		}
	}

	g_strfreev(tmp);
}

static void maki_in_err_nosuch (makiServer* serv, gchar* remaining, gint numeric)
{
	const gchar* reason = "";
	const gchar* type = "";
	gchar** arguments;
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
			reason = "nick";
			type = "n";
			break;
		/* ERR_NOSUCHSERVER */
		case 402:
			reason = "server";
			type = "s";
			break;
		/* ERR_NOSUCHCHANNEL */
		case 403:
			reason = "channel";
			type = "c";
			break;
		default:
			g_warn_if_reached();
			break;
	}

	arguments = i_strv_new(NULL, tmp[0], NULL);

	maki_dbus_emit_error(maki_server_name(serv), "no_such", reason, arguments);
	maki_dbus_emit_no_such(maki_server_name(serv), tmp[0], type);

	g_free(arguments);

	g_strfreev(tmp);
}

static void maki_in_err_cannot_join (makiServer* serv, gchar* remaining, gint numeric)
{
	const gchar* reason = "";
	const gchar* type = "";
	gchar** arguments;
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
			reason = "full";
			type = "l";
			break;
		/* ERR_INVITEONLYCHAN */
		case 473:
			reason = "invite";
			type = "i";
			break;
		/* ERR_BANNEDFROMCHAN */
		case 474:
			reason = "banned";
			type = "b";
			break;
		/* ERR_BADCHANNELKEY */
		case 475:
			reason = "key";
			type = "k";
			break;
		default:
			g_warn_if_reached();
			break;
	}

	arguments = i_strv_new(NULL, tmp[0], NULL);

	maki_dbus_emit_error(maki_server_name(serv), "cannot_join", reason, arguments);
	maki_dbus_emit_cannot_join(maki_server_name(serv), tmp[0], type);

	g_free(arguments);

	g_strfreev(tmp);
}

static void maki_in_err_chanoprivsneeded (makiServer* serv, gchar* remaining)
{
	gchar** arguments;
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

	arguments = i_strv_new(NULL, tmp[0], NULL);

	maki_dbus_emit_error(maki_server_name(serv), "privilege", "channel_operator", arguments);

	g_free(arguments);

	g_strfreev(tmp);
}

/* This function receives and handles all messages from sashimi. */
void maki_in_callback (const gchar* message, gpointer data)
{
	makiServer* serv = data;

	/* Check for valid UTF-8, because strange crashes can occur otherwise. */
	if (!g_utf8_validate(message, -1, NULL))
	{
		gchar* tmp;

		/* If the message is not in UTF-8 we will just assume that it is in ISO-8859-1. */
		if ((tmp = g_convert_with_fallback(message, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL, NULL)) == NULL)
		{
			return;
		}

		maki_in_callback(tmp, serv);
		g_free(tmp);
		return;
	}

	/* Extra check to avoid string operations when verbose is disabled. */
	if (opt_verbose)
	{
		gchar* time_str;

		if ((time_str = i_get_current_time_string("%Y-%m-%d %H:%M:%S")) != NULL)
		{
			maki_debug("IN: [%s/%s] %s\n", time_str, maki_server_name(serv), message);
			g_free(time_str);
		}
		else
		{
			maki_debug("IN: [%s] %s\n", maki_server_name(serv), message);
		}
	}

	if (G_LIKELY(message[0] == ':'))
	{
		gchar** parts;
		gchar** from;
		gchar** ignores;
		gchar* type;
		gchar* remaining;
		guint from_length;
		makiUser* user;

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
			guint i;
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
		from_length = g_strv_length(from);

		if (from_length < 1)
		{
			g_strfreev(parts);
			g_strfreev(from);
			return;
		}

		type = parts[1];
		remaining = parts[2];

		user = maki_server_add_user(serv, from[0]);

		if (from_length > 2)
		{
			maki_user_set_user(user, from[1]);
			maki_user_set_host(user, from[2]);
		}

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
					maki_in_mode(serv, user, remaining, TRUE);
					break;
				/* RPL_INVITING */
				case 341:
					maki_in_invite(serv, user, remaining, TRUE);
					break;
				/* RPL_NAMREPLY */
				case 353:
				/* RPL_ENDOFNAMES */
				case 366:
					maki_in_rpl_namreply(serv, remaining, (numeric == 366));
					break;
				/* RPL_WHOREPLY */
				case 352:
				/* RPL_ENDOFWHO */
				case 315:
					maki_in_rpl_whoreply(serv, remaining, (numeric == 315));
					break;
				/* RPL_UNAWAY */
				case 305:
					maki_user_set_away(maki_server_user(serv), FALSE);
					maki_user_set_away_message(maki_server_user(serv), NULL);
					maki_dbus_emit_back(maki_server_name(serv));
					maki_dbus_emit_user_away(maki_server_name(serv), maki_user_from(maki_server_user(serv)), FALSE);
					break;
				/* RPL_NOWAWAY */
				case 306:
					maki_user_set_away(maki_server_user(serv), TRUE);
					maki_dbus_emit_away(maki_server_name(serv));
					maki_dbus_emit_user_away(maki_server_name(serv), maki_user_from(maki_server_user(serv)), TRUE);
					break;
				/* RPL_AWAY */
				case 301:
					maki_in_rpl_away(serv, remaining);
					break;
				/* RPL_ENDOFMOTD */
				case 376:
				/* ERR_NOMOTD */
				case 422:
					maki_server_set_logged_in(serv, TRUE);
					maki_out_nickserv(serv);
					g_timeout_add_seconds(3, maki_join, serv);
					maki_commands(serv);

					if (maki_user_away(maki_server_user(serv)) && maki_user_away_message(maki_server_user(serv)) != NULL)
					{
						maki_out_away(serv, maki_user_away_message(maki_server_user(serv)));
					}

					maki_in_rpl_motd(serv, remaining, TRUE);
					break;
				/* ERR_NICKNAMEINUSE */
				case 433:
					if (!maki_server_logged_in(serv))
					{
						gchar* nick;

						nick = g_strconcat(maki_user_nick(maki_server_user(serv)), "_", NULL);

						maki_dbus_emit_nick(maki_server_name(serv), maki_user_nick(maki_server_user(serv)), nick);

						maki_server_rename_user(serv, maki_user_nick(maki_server_user(serv)), nick);

						g_free(nick);

						maki_out_nick(serv, maki_user_nick(maki_server_user(serv)));
					}
					/* FIXME else */
					break;
				/* ERR_NOSUCHNICK */
				case 401:
				/* ERR_NOSUCHSERVER */
				case 402:
				/* ERR_NOSUCHCHANNEL */
				case 403:
					maki_in_err_nosuch(serv, remaining, numeric);
					break;
				/* RPL_MOTD */
				case 372:
					maki_in_rpl_motd(serv, remaining, FALSE);
					break;
				/* RPL_TOPIC */
				case 332:
					maki_in_topic(serv, user, remaining, TRUE);
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
					maki_in_rpl_whois(serv, remaining, (numeric == 318));
					break;
				/* RPL_ISUPPORT */
				case 5:
					maki_in_rpl_isupport(serv, remaining);
					break;
				/* RPL_LIST */
				case 322:
				/* RPL_LISTEND */
				case 323:
					maki_in_rpl_list(serv, remaining, (numeric == 323));
					break;
				/* RPL_BANLIST */
				case 367:
				/* RPL_ENDOFBANLIST */
				case 368:
					maki_in_rpl_banlist(serv, remaining, (numeric == 368));
					break;
				/* RPL_YOUREOPER */
				case 381:
					maki_dbus_emit_oper(maki_server_name(serv));
					break;
				/* ERR_CHANNELISFULL */
				case 471:
				/* ERR_INVITEONLYCHAN */
				case 473:
				/* ERR_BANNEDFROMCHAN */
				case 474:
				/* ERR_BADCHANNELKEY */
				case 475:
					maki_in_err_cannot_join(serv, remaining, numeric);
					break;
				case 482:
					maki_in_err_chanoprivsneeded(serv, remaining);
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
				maki_in_privmsg(serv, user, remaining);
			}
			else if (strncmp(type, "JOIN", 4) == 0)
			{
				maki_in_join(serv, user, remaining);
			}
			else if (strncmp(type, "PART", 4) == 0)
			{
				maki_in_part(serv, user, remaining);
			}
			else if (strncmp(type, "QUIT", 4) == 0)
			{
				maki_in_quit(serv, user, remaining);
			}
			else if (strncmp(type, "KICK", 4) == 0)
			{
				maki_in_kick(serv, user, remaining);
			}
			else if (strncmp(type, "NICK", 4) == 0)
			{
				maki_in_nick(serv, user, remaining);
			}
			else if (strncmp(type, "NOTICE", 6) == 0)
			{
				maki_in_notice(serv, user, remaining);
			}
			else if (strncmp(type, "MODE", 4) == 0)
			{
				maki_in_mode(serv, user, remaining, FALSE);
			}
			else if (strncmp(type, "INVITE", 6) == 0)
			{
				maki_in_invite(serv, user, remaining, FALSE);
			}
			else if (strncmp(type, "TOPIC", 5) == 0)
			{
				maki_in_topic(serv, user, remaining, FALSE);
			}
			else
			{
				maki_debug("WARN: Unhandled message type '%s'\n", type);
			}
		}

		maki_server_remove_user(serv, maki_user_nick(user));

		g_strfreev(parts);
		g_strfreev(from);
	}
}
