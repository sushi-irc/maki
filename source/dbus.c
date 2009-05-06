/*
 * Copyright (c) 2008-2009 Michael Kuhn
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

#include <fcntl.h>
#include <string.h>

#include "marshal.h"

struct maki_dbus
{
	GObject parent;
	DBusGConnection* bus;
};

struct maki_dbus_class
{
	GObjectClass parent;
};

enum
{
	s_action,
	s_away,
	s_away_message,
	s_back,
	s_banlist,
	s_cannot_join,
	s_connect,
	s_connected,
	s_ctcp,
	s_invite,
	s_join,
	s_kick,
	s_list,
	s_message,
	s_mode,
	s_motd,
	s_names,
	s_nick,
	s_no_such,
	s_notice,
	s_oper,
	s_part,
	s_quit,
	s_shutdown,
	s_topic,
	s_whois,
	s_last
};

static guint signals[s_last];

void maki_dbus_emit_action (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(dbus, signals[s_action], 0, timestamp, server, nick, target, message);
}

void maki_dbus_emit_away (gint64 timestamp, const gchar* server)
{
	g_signal_emit(dbus, signals[s_away], 0, timestamp, server);
}

void maki_dbus_emit_away_message (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(dbus, signals[s_away_message], 0, timestamp, server, nick, message);
}

void maki_dbus_emit_back (gint64 timestamp, const gchar* server)
{
	g_signal_emit(dbus, signals[s_back], 0, timestamp, server);
}

void maki_dbus_emit_banlist (gint64 timestamp, const gchar* server, const gchar* channel, const gchar* mask, const gchar* who, gint64 when)
{
	g_signal_emit(dbus, signals[s_banlist], 0, timestamp, server, channel, mask, who, when);
}

void maki_dbus_emit_cannot_join (gint64 timestamp, const gchar* server, const gchar* channel, const gchar* reason)
{
	g_signal_emit(dbus, signals[s_cannot_join], 0, timestamp, server, channel, reason);
}

void maki_dbus_emit_connect (gint64 timestamp, const gchar* server)
{
	g_signal_emit(dbus, signals[s_connect], 0, timestamp, server);
}

void maki_dbus_emit_connected (gint64 timestamp, const gchar* server)
{
	g_signal_emit(dbus, signals[s_connected], 0, timestamp, server);
}

void maki_dbus_emit_ctcp (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(dbus, signals[s_ctcp], 0, timestamp, server, nick, target, message);
}

void maki_dbus_emit_invite (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* channel, const gchar* who)
{
	g_signal_emit(dbus, signals[s_invite], 0, timestamp, server, nick, channel, who);
}

void maki_dbus_emit_join (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* channel)
{
	g_signal_emit(dbus, signals[s_join], 0, timestamp, server, nick, channel);
}

void maki_dbus_emit_kick (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* channel, const gchar* who, const gchar* message)
{
	g_signal_emit(dbus, signals[s_kick], 0, timestamp, server, nick, channel, who, message);
}

void maki_dbus_emit_list (gint64 timestamp, const gchar* server, const gchar* channel, gint64 users, const gchar* topic)
{
	g_signal_emit(dbus, signals[s_list], 0, timestamp, server, channel, users, topic);
}

void maki_dbus_emit_message (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(dbus, signals[s_message], 0, timestamp, server, nick, target, message);
}

void maki_dbus_emit_mode (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* target, const gchar* mode, const gchar* parameter)
{
	g_signal_emit(dbus, signals[s_mode], 0, timestamp, server, nick, target, mode, parameter);
}

void maki_dbus_emit_motd (gint64 timestamp, const gchar* server, const gchar* message)
{
	g_signal_emit(dbus, signals[s_motd], 0, timestamp, server, message);
}

void maki_dbus_emit_names (gint64 timestamp, const gchar* server, const gchar* channel, gchar** nicks, gchar** prefixes)
{
	g_signal_emit(dbus, signals[s_names], 0, timestamp, server, channel, nicks, prefixes);
}

void maki_dbus_emit_nick (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* new_nick)
{
	g_signal_emit(dbus, signals[s_nick], 0, timestamp, server, nick, new_nick);
}

void maki_dbus_emit_no_such (gint64 timestamp, const gchar* server, const gchar* target, const gchar* type)
{
	g_signal_emit(dbus, signals[s_no_such], 0, timestamp, server, target, type);
}

void maki_dbus_emit_notice (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(dbus, signals[s_notice], 0, timestamp, server, nick, target, message);
}

void maki_dbus_emit_oper (gint64 timestamp, const gchar* server)
{
	g_signal_emit(dbus, signals[s_oper], 0, timestamp, server);
}

void maki_dbus_emit_part (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* channel, const gchar* message)
{
	g_signal_emit(dbus, signals[s_part], 0, timestamp, server, nick, channel, message);
}

void maki_dbus_emit_quit (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(dbus, signals[s_quit], 0, timestamp, server, nick, message);
}

void maki_dbus_emit_shutdown (gint64 timestamp)
{
	g_signal_emit(dbus, signals[s_shutdown], 0, timestamp);
}

void maki_dbus_emit_topic (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* channel, const gchar* topic)
{
	g_signal_emit(dbus, signals[s_topic], 0, timestamp, server, nick, channel, topic);
}

void maki_dbus_emit_whois (gint64 timestamp, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(dbus, signals[s_whois], 0, timestamp, server, nick, message);
}

static gboolean maki_dbus_action (makiDBus* self, const gchar* server, const gchar* channel, const gchar* message, GError** error)
{
	GTimeVal timeval;
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar* tmp;

		g_get_current_time(&timeval);

		tmp = g_strdup(message);
		g_strdelimit(tmp, "\r\n", ' ');

		maki_server_send_printf(serv, "PRIVMSG %s :\001ACTION %s\001", channel, tmp);

		maki_log(serv, channel, "%s %s", maki_user_nick(serv->user), tmp);

		maki_dbus_emit_action(timeval.tv_sec, server, maki_user_from(serv->user), channel, tmp);

		g_free(tmp);
	}

	return TRUE;
}

static gboolean maki_dbus_away (makiDBus* self, const gchar* server, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_out_away(serv, message);
		maki_user_set_away_message(serv->user, message);
	}

	return TRUE;
}

static gboolean maki_dbus_back (makiDBus* self, const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send(serv, "AWAY");
	}

	return TRUE;
}

static gboolean maki_dbus_channel_nicks (makiDBus* self, const gchar* server, const gchar* channel, gchar*** nicks, gchar*** prefixes, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*nicks = NULL;
	*prefixes = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			gchar** nick;
			gchar** prefix;
			gchar prefix_str[2];
			gsize length;
			GHashTableIter iter;
			gpointer key, value;

			nick = *nicks = g_new(gchar*, maki_channel_users_count(chan) + 1);
			prefix = *prefixes = g_new(gchar*, maki_channel_users_count(chan) + 1);
			maki_channel_users_iter(chan, &iter);

			length = strlen(serv->support.prefix.prefixes);
			prefix_str[1] = '\0';

			while (g_hash_table_iter_next(&iter, &key, &value))
			{
				guint pos;
				makiChannelUser* cuser = value;

				*nick = g_strdup(maki_user_nick(cuser->user));
				nick++;

				prefix_str[0] = '\0';

				for (pos = 0; pos < length; pos++)
				{
					if (cuser->prefix & (1 << pos))
					{
						prefix_str[0] = serv->support.prefix.prefixes[pos];
						break;
					}
				}

				*prefix = g_strdup(prefix_str);
				prefix++;
			}

			*nick = NULL;
			*prefix = NULL;
		}
	}

	return TRUE;
}

static gboolean maki_dbus_channels (makiDBus* self, const gchar* server, gchar*** channels, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*channels = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** channel;
		GHashTableIter iter;
		gpointer key, value;

		channel = *channels = g_new(gchar*, maki_server_channels_count(serv) + 1);
		maki_server_channels_iter(serv, &iter);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			const gchar* chan_name = key;
			makiChannel* chan = value;

			if (maki_channel_joined(chan))
			{
				*channel = g_strdup(chan_name);
				++channel;
			}
		}

		*channel = NULL;
	}

	return TRUE;
}

static gboolean maki_dbus_config_get (makiDBus* self, const gchar* group, const gchar* key, gchar** value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	*value = maki_instance_config_get_string(inst, group, key);

	return TRUE;
}

static gboolean maki_dbus_config_set (makiDBus* self, const gchar* group, const gchar* key, const gchar* value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_config_set_string(inst, group, key, value);

	return TRUE;
}

static gboolean maki_dbus_connect (makiDBus* self, const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		/* Disconnect, because strange things happen if we call maki_server_connect() while still connected. */
		maki_server_disconnect(serv, "");
		maki_server_connect(serv);
	}
	else
	{
		if ((serv = maki_server_new(inst, server)) != NULL)
		{
			maki_instance_add_server(inst, maki_server_name(serv), serv);

			if (!maki_server_autoconnect(serv))
			{
				maki_server_connect(serv);
			}
		}
	}

	return TRUE;
}

