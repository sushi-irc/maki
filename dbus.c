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

#include <glib/gstdio.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "maki.h"
#include "marshal.h"

enum
{
	s_action,
	s_away,
	s_away_message,
	s_back,
	s_banlist,
	s_connect,
	s_connected,
	s_ctcp,
	s_invalid_target,
	s_invite,
	s_join,
	s_kick,
	s_list,
	s_message,
	s_mode,
	s_motd,
	s_nick,
	s_notice,
	s_oper,
	s_own_ctcp,
	s_own_message,
	s_own_notice,
	s_part,
	s_query,
	s_query_ctcp,
	s_query_notice,
	s_quit,
	s_reconnect,
	s_shutdown,
	s_topic,
	s_whois,
	s_last
};

guint signals[s_last];

void maki_dbus_emit_action (gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_action], 0, time, server, nick, target, message);
}

void maki_dbus_emit_away (gint64 time, const gchar* server)
{
	g_signal_emit(maki()->bus, signals[s_away], 0, time, server);
}

void maki_dbus_emit_away_message (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_away_message], 0, time, server, nick, message);
}

void maki_dbus_emit_back (gint64 time, const gchar* server)
{
	g_signal_emit(maki()->bus, signals[s_back], 0, time, server);
}

void maki_dbus_emit_banlist (gint64 time, const gchar* server, const gchar* channel, const gchar* mask, const gchar* who, gint64 when)
{
	g_signal_emit(maki()->bus, signals[s_banlist], 0, time, server, channel, mask, who, when);
}

void maki_dbus_emit_connect (gint64 time, const gchar* server)
{
	g_signal_emit(maki()->bus, signals[s_connect], 0, time, server);
}

void maki_dbus_emit_connected (gint64 time, const gchar* server, const gchar* nick)
{
	g_signal_emit(maki()->bus, signals[s_connected], 0, time, server, nick);
}

void maki_dbus_emit_ctcp (gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_ctcp], 0, time, server, nick, target, message);
}

void maki_dbus_emit_invalid_target (gint64 time, const gchar* server, const gchar* target)
{
	g_signal_emit(maki()->bus, signals[s_invalid_target], 0, time, server, target);
}

void maki_dbus_emit_invite (gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* who)
{
	g_signal_emit(maki()->bus, signals[s_invite], 0, time, server, nick, channel, who);
}

void maki_dbus_emit_join (gint64 time, const gchar* server, const gchar* nick, const gchar* channel)
{
	g_signal_emit(maki()->bus, signals[s_join], 0, time, server, nick, channel);
}

void maki_dbus_emit_kick (gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* who, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_kick], 0, time, server, nick, channel, who, message);
}

void maki_dbus_emit_list (gint64 time, const gchar* server, const gchar* channel, gint64 users, const gchar* topic)
{
	g_signal_emit(maki()->bus, signals[s_list], 0, time, server, channel, users, topic);
}

void maki_dbus_emit_message (gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_message], 0, time, server, nick, target, message);
}

void maki_dbus_emit_mode (gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* mode, const gchar* parameter)
{
	g_signal_emit(maki()->bus, signals[s_mode], 0, time, server, nick, target, mode, parameter);
}

void maki_dbus_emit_motd (gint64 time, const gchar* server, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_motd], 0, time, server, message);
}

void maki_dbus_emit_nick (gint64 time, const gchar* server, const gchar* nick, const gchar* new_nick)
{
	g_signal_emit(maki()->bus, signals[s_nick], 0, time, server, nick, new_nick);
}

void maki_dbus_emit_notice (gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_notice], 0, time, server, nick, target, message);
}

void maki_dbus_emit_oper (gint64 time, const gchar* server)
{
	g_signal_emit(maki()->bus, signals[s_oper], 0, time, server);
}

void maki_dbus_emit_own_ctcp (gint64 time, const gchar* server, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_own_ctcp], 0, time, server, target, message);
}

void maki_dbus_emit_own_message (gint64 time, const gchar* server, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_own_message], 0, time, server, target, message);
}

void maki_dbus_emit_own_notice (gint64 time, const gchar* server, const gchar* target, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_own_notice], 0, time, server, target, message);
}

