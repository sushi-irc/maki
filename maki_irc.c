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
#include <unistd.h>

#include "maki.h"
#include "maki_irc.h"
#include "maki_misc.h"

/**
 * This function gets called after a successful login.
 * It joins all configured channels.
 */
gboolean maki_join (gpointer data)
{
	GHashTableIter iter;
	gpointer key;
	gpointer value;
	struct maki_connection* m_conn = data;

	g_hash_table_iter_init(&iter, m_conn->channels);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		gchar* buffer;
		struct maki_channel* m_chan = value;

		if (m_chan->key != NULL)
		{
			buffer = g_strdup_printf("JOIN %s %s", m_chan->name, m_chan->key);
		}
		else
		{
			buffer = g_strdup_printf("JOIN %s", m_chan->name);
		}

		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return FALSE;
}

/**
 * This function is run in its own thread.
 * It receives and handles all messages from sashimi.
 */
gpointer maki_irc_parser (gpointer data)
{
	gchar* message;
	GTimeVal time;
	struct maki* maki = data;
	struct maki_connection* m_conn;
	struct sashimi_message* s_msg;

	for (;;)
	{
		g_get_current_time(&time);
		g_time_val_add(&time, 1000000);

		s_msg = g_async_queue_timed_pop(maki->message_queue, &time);

		if (s_msg == NULL)
		{
			if (maki->threads.terminate)
			{
				g_thread_exit(NULL);
			}

			continue;
		}

		m_conn = s_msg->data;
		message = s_msg->message;

		g_free(s_msg);

		if (!g_utf8_validate(message, -1, NULL))
		{
			gchar* tmp;

			if ((tmp = g_convert_with_fallback(message, -1, "UTF-8", "ISO-8859-1", "?", NULL, NULL, NULL)) != NULL)
			{
				g_free(message);
				message = tmp;
			}
			else
			{
				g_free(message);

				continue;
			}
		}

		g_get_current_time(&time);

		g_print("%ld %s %s\n", time.tv_sec, m_conn->server, message);

		if (message[0] == ':')
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

			if (from && from_nick && type)
			{
				if (g_ascii_strncasecmp(type, "PRIVMSG", 7) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 2);
					gchar* to = tmp[0];
					gchar* msg = maki_remove_colon(tmp[1]);

					if (msg && msg[0] == '\1')
					{
						gchar** magic;

						magic = g_strsplit(msg + 1, " ", 2);

						if (g_ascii_strncasecmp(magic[0], "ACTION", 7) == 0 && magic[1])
						{
							if (strlen(magic[1]) > 1 && magic[1][strlen(magic[1]) - 1] == '\1')
							{
								magic[1][strlen(magic[1]) - 1] = '\0';
							}

							maki_dbus_emit_action(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick, magic[1]);
						}

						g_strfreev(magic);
					}
					else if (msg)
					{
						maki_dbus_emit_message(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick, msg);
					}

					g_strfreev(tmp);
				}
				else if (g_ascii_strncasecmp(type, "JOIN", 4) == 0 && remaining)
				{
					gchar* channel = (remaining[0] == ':') ? remaining + 1 : remaining;
					struct maki_channel* m_chan;

					g_strdelimit(channel, " ", '\0');

					if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
					{
						struct maki_user* m_user;

						m_user = g_new(struct maki_user, 1);
						m_user->nick = g_strdup(from_nick);
						g_hash_table_replace(m_chan->users, m_user->nick, m_user);
					}

					if (g_ascii_strcasecmp(from_nick, m_conn->nick) == 0)
					{
						if (m_chan != NULL)
						{
							m_chan->joined = TRUE;
						}
						else
						{
							m_chan = g_new(struct maki_channel, 1);
							m_chan->name = g_strdup(channel);
							m_chan->joined = TRUE;
							m_chan->key = NULL;
							m_chan->users = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_user_destroy);

							g_hash_table_replace(m_conn->channels, m_chan->name, m_chan);
						}
					}

					maki_dbus_emit_join(m_conn->maki->bus, time.tv_sec, m_conn->server, channel, from_nick);
				}
				else if (g_ascii_strncasecmp(type, "PART", 4) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 2);
					gchar* channel = tmp[0];
					gchar* msg = maki_remove_colon(tmp[1]);
					struct maki_channel* m_chan;

					if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
					{
						g_hash_table_remove(m_chan->users, from_nick);

						if (g_ascii_strcasecmp(from_nick, m_conn->nick) == 0)
						{
							m_chan->joined = FALSE;
						}
					}

					if (msg)
					{
						maki_dbus_emit_part(m_conn->maki->bus, time.tv_sec, m_conn->server, channel, from_nick, msg);
					}
					else
					{
						maki_dbus_emit_part(m_conn->maki->bus, time.tv_sec, m_conn->server, channel, from_nick, "");
					}

					g_strfreev(tmp);
				}
				else if (g_ascii_strncasecmp(type, "QUIT", 4) == 0)
				{
					if (remaining)
					{
						maki_dbus_emit_quit(m_conn->maki->bus, time.tv_sec, m_conn->server, from_nick, maki_remove_colon(remaining));
					}
					else
					{
						maki_dbus_emit_quit(m_conn->maki->bus, time.tv_sec, m_conn->server, from_nick, "");
					}
				}
				else if (g_ascii_strncasecmp(type, "KICK", 4) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 3);
					gchar* channel = tmp[0];
					gchar* nick = tmp[1];
					gchar* msg = maki_remove_colon(tmp[2]);

					if (nick && msg)
					{
						maki_dbus_emit_kick(m_conn->maki->bus, time.tv_sec, m_conn->server, channel, from_nick, nick, msg);
					}
					else if (nick)
					{
						maki_dbus_emit_kick(m_conn->maki->bus, time.tv_sec, m_conn->server, channel, from_nick, nick, "");
					}

					g_strfreev(tmp);
				}
				else if (g_ascii_strncasecmp(type, "NICK", 4) == 0 && remaining)
				{
					gchar* nick = maki_remove_colon(remaining);

					if (g_ascii_strcasecmp(from_nick, m_conn->nick) == 0)
					{
						g_free(m_conn->nick);
						m_conn->nick = g_strdup(nick);
					}

					maki_dbus_emit_nick(m_conn->maki->bus, time.tv_sec, m_conn->server, from_nick, nick);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_NAMREPLY, 3) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 0);
					gchar* channel = tmp[2];
					gint i;
					struct maki_channel* m_chan;

					if (g_strv_length(tmp) > 3 && (m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
					{
						for (i = 3; i < g_strv_length(tmp); ++i)
						{
							gchar* nick = maki_remove_colon(tmp[i]);
							struct maki_user* m_user;

							if (nick[0] == '@'
							    || nick[0] == '+')
							{
								++nick;
							}

							m_user = g_new(struct maki_user, 1);
							m_user->nick = g_strdup(nick);
							g_hash_table_replace(m_chan->users, m_user->nick, m_user);
						}
					}

					g_strfreev(tmp);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_UNAWAY, 3) == 0)
				{
					maki_dbus_emit_back(m_conn->maki->bus, time.tv_sec, m_conn->server);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_NOWAWAY, 3) == 0)
				{
					maki_dbus_emit_away(m_conn->maki->bus, time.tv_sec, m_conn->server);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_AWAY, 3) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 3);
					gchar* nick = tmp[1];
					gchar* msg = maki_remove_colon(tmp[2]);

					if (nick && msg)
					{
						maki_dbus_emit_away_message(m_conn->maki->bus, time.tv_sec, m_conn->server, nick, msg);
					}

					g_strfreev(tmp);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_ENDOFMOTD, 3) == 0 || g_ascii_strncasecmp(type, IRC_ERR_NOMOTD, 3) == 0)
				{
					g_timeout_add_seconds(1, maki_join, m_conn);
					m_conn->connected = TRUE;
					maki_dbus_emit_connected(m_conn->maki->bus, time.tv_sec, m_conn->server, m_conn->nick);
				}
				else if (g_ascii_strncasecmp(type, IRC_ERR_NICKNAMEINUSE, 3) == 0)
				{
					if (!m_conn->connected)
					{
						gchar* buffer;
						gchar* nick;

						nick = g_strconcat(m_conn->nick, "_", NULL);
						g_free(m_conn->nick);
						m_conn->nick = nick;

						buffer = g_strdup_printf("NICK %s", m_conn->nick);
						sashimi_send(m_conn->connection, buffer);
						g_free(buffer);
					}
				}
			}

			g_strfreev(from);
			g_strfreev(parts);
		}


		g_free(message);
	}
}