static gboolean maki_dbus_ctcp (makiDBus* self, const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		GTimeVal timeval;

		maki_server_send_printf(serv, "PRIVMSG %s :\001%s\001", target, message);

		g_get_current_time(&timeval);
		maki_dbus_emit_ctcp(timeval.tv_sec, server, maki_user_from(serv->user), target, message);
		maki_log(serv, target, "=%s= %s", maki_user_nick(serv->user), message);
	}

	return TRUE;
}

static gboolean maki_dbus_dcc_send (makiDBus* self, const gchar* server, const gchar* target, const gchar* path, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiDCCSend* dcc;
		makiUser* user;

		user = i_cache_insert(serv->users, target);
		dcc = maki_dcc_send_new_out(serv, user, path);
		i_cache_remove(serv->users, maki_user_nick(user));
	}

	return TRUE;
}

static gboolean maki_dbus_ignore (makiDBus* self, const gchar* server, const gchar* pattern, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** ignores;

		ignores = maki_server_config_get_string_list(serv, "server", "ignores");

		if (ignores != NULL)
		{
			guint length;

			length = g_strv_length(ignores);
			ignores = g_renew(gchar*, ignores, length + 2);
			ignores[length] = g_strdup(pattern);
			ignores[length + 1] = NULL;
		}
		else
		{
			ignores = g_new(gchar*, 2);
			ignores[0] = g_strdup(pattern);
			ignores[1] = NULL;
		}

		maki_server_config_set_string_list(serv, "server", "ignores", ignores);

		g_strfreev(ignores);
	}

	return TRUE;
}