void maki_dbus_emit_part (gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_part], 0, time, server, nick, channel, message);
}

void maki_dbus_emit_query (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_query], 0, time, server, nick, message);
}

void maki_dbus_emit_query_ctcp (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_query_ctcp], 0, time, server, nick, message);
}

void maki_dbus_emit_query_notice (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_query_notice], 0, time, server, nick, message);
}

void maki_dbus_emit_quit (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_quit], 0, time, server, nick, message);
}

void maki_dbus_emit_reconnect (gint64 time, const gchar* server)
{
	g_signal_emit(maki()->bus, signals[s_reconnect], 0, time, server);
}

void maki_dbus_emit_shutdown (gint64 time)
{
	g_signal_emit(maki()->bus, signals[s_shutdown], 0, time);
}

void maki_dbus_emit_topic (gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* topic)
{
	g_signal_emit(maki()->bus, signals[s_topic], 0, time, server, nick, channel, topic);
}

void maki_dbus_emit_whois (gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(maki()->bus, signals[s_whois], 0, time, server, nick, message);
}

gboolean maki_dbus_action (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	GTimeVal time;
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		g_get_current_time(&time);

		g_strdelimit(message, "\r\n", ' ');

		maki_send_printf(serv, "PRIVMSG %s :\001ACTION %s\001", channel, message);

		maki_dbus_emit_action(time.tv_sec, server, serv->user->nick, channel, message);
	}

	return TRUE;
}

gboolean maki_dbus_away (makiDBus* self, gchar* server, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_out_away(serv, message);
		g_free(serv->user->away_message);
		serv->user->away_message = g_strdup(message);
	}

	return TRUE;
}

gboolean maki_dbus_back (makiDBus* self, gchar* server, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		sashimi_send(serv->connection, "AWAY");
	}

	return TRUE;
}

gboolean maki_dbus_channels (makiDBus* self, gchar* server, gchar*** channels, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*channels = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		gchar** channel;
		GList* list;
		GList* tmp;

		channel = *channels = g_new(gchar*, g_hash_table_size(serv->channels) + 1);
		list = g_hash_table_get_values(serv->channels);

		for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
		{
			struct maki_channel* chan = tmp->data;

			if (chan->joined)
			{
				*channel = g_strdup(chan->name);
				++channel;
			}
		}

		g_list_free(list);
		*channel = NULL;
	}

	return TRUE;
}

gboolean maki_dbus_channel_topic (makiDBus* self, gchar* server, gchar* channel, gchar** topic, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*topic = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_channel* chan;

		if ((chan = g_hash_table_lookup(serv->channels, channel)) != NULL
		    && chan->topic != NULL)
		{
			*topic = g_strdup(chan->topic);
		}
	}

	return TRUE;
}

gboolean maki_dbus_connect (makiDBus* self, gchar* server, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		/*
		 * Disconnect, because strange things happen if we call maki_server_connect() while still connected.
		 */
		maki_server_disconnect(serv, NULL);

		if (maki_server_connect(serv) != 0)
		{
			maki_reconnect_callback(serv);
		}
	}
	else
	{
		if ((serv = maki_server_new(server)) != NULL)
		{
			g_hash_table_replace(m->servers, serv->server, serv);

			if (!serv->autoconnect && maki_server_connect(serv) != 0)
			{
				maki_reconnect_callback(serv);
			}
		}
	}

	return TRUE;
}

gboolean maki_dbus_ctcp (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		GTimeVal time;

		maki_send_printf(serv, "PRIVMSG %s :\1%s\1", target, message);

		g_get_current_time(&time);
		maki_dbus_emit_own_ctcp(time.tv_sec, server, target, message);
		maki_log(serv, target, "=%s= %s", serv->user->nick, message);
	}

	return TRUE;
}

