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
#include "maki_misc.h"
#include "maki_servers.h"

void maki_shutdown (struct maki* maki)
{
	maki->threads.terminate = TRUE;
	g_thread_join(maki->threads.messages);
	g_async_queue_unref(maki->message_queue);

	g_hash_table_destroy(maki->connections);

	g_free(maki->directories.logs);
	g_free(maki->directories.servers);

	dbus_g_connection_unref(maki->bus->bus);
	g_object_unref(maki->bus);

	g_main_loop_quit(maki->loop);
	g_main_loop_unref(maki->loop);
}

gpointer maki_callback (gpointer data)
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

		g_get_current_time(&time);

		g_print("%ld %s %s\n", time.tv_sec, m_conn->server, message);

		if (message[0] == ':')
		{
			gchar** parts;
			gchar** from;
			gchar* from_nick;
			gchar* type;
			gchar* to;
			gchar* msg;

			parts = g_strsplit(message, " ", 4);
			from = g_strsplit_set(maki_remove_colon(parts[0]), "!@", 3);
			from_nick = from[0];
			type = parts[1];
			to = maki_remove_colon(parts[2]);
			msg = maki_remove_colon(parts[3]);

			if (from && from_nick && type)
			{
				if (g_ascii_strncasecmp(type, "PRIVMSG", 7) == 0 && msg)
				{
					if (msg[0] == '\1')
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
					else
					{
						maki_dbus_emit_message(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick, msg);
					}
				}
				else if (g_ascii_strncasecmp(type, "JOIN", 4) == 0 && to)
				{
					struct maki_channel* m_chan;

					if (g_ascii_strcasecmp(from_nick, m_conn->nick) == 0)
					{
						struct maki_channel* m_chan;

						m_chan = g_new(struct maki_channel, 1);
						m_chan->name = g_strdup(to);
						m_chan->users = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_user_destroy);

						g_hash_table_insert(m_conn->channels, m_chan->name, m_chan);
					}

					if ((m_chan = g_hash_table_lookup(m_conn->channels, to)) != NULL)
					{
						struct maki_user* m_user;

						m_user = g_new(struct maki_user, 1);
						m_user->nick = g_strdup(from_nick);
						g_hash_table_replace(m_chan->users, m_user->nick, m_user);
					}

					maki_dbus_emit_join(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick);
				}
				else if (g_ascii_strncasecmp(type, "PART", 4) == 0 && to)
				{
					struct maki_channel* m_chan;

					if (g_ascii_strcasecmp(from_nick, m_conn->nick) == 0)
					{
						g_hash_table_remove(m_conn->channels, to);
					}

					if ((m_chan = g_hash_table_lookup(m_conn->channels, to)) != NULL)
					{
						g_hash_table_remove(m_chan->users, from_nick);
					}

					maki_dbus_emit_part(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick);
				}
				else if (g_ascii_strncasecmp(type, "QUIT", 4) == 0)
				{
					maki_dbus_emit_quit(m_conn->maki->bus, time.tv_sec, m_conn->server, from_nick);
				}
				else if (g_ascii_strncasecmp(type, "KICK", 4) == 0 && to && msg)
				{
					gchar** kick;

					kick = g_strsplit(msg, " ", 2);
					maki_dbus_emit_kick(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick, kick[0]);
					g_strfreev(kick);
				}
				else if (g_ascii_strncasecmp(type, "NICK", 4) == 0 && to)
				{
					if (g_ascii_strcasecmp(m_conn->nick, from_nick) == 0)
					{
						g_free(m_conn->nick);
						m_conn->nick = g_strdup(to);
					}

					maki_dbus_emit_nick(m_conn->maki->bus, time.tv_sec, m_conn->server, from_nick, to);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_NAMREPLY, 3) == 0 && to && msg)
				{
					gint i;
					gchar** reply = g_strsplit(msg + 2, " ", 0);
					struct maki_channel* m_chan;

					m_chan = g_hash_table_lookup(m_conn->channels, reply[0]);

					for (i = 1; i < g_strv_length(reply); ++i)
					{
						struct maki_user* m_user;

						m_user = g_new(struct maki_user, 1);
						m_user->nick = g_strdup(maki_remove_colon(reply[i]));
						g_hash_table_replace(m_chan->users, m_user->nick, m_user);
					}

					g_strfreev(reply);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_UNAWAY, 3) == 0)
				{
					maki_dbus_emit_back(m_conn->maki->bus, time.tv_sec, m_conn->server);
				}
				else if (g_ascii_strncasecmp(type, IRC_RPL_NOWAWAY, 3) == 0)
				{
					maki_dbus_emit_away(m_conn->maki->bus, time.tv_sec, m_conn->server);
				}
			}

			g_strfreev(from);
			g_strfreev(parts);
		}


		g_free(message);
	}
}

int main (int argc, char* argv[])
{
	struct maki maki;

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	maki.bus = g_object_new(MAKI_DBUS_TYPE, NULL);
	maki.bus->maki = &maki;

	maki.connections = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_connection_destroy);

	maki.directories.logs = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "logs", NULL);
	maki.directories.servers = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "servers", NULL);

	maki.message_queue = g_async_queue_new();

	maki.threads.messages = g_thread_create(maki_callback, &maki, TRUE, NULL);
	maki.threads.terminate = FALSE;

	maki_servers(&maki);

	maki.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(maki.loop);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	*/

	return 0;
}