static gboolean maki_dbus_ignores (makiDBus* self, const gchar* server, gchar*** ignores, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*ignores = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*ignores = maki_server_config_get_string_list(serv, "server", "ignores");
	}

	return TRUE;
}

static gboolean maki_dbus_invite (makiDBus* self, const gchar* server, const gchar* channel, const gchar* who, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "INVITE %s %s", who, channel);
	}

	return TRUE;
}

static gboolean maki_dbus_join (makiDBus* self, const gchar* server, const gchar* channel, const gchar* key, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			gchar* channel_key;

			channel_key = maki_channel_key(chan);

			if (channel_key != NULL
			    && key[0] == '\0')
			{
				/* The channel has a key set and none was supplied. */
				maki_out_join(serv, channel, channel_key);
			}
			else
			{
				maki_out_join(serv, channel, key);
			}

			g_free(channel_key);
		}
		else
		{
			maki_out_join(serv, channel, key);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_kick (makiDBus* self, const gchar* server, const gchar* channel, const gchar* who, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (message[0])
		{
			maki_server_send_printf(serv, "KICK %s %s :%s", channel, who, message);
		}
		else
		{
			maki_server_send_printf(serv, "KICK %s %s", channel, who);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_list (makiDBus* self, const gchar* server, const gchar* channel, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (channel[0])
		{
			maki_server_send_printf(serv, "LIST %s", channel);
		}
		else
		{
			maki_server_send_printf(serv, "LIST");
		}
	}

	return TRUE;
}

static gboolean maki_dbus_log (makiDBus* self, const gchar* server, const gchar* target, guint64 lines, gchar*** log, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*log = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar* file;
		gchar* file_format;
		gchar* file_tmp;
		gchar* filename;
		gchar* path;
		gchar* logs_dir;
		GIOChannel* io_channel;

		file_format = maki_instance_config_get_string(inst, "logging", "format");
		file_tmp = i_strreplace(file_format, "$n", target, 0);
		file = i_get_current_time_string(file_tmp);

		filename = g_strconcat(file, ".txt", NULL);
		logs_dir = maki_instance_config_get_string(inst, "directories", "logs");
		path = g_build_filename(logs_dir, server, filename, NULL);

		g_free(file_format);
		g_free(file_tmp);
		g_free(file);

		g_free(filename);
		g_free(logs_dir);

		if ((io_channel = g_io_channel_new_file(path, "r", NULL)) != NULL)
		{
			guint length = 0;
			gchar* line;
			gchar** tmp;

			g_io_channel_set_close_on_unref(io_channel, TRUE);
			g_io_channel_set_encoding(io_channel, NULL, NULL);
			g_io_channel_seek_position(io_channel, -10 * 1024, G_SEEK_END, NULL);

			/* Discard the first line. */
			g_io_channel_read_line(io_channel, &line, NULL, NULL, NULL);
			g_free(line);

			tmp = g_new(gchar*, length + 1);
			tmp[length] = NULL;

			while (g_io_channel_read_line(io_channel, &line, NULL, NULL, NULL) == G_IO_STATUS_NORMAL)
			{
				g_strchomp(line);

				tmp = g_renew(gchar*, tmp, length + 2);
				/* The DBus specification says that strings may contain only one \0 character.
				 * Since line contains multiple \0 characters, provide a cleaned-up copy here. */
				tmp[length] = g_strdup(line);
				tmp[length + 1] = NULL;

				g_free(line);

				length++;
			}

			if (length > lines)
			{
				guint i;
				guint j;
				gchar** trunc;

				trunc = g_new(gchar*, lines + 1);

				for (i = 0; i < length - lines; i++)
				{
					g_free(tmp[i]);
				}

				for (i = length - lines, j = 0; i < length; i++, j++)
				{
					trunc[j] = tmp[i];
				}

				g_free(tmp[length]);
				g_free(tmp);

				trunc[lines] = NULL;

				tmp = trunc;
			}

			*log = tmp;

			g_io_channel_shutdown(io_channel, FALSE, NULL);
			g_io_channel_unref(io_channel);
		}

		g_free(path);
	}

	return TRUE;
}

static gboolean maki_dbus_message (makiDBus* self, const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	GTimeVal timeval;
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		const gchar* buffer;
		gchar** messages = NULL;

		g_get_current_time(&timeval);

		for (buffer = message; *buffer != '\0'; ++buffer)
		{
			if (*buffer == '\r' || *buffer == '\n')
			{
				messages = g_strsplit(message, "\n", 0);
				break;
			}
		}

		if (messages == NULL)
		{
			maki_out_privmsg(serv, target, message, FALSE);
		}
		else
		{
			gchar** tmp;

			for (tmp = messages; *tmp != NULL; ++tmp)
			{
				g_strchomp(*tmp);

				if ((*tmp)[0])
				{
					maki_out_privmsg(serv, target, *tmp, TRUE);
				}
			}

			g_strfreev(messages);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_mode (makiDBus* self, const gchar* server, const gchar* target, const gchar* mode, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (mode[0])
		{
			maki_server_send_printf(serv, "MODE %s %s", target, mode);
		}
		else
		{
			maki_server_send_printf(serv, "MODE %s", target);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_names (makiDBus* self, const gchar* server, const gchar* channel, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (channel[0] != '\0')
		{
			maki_server_send_printf(serv, "NAMES %s", channel);
		}
		else
		{
			maki_server_send(serv, "NAMES");
		}
	}

	return TRUE;
}

static gboolean maki_dbus_nick (makiDBus* self, const gchar* server, const gchar* nick, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (nick[0] != '\0')
		{
			maki_out_nick(serv, nick);
		}
		else
		{
			GTimeVal timeval;

			g_get_current_time(&timeval);
			maki_dbus_emit_nick(timeval.tv_sec, maki_server_name(serv), "", maki_user_nick(serv->user));
		}
	}

	return TRUE;
}


static gboolean maki_dbus_nickserv (makiDBus* self, const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_out_nickserv(serv);
	}

	return TRUE;
}

static gboolean maki_dbus_notice (makiDBus* self, const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		GTimeVal timeval;

		maki_server_send_printf(serv, "NOTICE %s :%s", target, message);

		g_get_current_time(&timeval);
		maki_dbus_emit_notice(timeval.tv_sec, maki_server_name(serv), maki_user_from(serv->user), target, message);
		maki_log(serv, target, "-%s- %s", maki_user_nick(serv->user), message);
	}

	return TRUE;
}

static gboolean maki_dbus_oper (makiDBus* self, const gchar* server, const gchar* name, const gchar* password, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "OPER %s %s", name, password);
	}

	return TRUE;
}