gboolean maki_dbus_ignore (makiDBus* self, gchar* server, gchar* pattern, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		gchar* path;
		GKeyFile* key_file;

		if (serv->ignores != NULL)
		{
			guint length;

			length = g_strv_length(serv->ignores);
			serv->ignores = g_renew(gchar*, serv->ignores, length + 2);
			serv->ignores[length] = g_strdup(pattern);
			serv->ignores[length + 1] = NULL;
		}
		else
		{
			serv->ignores = g_new(gchar*, 2);
			serv->ignores[0] = g_strdup(pattern);
			serv->ignores[1] = NULL;
		}

		path = g_build_filename(m->directories.servers, serv->server, NULL);
		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			g_key_file_set_string_list(key_file, "server", "ignores", (const gchar**)serv->ignores, g_strv_length(serv->ignores));
			maki_key_file_to_file(key_file, path, S_IRUSR | S_IWUSR);
		}

		g_key_file_free(key_file);
		g_free(path);
	}

	return TRUE;
}

gboolean maki_dbus_ignores (makiDBus* self, gchar* server, gchar*** ignores, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*ignores = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (serv->ignores != NULL)
		{
			*ignores = g_strdupv(serv->ignores);
		}
	}

	return TRUE;
}

gboolean maki_dbus_invite (makiDBus* self, gchar* server, gchar* channel, gchar* who, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_send_printf(serv, "INVITE %s %s", who, channel);
	}

	return TRUE;
}

gboolean maki_dbus_join (makiDBus* self, gchar* server, gchar* channel, gchar* key, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_channel* chan;

		if ((chan = g_hash_table_lookup(serv->channels, channel)) != NULL
		    && chan->key != NULL
		    && !key[0])
		{
			key = chan->key;
		}

		maki_out_join(serv, channel, key);
	}

	return TRUE;
}

gboolean maki_dbus_kick (makiDBus* self, gchar* server, gchar* channel, gchar* who, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (message[0])
		{
			maki_send_printf(serv, "KICK %s %s :%s", channel, who, message);
		}
		else
		{
			maki_send_printf(serv, "KICK %s %s", channel, who);
		}
	}

	return TRUE;
}

gboolean maki_dbus_kill (makiDBus* self, gchar* server, gchar* nick, gchar* reason, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_send_printf(serv, "KILL %s :%s", nick, reason);
	}

	return TRUE;
}

gboolean maki_dbus_list (makiDBus* self, gchar* server, gchar* channel, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (channel[0])
		{
			maki_send_printf(serv, "LIST %s", channel);
		}
		else
		{
			maki_send_printf(serv, "LIST");
		}
	}

	return TRUE;
}

gboolean maki_dbus_log (makiDBus* self, gchar* server, gchar* target, guint64 lines, gchar*** log, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*log = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		int fd;
		gchar* filename;
		gchar* path;

		filename = g_strconcat(target, ".txt", NULL);
		path = g_build_filename(m->config->directories.logs, server, filename, NULL);

		if ((fd = open(path, O_RDONLY)) != -1)
		{
			guint length = 0;
			gchar* line;
			gchar** tmp;
			GIOChannel* io_channel;

			io_channel = g_io_channel_unix_new(fd);
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
				/*
				 * The DBus specification says that strings may contain only one \0 character.
				 * Since line contains multiple \0 characters, provide a cleaned-up copy here.
				 */
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

		g_free(filename);
		g_free(path);
	}

	return TRUE;
}

gboolean maki_dbus_message (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
{
	gchar* buffer;
	GTimeVal time;
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		gchar** messages = NULL;

		g_get_current_time(&time);

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

gboolean maki_dbus_mode (makiDBus* self, gchar* server, gchar* target, gchar* mode, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (mode[0])
		{
			maki_send_printf(serv, "MODE %s %s", target, mode);
		}
		else
		{
			maki_send_printf(serv, "MODE %s", target);
		}
	}

	return TRUE;
}

gboolean maki_dbus_nick (makiDBus* self, gchar* server, gchar* nick, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_out_nick(serv, nick);
	}

	return TRUE;
}

gboolean maki_dbus_nicks (makiDBus* self, gchar* server, gchar* channel, gchar*** nicks, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*nicks = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_channel* chan;

		if ((chan = g_hash_table_lookup(serv->channels, channel)) != NULL)
		{
			gchar** nick;
			GList* list;
			GList* tmp;

			nick = *nicks = g_new(gchar*, g_hash_table_size(chan->users) + 1);
			list = g_hash_table_get_values(chan->users);

			for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
			{
				struct maki_channel_user* cuser = tmp->data;

				*nick = g_strdup(cuser->user->nick);
				++nick;
			}

			g_list_free(list);
			*nick = NULL;
		}
	}

	return TRUE;
}

