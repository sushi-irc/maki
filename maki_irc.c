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
#include "maki_cache.h"
#include "maki_irc.h"
#include "maki_misc.h"

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

void maki_nickserv (struct maki_connection* m_conn)
{
	if (m_conn->nickserv.password != NULL)
	{
		gchar* buffer;

		if (strcmp(m_conn->nick, m_conn->initial_nick) != 0)
		{
			buffer = g_strdup_printf("PRIVMSG NickServ :GHOST %s %s", m_conn->initial_nick, m_conn->nickserv.password);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);

			buffer = g_strdup_printf("NICK %s", m_conn->initial_nick);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);
		}

		buffer = g_strdup_printf("PRIVMSG NickServ :IDENTIFY %s", m_conn->nickserv.password);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}
}

gboolean maki_mode_has_parameter (struct maki_connection* m_conn, gchar sign, gchar mode)
{
	gint type;
	gchar* chanmode;

	if (m_conn->support.chanmodes == NULL)
	{
		return FALSE;
	}

	if (strchr(m_conn->support.prefix.modes, mode) != NULL)
	{
		return TRUE;
	}

	for (chanmode = m_conn->support.chanmodes, type = 0; *chanmode != '\0'; ++chanmode)
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
				if (sign == '+')
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			else if (type == 3)
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

gint maki_prefix_position (struct maki_connection* m_conn, gboolean is_prefix, gchar prefix)
{
	guint pos = 0;

	if (is_prefix)
	{
		while (m_conn->support.prefix.prefixes[pos] != '\0')
		{
			if (m_conn->support.prefix.prefixes[pos] == prefix)
			{
				return pos;
			}

			pos++;
		}
	}
	else
	{
		while (m_conn->support.prefix.modes[pos] != '\0')
		{
			if (m_conn->support.prefix.modes[pos] == prefix)
			{
				return pos;
			}

			pos++;
		}
	}

	return -1;
}

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

		if (!m_chan->autojoin && !m_chan->joined)
		{
			continue;
		}

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

void maki_commands (struct maki_connection* m_conn)
{
	if (m_conn->commands != NULL)
	{
		gchar** command;

		command = m_conn->commands;

		while (*command != NULL)
		{
			sashimi_send(m_conn->connection, *command);
			++command;
		}
	}
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

		/*
		 * Check for valid UTF-8, because strange crashes can occur otherwise.
		 */
		if (!g_utf8_validate(message, -1, NULL))
		{
			gchar* tmp;

			/*
			 * If the message is not in UTF-8 we will just assume that it is in ISO-8859-1.
			 */
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

			if (m_conn->ignores != NULL)
			{
				gboolean match = FALSE;
				gint i;

				for (i = 0; i < g_strv_length(m_conn->ignores); ++i)
				{
					if (g_pattern_match_simple(m_conn->ignores[i], maki_remove_colon(parts[0])))
					{
						match = TRUE;
						break;
					}
				}

				if (match)
				{
					g_strfreev(from);
					g_strfreev(parts);
					g_free(message);
					continue;
				}
			}

			if (from && from_nick && type)
			{
				if (strncmp(type, "PRIVMSG", 7) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 2);
					gchar* target = tmp[0];
					gchar* msg = maki_remove_colon(tmp[1]);

					if (target != NULL && msg != NULL)
					{
						if (msg[0] == '\1')
						{
							++msg;

							if (msg[0] && msg[strlen(msg) - 1] == '\1')
							{
								msg[strlen(msg) - 1] = '\0';
							}

							if (strncmp(msg, "ACTION", 6) == 0 && strlen(msg) > 6)
							{
								maki_dbus_emit_action(maki->bus, time.tv_sec, m_conn->server, from_nick, target, msg + 7);
							}
							else
							{
								if (strcmp(target, m_conn->nick) == 0)
								{
									gchar* buffer = NULL;

									if (strncmp(msg, "VERSION", 7) == 0)
									{
										buffer = g_strdup_printf("NOTICE %s :\001VERSION %s %s\001", from_nick, SUSHI_NAME, SUSHI_VERSION);
									}
									else if (strncmp(msg, "PING", 4) == 0)
									{
										buffer = g_strdup_printf("NOTICE %s :\001%s\001", from_nick, msg);
									}

									if (buffer != NULL)
									{
										sashimi_send(m_conn->connection, buffer);
										g_free(buffer);
									}
								}

								maki_dbus_emit_ctcp(maki->bus, time.tv_sec, m_conn->server, from_nick, target, msg);
							}
						}
						else
						{
							maki_dbus_emit_message(maki->bus, time.tv_sec, m_conn->server, from_nick, target, msg);
						}
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, "JOIN", 4) == 0 && remaining)
				{
					gchar* channel = (remaining[0] == ':') ? remaining + 1 : remaining;
					struct maki_channel* m_chan;

					g_strdelimit(channel, " ", '\0');

					if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
					{
						struct maki_channel_user* m_cuser;
						struct maki_user* m_user;

						m_user = maki_cache_insert(m_conn->users, from_nick);
						m_cuser = maki_channel_user_new(m_user);
						g_hash_table_replace(m_chan->users, m_cuser->user->nick, m_cuser);
					}

					if (strcmp(from_nick, m_conn->nick) == 0)
					{
						if (m_chan != NULL)
						{
							m_chan->joined = TRUE;
						}
						else
						{
							m_chan = maki_channel_new(channel);
							m_chan->joined = TRUE;

							g_hash_table_replace(m_conn->channels, m_chan->name, m_chan);
						}
					}

					maki_dbus_emit_join(maki->bus, time.tv_sec, m_conn->server, from_nick, channel);
				}
				else if (strncmp(type, "PART", 4) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 2);
					gchar* channel = tmp[0];
					gchar* msg = maki_remove_colon(tmp[1]);
					struct maki_channel* m_chan;

					if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
					{
						g_hash_table_remove(m_chan->users, from_nick);

						if (strcmp(from_nick, m_conn->nick) == 0)
						{
							if (!m_chan->autojoin && m_chan->key == NULL)
							{
								g_hash_table_remove(m_conn->channels, channel);
							}
							else
							{
								m_chan->joined = FALSE;
							}
						}
					}

					if (msg)
					{
						maki_dbus_emit_part(maki->bus, time.tv_sec, m_conn->server, from_nick, channel, msg);
					}
					else
					{
						maki_dbus_emit_part(maki->bus, time.tv_sec, m_conn->server, from_nick, channel, "");
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, "QUIT", 4) == 0)
				{
					GHashTableIter iter;
					gpointer key;
					gpointer value;

					g_hash_table_iter_init(&iter, m_conn->channels);

					while (g_hash_table_iter_next(&iter, &key, &value))
					{
						struct maki_channel* m_chan = value;

						g_hash_table_remove(m_chan->users, from_nick);
					}

					if (remaining)
					{
						maki_dbus_emit_quit(maki->bus, time.tv_sec, m_conn->server, from_nick, maki_remove_colon(remaining));
					}
					else
					{
						maki_dbus_emit_quit(maki->bus, time.tv_sec, m_conn->server, from_nick, "");
					}
				}
				else if (strncmp(type, "KICK", 4) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 3);
					gchar* channel = tmp[0];
					gchar* nick = tmp[1];
					gchar* msg = maki_remove_colon(tmp[2]);

					if (channel != NULL && nick != NULL)
					{
						struct maki_channel* m_chan;

						if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
						{
							g_hash_table_remove(m_chan->users, nick);

							if (strcmp(nick, m_conn->nick) == 0)
							{
								if (!m_chan->autojoin && m_chan->key == NULL)
								{
									g_hash_table_remove(m_conn->channels, channel);
								}
								else
								{
									m_chan->joined = FALSE;
								}
							}
						}

						if (msg != NULL)
						{
							maki_dbus_emit_kick(maki->bus, time.tv_sec, m_conn->server, from_nick, channel, nick, msg);
						}
						else
						{
							maki_dbus_emit_kick(maki->bus, time.tv_sec, m_conn->server, from_nick, channel, nick, "");
						}
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, "NICK", 4) == 0 && remaining)
				{
					gchar* nick = maki_remove_colon(remaining);
					GHashTableIter iter;
					gpointer key;
					gpointer value;

					g_hash_table_iter_init(&iter, m_conn->channels);

					while (g_hash_table_iter_next(&iter, &key, &value))
					{
						struct maki_channel* m_chan = value;
						struct maki_channel_user* m_cuser;

						if ((m_cuser = g_hash_table_lookup(m_chan->users, from_nick)) != NULL)
						{
							guint prefix;
							struct maki_user* m_user;

							prefix = m_cuser->prefix;

							g_hash_table_remove(m_chan->users, from_nick);

							m_user = maki_cache_insert(m_conn->users, nick);
							m_cuser = maki_channel_user_new(m_user);
							m_cuser->prefix = prefix;
							g_hash_table_replace(m_chan->users, m_cuser->user->nick, m_cuser);
						}
					}

					if (strcmp(from_nick, m_conn->nick) == 0)
					{
						g_free(m_conn->nick);
						m_conn->nick = g_strdup(nick);

						if (strcmp(m_conn->nick, m_conn->initial_nick) == 0)
						{
							maki_nickserv(m_conn);
						}
					}

					maki_dbus_emit_nick(maki->bus, time.tv_sec, m_conn->server, from_nick, nick);
				}
				else if (strncmp(type, "NOTICE", 6) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 2);
					gchar* target = tmp[0];
					gchar* msg = maki_remove_colon(tmp[1]);

					if (target != NULL && msg != NULL)
					{
						maki_dbus_emit_notice(maki->bus, time.tv_sec, m_conn->server, from_nick, target, msg);
					}

					g_strfreev(tmp);
				}
				else if ((strncmp(type, "MODE", 4) == 0 || strncmp(type, IRC_RPL_CHANNELMODEIS, 3) == 0) && remaining)
				{
					gint offset = 0;
					gchar** tmp;
					gchar* target;

					if (type[0] != 'M')
					{
						offset = 1;
						from_nick = "";
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
							gchar sign = '+';
							gchar buffer[3];
							gchar* mode;

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

								if (maki_mode_has_parameter(m_conn, sign, *mode) && i < g_strv_length(modes))
								{
									gint pos;

									if ((pos = maki_prefix_position(m_conn, FALSE, *mode)) >= 0)
									{
										struct maki_channel* m_chan;
										struct maki_channel_user* m_cuser;

										if ((m_chan = g_hash_table_lookup(m_conn->channels, target)) != NULL
										    && (m_cuser = g_hash_table_lookup(m_chan->users, modes[i])) != NULL)
										{
											if (sign == '+')
											{
												m_cuser->prefix |= (1 << pos);
											}
											else
											{
												m_cuser->prefix &= ~(1 << pos);
											}
										}
									}

									maki_dbus_emit_mode(maki->bus, time.tv_sec, m_conn->server, from_nick, target, buffer, modes[i]);
									++i;
								}
								else
								{
									maki_dbus_emit_mode(maki->bus, time.tv_sec, m_conn->server, from_nick, target, buffer, "");
								}
							}
						}

						g_strfreev(modes);
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, IRC_RPL_NAMREPLY, 3) == 0 && remaining)
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
							guint prefix = 0;
							gint pos;
							struct maki_channel_user* m_cuser;
							struct maki_user* m_user;

							if ((pos = maki_prefix_position(m_conn, TRUE, *nick)) >= 0)
							{
								prefix |= (1 << pos);
								++nick;
							}

							m_user = maki_cache_insert(m_conn->users, nick);
							m_cuser = maki_channel_user_new(m_user);
							g_hash_table_replace(m_chan->users, m_cuser->user->nick, m_cuser);
							m_cuser->prefix = prefix;
						}
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, IRC_RPL_UNAWAY, 3) == 0)
				{
					maki_dbus_emit_back(maki->bus, time.tv_sec, m_conn->server);
				}
				else if (strncmp(type, IRC_RPL_NOWAWAY, 3) == 0)
				{
					maki_dbus_emit_away(maki->bus, time.tv_sec, m_conn->server);
				}
				else if (strncmp(type, IRC_RPL_AWAY, 3) == 0 && remaining)
				{
					gchar** tmp = g_strsplit(remaining, " ", 3);
					gchar* nick = tmp[1];
					gchar* msg = maki_remove_colon(tmp[2]);

					if (nick && msg)
					{
						maki_dbus_emit_away_message(maki->bus, time.tv_sec, m_conn->server, nick, msg);
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, IRC_RPL_ENDOFMOTD, 3) == 0 || strncmp(type, IRC_ERR_NOMOTD, 3) == 0)
				{
					m_conn->connected = TRUE;
					maki_dbus_emit_connected(maki->bus, time.tv_sec, m_conn->server, m_conn->nick);
					maki_nickserv(m_conn);
					g_timeout_add_seconds(3, maki_join, m_conn);
					maki_commands(m_conn);
				}
				else if (strncmp(type, IRC_ERR_NICKNAMEINUSE, 3) == 0)
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
				else if (strncmp(type, IRC_RPL_MOTD, 3) == 0 && remaining)
				{
					gchar** tmp;

					tmp = g_strsplit(remaining, " ", 2);

					if (tmp[1] != NULL)
					{
						maki_dbus_emit_motd(maki->bus, time.tv_sec, m_conn->server, maki_remove_colon(tmp[1]));
					}

					g_strfreev(tmp);
				}
				else if ((strncmp(type, IRC_RPL_TOPIC, 3) == 0 || strncmp(type, "TOPIC", 5) == 0) && remaining)
				{
					gint offset = 0;
					gchar** tmp;
					gchar* channel;
					gchar* topic;

					if (type[0] != 'T')
					{
						offset = 1;
						from_nick = "";
					}

					tmp = g_strsplit(remaining, " ", 2 + offset);
					channel = tmp[offset];
					topic = maki_remove_colon(tmp[1 + offset]);

					if (tmp[0] != NULL && channel != NULL && topic != NULL)
					{
						maki_dbus_emit_topic(maki->bus, time.tv_sec, m_conn->server, from_nick, channel, topic);
					}

					g_strfreev(tmp);
				}
				else if (strncmp(type, IRC_RPL_ISUPPORT, 3) == 0 && remaining)
				{
					gint i;
					gchar** tmp;

					tmp = g_strsplit(remaining, " ", 0);

					for (i = 1; i < g_strv_length(tmp); ++i)
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
								g_free(m_conn->support.chanmodes);
								m_conn->support.chanmodes = g_strdup(support[1]);
							}
							else if (strncmp(support[0], "PREFIX", 6) == 0)
							{
								gchar* paren;

								paren = strchr(support[1], ')');

								if (support[1][0] == '(' && paren != NULL)
								{
									*paren = '\0';
									g_free(m_conn->support.prefix.modes);
									g_free(m_conn->support.prefix.prefixes);
									m_conn->support.prefix.modes = g_strdup(support[1] + 1);
									m_conn->support.prefix.prefixes = g_strdup(paren + 1);
								}
							}
						}

						g_strfreev(support);
					}

					g_strfreev(tmp);
				}
			}

			g_strfreev(from);
			g_strfreev(parts);
		}


		g_free(message);
	}
}