static gboolean maki_dbus_part (makiDBus* self, const gchar* server, const gchar* channel, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (message[0])
		{
			maki_server_send_printf(serv, "PART %s :%s", channel, message);
		}
		else
		{
			maki_server_send_printf(serv, "PART %s", channel);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_quit (makiDBus* self, const gchar* server, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_disconnect(serv, message);
	}

	return TRUE;
}

static gboolean maki_dbus_raw (makiDBus* self, const gchar* server, const gchar* command, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send(serv, command);
	}

	return TRUE;
}

static gboolean maki_dbus_server_get (makiDBus* self, const gchar* server, const gchar* group, const gchar* key, gchar** value, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*value = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*value = maki_server_config_get_string(serv, group, key);
	}

	return TRUE;
}

static gboolean maki_dbus_server_get_list (makiDBus* self, const gchar* server, const gchar* group, const gchar* key, gchar*** list, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*list = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*list = maki_server_config_get_string_list(serv, group, key);
	}

	return TRUE;
}

static gboolean maki_dbus_server_list (makiDBus* self, const gchar* server, const gchar* group, gchar*** result, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*result = NULL;

	if (server[0])
	{
		if ((serv = maki_instance_get_server(inst, server)) != NULL)
		{
			if (group[0])
			{
				*result = maki_server_config_get_keys(serv, group);
			}
			else
			{
				*result = maki_server_config_get_groups(serv);

			}
		}
	}
	else
	{
		GHashTableIter iter;
		gpointer key, value;
		guint i = 0;
		gchar** tmp;

		tmp = g_new(gchar*, maki_instance_servers_count(inst) + 1);

		maki_instance_servers_iter(inst, &iter);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			const gchar* name = key;

			tmp[i] = g_strdup(name);
			i++;
		}

		tmp[i] = NULL;
		*result = tmp;
	}

	return TRUE;
}