gboolean maki_dbus_nickserv (makiDBus* self, gchar* server, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_out_nickserv(serv);
	}

	return TRUE;
}

gboolean maki_dbus_notice (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		GTimeVal time;

		maki_send_printf(serv, "NOTICE %s :%s", target, message);

		g_get_current_time(&time);
		maki_dbus_emit_own_notice(time.tv_sec, serv->server, target, message);
		maki_log(serv, target, "-%s- %s", serv->user->nick, message);
	}

	return TRUE;
}

gboolean maki_dbus_oper (makiDBus* self, gchar* server, gchar* name, gchar* password, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_send_printf(serv, "OPER %s %s", name, password);
	}

	return TRUE;
}

gboolean maki_dbus_own_nick (makiDBus* self, gchar* server, gchar** nick, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*nick = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		*nick = g_strdup(serv->user->nick);
	}

	return TRUE;
}

gboolean maki_dbus_part (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (message[0])
		{
			maki_send_printf(serv, "PART %s :%s", channel, message);
		}
		else
		{
			maki_send_printf(serv, "PART %s", channel);
		}
	}

	return TRUE;
}

gboolean maki_dbus_quit (makiDBus* self, gchar* server, gchar* message, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (message[0])
		{
			maki_server_disconnect(serv, message);
		}
		else
		{
			maki_server_disconnect(serv, SUSHI_QUIT_MESSAGE);
		}
	}

	return TRUE;
}

gboolean maki_dbus_raw (makiDBus* self, gchar* server, gchar* command, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		sashimi_send(serv->connection, command);
	}

	return TRUE;
}

gboolean maki_dbus_server_get (makiDBus* self, gchar* server, gchar* group, gchar* key, gchar** value, GError** error)
{
	gchar* path;
	GKeyFile* key_file;
	struct maki* m = maki();

	*value = NULL;

	path = g_build_filename(m->directories.servers, server, NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		*value = g_key_file_get_string(key_file, group, key, NULL);
	}

	g_key_file_free(key_file);
	g_free(path);

	return TRUE;
}

gboolean maki_dbus_server_list (makiDBus* self, gchar* server, gchar* group, gchar*** result, GError** error)
{
	gchar* path;
	GDir* dir;
	struct maki* m = maki();

	*result = NULL;

	if (server[0])
	{
		GKeyFile* key_file;

		path = g_build_filename(m->directories.servers, server, NULL);
		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			if (group[0])
			{
				*result = g_key_file_get_keys(key_file, group, NULL, NULL);
			}
			else
			{
				*result = g_key_file_get_groups(key_file, NULL);

			}
		}

		g_key_file_free(key_file);
		g_free(path);
	}
	else
	{
		if ((dir = g_dir_open(m->directories.servers, 0, NULL)) != NULL)
		{
			guint i = 0;
			const gchar* name;
			gchar** tmp;

			tmp = g_new(gchar*, 1);

			while ((name = g_dir_read_name(dir)) != NULL)
			{
				tmp = g_renew(gchar*, tmp, i + 2);
				tmp[i] = g_strdup(name);
				++i;
			}

			tmp[i] = NULL;
			*result = tmp;

			g_dir_close(dir);
		}
	}

	return TRUE;
}

gboolean maki_dbus_server_remove (makiDBus* self, gchar* server, gchar* group, gchar* key, GError** error)
{
	gchar* path;
	struct maki* m = maki();

	path = g_build_filename(m->directories.servers, server, NULL);

	if (group[0])
	{
		GKeyFile* key_file;

		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			if (key[0])
			{
				g_key_file_remove_key(key_file, group, key, NULL);
			}
			else
			{
				g_key_file_remove_group(key_file, group, NULL);
			}

			maki_key_file_to_file(key_file, path, S_IRUSR | S_IWUSR);
		}

		g_key_file_free(key_file);
	}
	else
	{
		g_unlink(path);
	}

	g_free(path);

	return TRUE;
}

gboolean maki_dbus_server_rename (makiDBus* self, gchar* old, gchar* new, GError** error)
{
	gchar* old_path;
	gchar* new_path;
	struct maki* m = maki();

	old_path = g_build_filename(m->directories.servers, old, NULL);
	new_path = g_build_filename(m->directories.servers, new, NULL);

	g_rename(old_path, new_path);

	g_free(old_path);
	g_free(new_path);

	return TRUE;
}

gboolean maki_dbus_server_set (makiDBus* self, gchar* server, gchar* group, gchar* key, gchar* value, GError** error)
{
	gchar* path;
	GKeyFile* key_file;
	struct maki* m = maki();

	path = g_build_filename(m->directories.servers, server, NULL);
	key_file = g_key_file_new();

	g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(key_file, group, key, value);
	maki_key_file_to_file(key_file, path, S_IRUSR | S_IWUSR);

	g_key_file_free(key_file);
	g_free(path);

	return TRUE;
}

gboolean maki_dbus_servers (makiDBus* self, gchar*** servers, GError** error)
{
	gchar** server;
	GList* list;
	GList* tmp;
	struct maki* m = maki();

	server = *servers = g_new(gchar*, g_hash_table_size(m->servers) + 1);
	list = g_hash_table_get_values(m->servers);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_server* serv = tmp->data;

		if (serv->connected)
		{
			*server = g_strdup(serv->server);
			++server;
		}
	}

	g_list_free(list);
	*server = NULL;

	return TRUE;
}

gboolean maki_dbus_shutdown (makiDBus* self, gchar* message, GError** error)
{
	GTimeVal time;
	GList* list;
	GList* tmp;
	struct maki* m = maki();

	list = g_hash_table_get_values(m->servers);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		struct maki_server* serv = tmp->data;

		if (message[0])
		{
			maki_server_disconnect(serv, message);
		}
		else
		{
			maki_server_disconnect(serv, SUSHI_QUIT_MESSAGE);
		}
	}

	g_list_free(list);

	g_get_current_time(&time);
	maki_dbus_emit_shutdown(time.tv_sec);

	maki_free(m);

	return TRUE;
}

gboolean maki_dbus_support_chantypes (makiDBus* self, gchar* server, gchar** chantypes, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*chantypes = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		*chantypes = g_strdup(serv->support.chantypes);
	}

	return TRUE;
}

gboolean maki_dbus_support_prefix (makiDBus* self, gchar* server, gchar*** prefix, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*prefix = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		*prefix = g_new(gchar*, 3);
		(*prefix)[0] = g_strdup(serv->support.prefix.modes);
		(*prefix)[1] = g_strdup(serv->support.prefix.prefixes);
		(*prefix)[2] = NULL;
	}

	return TRUE;
}

gboolean maki_dbus_topic (makiDBus* self, gchar* server, gchar* channel, gchar* topic, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (topic[0])
		{
			maki_send_printf(serv, "TOPIC %s :%s", channel, topic);
		}
		else
		{
			maki_send_printf(serv, "TOPIC %s", channel);
		}
	}

	return TRUE;
}

gboolean maki_dbus_unignore (makiDBus* self, gchar* server, gchar* pattern, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		if (serv->ignores != NULL)
		{
			gint i;
			gint j;
			guint length;
			gchar* path;
			gchar** tmp;
			GKeyFile* key_file;

			path = g_build_filename(m->directories.servers, serv->server, NULL);
			key_file = g_key_file_new();

			length = g_strv_length(serv->ignores);

			for (i = 0, j = 0; i < length; ++i)
			{
				if (strcmp(serv->ignores[i], pattern) == 0)
				{
					++j;
				}
			}

			if (length - j == 0)
			{
				g_strfreev(serv->ignores);
				serv->ignores = NULL;

				if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
				{
					g_key_file_remove_key(key_file, "server", "ignores", NULL);
					maki_key_file_to_file(key_file, path, S_IRUSR | S_IWUSR);
				}

				g_key_file_free(key_file);
				g_free(path);

				return TRUE;
			}

			tmp = g_new(gchar*, length - j + 1);

			for (i = 0, j = 0; i < length; ++i)
			{
				if (strcmp(serv->ignores[i], pattern) != 0)
				{
					tmp[j] = serv->ignores[i];
					++j;
				}
				else
				{
					g_free(serv->ignores[i]);
				}
			}

			tmp[j] = NULL;

			g_free(serv->ignores[length]);
			g_free(serv->ignores);
			serv->ignores = tmp;

			if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
			{
				g_key_file_set_string_list(key_file, "server", "ignores", (const gchar**)serv->ignores, g_strv_length(serv->ignores));
				maki_key_file_to_file(key_file, path, S_IRUSR | S_IWUSR);
			}

			g_key_file_free(key_file);
			g_free(path);
		}
	}

	return TRUE;
}