static gboolean maki_dbus_server_remove (makiDBus* self, const gchar* server, const gchar* group, const gchar* key, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (group[0])
		{
			if (key[0])
			{
				maki_server_config_remove_key(serv, group, key);
			}
			else
			{
				maki_server_config_remove_group(serv, group);
			}
		}
		else
		{
			gchar* path;

			maki_instance_remove_server(inst, server);

			path = g_build_filename(maki_instance_directory(inst, "servers"), server, NULL);
			g_unlink(path);
			g_free(path);
		}
	}

	return TRUE;
}

/* FIXME */
static gboolean maki_dbus_server_rename (makiDBus* self, const gchar* old, const gchar* new, GError** error)
{
	gchar* old_path;
	gchar* new_path;
	makiInstance* inst = maki_instance_get_default();

	old_path = g_build_filename(maki_instance_directory(inst, "servers"), old, NULL);
	new_path = g_build_filename(maki_instance_directory(inst, "servers"), new, NULL);

	g_rename(old_path, new_path);

	g_free(old_path);
	g_free(new_path);

	return TRUE;
}

static gboolean maki_dbus_server_set (makiDBus* self, const gchar* server, const gchar* group, const gchar* key, const gchar* value, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_config_set_string(serv, group, key, value);
	}
	else
	{
		if ((serv = maki_server_new(inst, server)) != NULL)
		{
			maki_server_config_set_string(serv, group, key, value);
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_server_set_list (makiDBus* self, const gchar* server, const gchar* group, const gchar* key, gchar** list, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_config_set_string_list(serv, group, key, list);
	}
	else
	{
		if ((serv = maki_server_new(inst, server)) != NULL)
		{
			maki_server_config_set_string_list(serv, group, key, list);
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_servers (makiDBus* self, gchar*** servers, GError** error)
{
	gchar** server;
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	server = *servers = g_new(gchar*, maki_instance_servers_count(inst) + 1);
	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		if (serv->connected)
		{
			*server = g_strdup(maki_server_name(serv));
			++server;
		}
	}

	*server = NULL;

	return TRUE;
}

static gboolean maki_dbus_shutdown (makiDBus* self, const gchar* message, GError** error)
{
	GTimeVal timeval;
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, message);
	}

	g_get_current_time(&timeval);
	maki_dbus_emit_shutdown(timeval.tv_sec);

	g_main_loop_quit(main_loop);

	return TRUE;
}

static gboolean maki_dbus_support_chantypes (makiDBus* self, const gchar* server, gchar** chantypes, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*chantypes = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*chantypes = g_strdup(serv->support.chantypes);
	}

	return TRUE;
}

static gboolean maki_dbus_support_prefix (makiDBus* self, const gchar* server, gchar*** prefix, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*prefix = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*prefix = g_new(gchar*, 3);
		(*prefix)[0] = g_strdup(serv->support.prefix.modes);
		(*prefix)[1] = g_strdup(serv->support.prefix.prefixes);
		(*prefix)[2] = NULL;
	}

	return TRUE;
}

static gboolean maki_dbus_topic (makiDBus* self, const gchar* server, const gchar* channel, const gchar* topic, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (topic[0])
		{
			maki_server_send_printf(serv, "TOPIC %s :%s", channel, topic);
		}
		else
		{
			makiChannel* chan;

			if ((chan = maki_server_get_channel(serv, channel)) != NULL
			    && maki_channel_topic(chan) != NULL)
			{
				GTimeVal timeval;

				g_get_current_time(&timeval);
				maki_dbus_emit_topic(timeval.tv_sec, maki_server_name(serv), "", channel, maki_channel_topic(chan));
			}
			else
			{
				maki_server_send_printf(serv, "TOPIC %s", channel);
			}
		}
	}

	return TRUE;
}

static gboolean maki_dbus_unignore (makiDBus* self, const gchar* server, const gchar* pattern, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** ignores;

		ignores = maki_server_config_get_string_list(serv, "server", "ignores");

		if (ignores != NULL)
		{
			guint i;
			guint j;
			guint length;
			gchar** tmp;

			length = g_strv_length(ignores);
			j = 0;

			for (i = 0; i < length; ++i)
			{
				if (strcmp(ignores[i], pattern) == 0)
				{
					++j;
				}
			}

			if (length - j == 0)
			{
				maki_server_config_remove_key(serv, "server", "ignores");

				g_strfreev(ignores);

				return TRUE;
			}

			tmp = g_new(gchar*, length - j + 1);
			j = 0;

			for (i = 0; i < length; ++i)
			{
				if (strcmp(ignores[i], pattern) != 0)
				{
					tmp[j] = g_strdup(ignores[i]);
					++j;
				}
			}

			tmp[j] = NULL;

			maki_server_config_set_string_list(serv, "server", "ignores", tmp);

			g_strfreev(tmp);
		}

		g_strfreev(ignores);
	}

	return TRUE;
}

static gboolean maki_dbus_user_away (makiDBus* self, const gchar* server, const gchar* nick, gboolean* away, GError** error)
{

	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*away = FALSE;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiUser* user;

		if ((user = i_cache_lookup(serv->users, nick)) != NULL)
		{
			*away = maki_user_away(user);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_user_channel_mode (makiDBus* self, const gchar* server, const gchar* channel, const gchar* nick, gchar** mode, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*mode = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			makiChannelUser* cuser;

			if ((cuser = maki_channel_get_user(chan, nick)) != NULL)
			{
				gint pos;
				gint length;
				gchar tmp = '\0';

				length = strlen(serv->support.prefix.modes);

				for (pos = 0; pos < length; pos++)
				{
					if (cuser->prefix & (1 << pos))
					{
						tmp = serv->support.prefix.modes[pos];
						break;
					}
				}

				if (tmp)
				{
					*mode = g_new(gchar, 2);
					(*mode)[0] = tmp;
					(*mode)[1] = '\0';
				}
				else
				{
					*mode = g_new(gchar, 1);
					(*mode)[0] = '\0';
				}
			}
		}
	}

	return TRUE;
}

static gboolean maki_dbus_user_channel_prefix (makiDBus* self, const gchar* server, const gchar* channel, const gchar* nick, gchar** prefix, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*prefix = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			makiChannelUser* cuser;

			if ((cuser = maki_channel_get_user(chan, nick)) != NULL)
			{
				gint pos;
				gint length;
				gchar tmp = '\0';

				length = strlen(serv->support.prefix.prefixes);

				for (pos = 0; pos < length; pos++)
				{
					if (cuser->prefix & (1 << pos))
					{
						tmp = serv->support.prefix.prefixes[pos];
						break;
					}
				}

				if (tmp)
				{
					*prefix = g_new(gchar, 2);
					(*prefix)[0] = tmp;
					(*prefix)[1] = '\0';
				}
				else
				{
					*prefix = g_new(gchar, 1);
					(*prefix)[0] = '\0';
				}
			}
		}
	}

	return TRUE;
}