gboolean maki_dbus_user_away (makiDBus* self, gchar* server, gchar* nick, gboolean* away, GError** error)
{

	struct maki_server* serv;
	struct maki* m = maki();

	*away = FALSE;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_user* user;

		if ((user = maki_cache_insert(serv->users, nick)) != NULL)
		{
			*away = user->away;
			maki_cache_remove(serv->users, nick);
		}
	}

	return TRUE;
}

gboolean maki_dbus_user_channel_mode (makiDBus* self, gchar* server, gchar* channel, gchar* nick, gchar** mode, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*mode = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_channel* chan;

		if ((chan = g_hash_table_lookup(serv->channels, channel)) != NULL)
		{
			struct maki_channel_user* cuser;

			if ((cuser = g_hash_table_lookup(chan->users, nick)) != NULL)
			{
				gint pos;
				gint length;
				gchar tmp = '\0';

				length = strlen(serv->support.prefix.prefixes);

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

gboolean maki_dbus_user_channel_prefix (makiDBus* self, gchar* server, gchar* channel, gchar* nick, gchar** prefix, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	*prefix = NULL;

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		struct maki_channel* chan;

		if ((chan = g_hash_table_lookup(serv->channels, channel)) != NULL)
		{
			struct maki_channel_user* cuser;

			if ((cuser = g_hash_table_lookup(chan->users, nick)) != NULL)
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

gboolean maki_dbus_whois (makiDBus* self, gchar* server, gchar* mask, GError** error)
{
	struct maki_server* serv;
	struct maki* m = maki();

	if ((serv = g_hash_table_lookup(m->servers, server)) != NULL)
	{
		maki_send_printf(serv, "WHOIS %s", mask);
	}

	return TRUE;
}

#include "dbus_glue.h"

G_DEFINE_TYPE(makiDBus, maki_dbus, G_TYPE_OBJECT)

static void maki_dbus_init (makiDBus* obj)
{
	DBusGProxy* proxy;
	GError* error = NULL;
	guint request_name_result;

	if ((obj->bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error)) != NULL)
	{
		dbus_g_connection_register_g_object(obj->bus, "/de/ikkoku/sushi", G_OBJECT(obj));

		if ((proxy = dbus_g_proxy_new_for_name(obj->bus, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS)) != NULL)
		{
			if (!org_freedesktop_DBus_request_name(proxy, "de.ikkoku.sushi", 0, &request_name_result, &error))
			{
				g_error_free(error);
			}

			g_object_unref(proxy);
		}
	}
	else
	{
		g_error_free(error);
	}
}

static void maki_dbus_class_init (makiDBusClass* klass)
{
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
		             G_TYPE_NONE, 3,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_ctcp] =
		g_signal_new("ctcp",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_invalid_target] =
		g_signal_new("invalid_target",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING,
		             G_TYPE_NONE, 3,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
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
	signals[s_nick] =
		g_signal_new("nick",
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
	signals[s_own_ctcp] =
		g_signal_new("own_ctcp",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_own_message] =
		g_signal_new("own_message",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_own_notice] =
		g_signal_new("own_notice",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_part] =
		g_signal_new("part",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_query] =
		g_signal_new("query",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_query_ctcp] =
		g_signal_new("query_ctcp",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_query_notice] =
		g_signal_new("query_notice",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_quit] =
		g_signal_new("quit",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_reconnect] =
		g_signal_new("reconnect",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             maki_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
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