static gboolean maki_dbus_version (makiDBus* self, GArray** version, GError** error)
{
	gchar** p;
	guint p_len;
	guint i;

	*version = g_array_new(FALSE, FALSE, sizeof(guint64));

	p = g_strsplit(SUSHI_VERSION, ".", 0);
	p_len = g_strv_length(p);

	for (i = 0; i < p_len; i++)
	{
		guint64 v;

		v = g_ascii_strtoull(p[i], NULL, 10);
		*version = g_array_append_val(*version, v);
	}

	g_strfreev(p);

	return TRUE;
}

static gboolean maki_dbus_who (makiDBus* self, const gchar* server, const gchar* mask, gboolean operators_only, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (operators_only)
		{
			maki_server_send_printf(serv, "WHO %s o", mask);
		}
		else
		{
			maki_server_send_printf(serv, "WHO %s", mask);
		}
	}

	return TRUE;
}

static gboolean maki_dbus_whois (makiDBus* self, const gchar* server, const gchar* mask, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "WHOIS %s", mask);
	}

	return TRUE;
}

#include "dbus_glue.h"

G_DEFINE_TYPE(makiDBus, maki_dbus, G_TYPE_OBJECT)

static void maki_dbus_init (makiDBus* self)
{
	DBusGProxy* proxy;
	GError* error = NULL;
	guint request_name_result;

	if ((self->bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error)) != NULL)
	{
		dbus_g_connection_register_g_object(self->bus, "/de/ikkoku/sushi", G_OBJECT(self));

		if ((proxy = dbus_g_proxy_new_for_name(self->bus, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS)) != NULL)
		{
			if (dbus_g_proxy_call(proxy, "RequestName", &error, G_TYPE_STRING, "de.ikkoku.sushi", G_TYPE_UINT, 0, G_TYPE_INVALID, G_TYPE_UINT, &request_name_result, G_TYPE_INVALID))
			{
				if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
				{
					dbus_g_connection_unref(self->bus);
					self->bus = NULL;
				}
			}
			else
			{
				g_error_free(error);
				dbus_g_connection_unref(self->bus);
				self->bus = NULL;
			}

			g_object_unref(proxy);
		}
		else
		{
			dbus_g_connection_unref(self->bus);
			self->bus = NULL;
		}
	}
	else
	{
		g_error_free(error);
	}
}

static void maki_dbus_finalize (GObject* object)
{
	makiDBus* self = MAKI_DBUS(object);

	/* FIXME ReleaseName? */

	if (self->bus != NULL)
	{
		dbus_g_connection_unref(self->bus);
		self->bus = NULL;
	}
}

gboolean maki_dbus_connected (makiDBus* self)
{
	return (self->bus != NULL);
}

static void maki_dbus_class_init (makiDBusClass* klass)
{
	GObjectClass* object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = maki_dbus_finalize;

	dbus_g_object_type_install_info(MAKI_DBUS_TYPE, &dbus_glib_maki_dbus_object_info);

	signals[s_action] =
		g_signal_new("action",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_away] =
		g_signal_new("away",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_away_message] =
		g_signal_new("away_message",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_back] =
		g_signal_new("back",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_banlist] =
		g_signal_new("banlist",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING_INT64,
		             G_TYPE_NONE, 6,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT64);
	signals[s_cannot_join] =
		g_signal_new("cannot_join",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_connect] =
		g_signal_new("connect",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_connected] =
		g_signal_new("connected",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_ctcp] =
		g_signal_new("ctcp",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_invite] =
		g_signal_new("invite",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_join] =
		g_signal_new("join",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_kick] =
		g_signal_new("kick",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 6,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_list] =
		g_signal_new("list",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_INT64_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT64, G_TYPE_STRING);
	signals[s_message] =
		g_signal_new("message",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_mode] =
		g_signal_new("mode",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 6,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_motd] =
		g_signal_new("motd",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING,
		             G_TYPE_NONE, 3,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_names] =
		g_signal_new("names",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRV, G_TYPE_STRV);
	signals[s_nick] =
		g_signal_new("nick",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_no_such] =
		g_signal_new("no_such",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_notice] =
		g_signal_new("notice",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_oper] =
		g_signal_new("oper",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_part] =
		g_signal_new("part",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_quit] =
		g_signal_new("quit",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_shutdown] =
		g_signal_new("shutdown",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64,
		             G_TYPE_NONE, 1,
		             G_TYPE_INT64);
	signals[s_topic] =
		g_signal_new("topic",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_whois] =
		g_signal_new("whois",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}
