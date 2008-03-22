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

#include <unistd.h>

#include "maki.h"

void maki_shutdown (struct maki* maki)
{
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_hash_table_iter_init(&iter, maki->connections);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		struct maki_connection* m_conn = value;

		while (!g_queue_is_empty(m_conn->channels))
		{
			g_free(g_queue_pop_head(m_conn->channels));
		}

		g_queue_free(m_conn->channels);
		sashimi_disconnect(m_conn->connection);
		sashimi_free(m_conn->connection);
		g_free(m_conn->server);
		g_free(m_conn);
	}

	g_hash_table_unref(maki->connections);

	g_free(maki->directories.logs);
	g_free(maki->directories.servers);

	g_main_loop_unref(maki->loop);

	dbus_g_connection_unref(maki->bus->bus);
	g_object_unref(maki->bus);
}

gchar* maki_remove_colon (gchar* string)
{
	if (string != NULL && string[0] == ':')
	{
		++string;
	}

	return string;
}

void maki_callback (gchar* message, gpointer data)
{
	GTimeVal time;
	struct maki_connection* m_conn = data;

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
		from = g_strsplit_set(parts[0], "!@", 3);
		from_nick = from[0];
		type = parts[1];
		to = parts[2];
		msg = parts[3];

		if (from && from_nick && type && to)
		{
			from_nick = maki_remove_colon(from_nick);
			to = maki_remove_colon(to);
			msg = maki_remove_colon(msg);

			if (g_ascii_strncasecmp(type, "PRIVMSG", 7) == 0 && msg)
			{
				maki_dbus_emit_message(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick, msg);
			}
			else if (g_ascii_strncasecmp(type, "JOIN", 4) == 0)
			{
				maki_dbus_emit_join(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick);
			}
			else if (g_ascii_strncasecmp(type, "PART", 4) == 0)
			{
				maki_dbus_emit_part(m_conn->maki->bus, time.tv_sec, m_conn->server, to, from_nick);
			}
		}

		g_strfreev(from);
		g_strfreev(parts);
	}


	g_free(message);
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

	maki.connections = g_hash_table_new(g_str_hash, g_str_equal);
	maki.directories.logs = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "logs", NULL);
	maki.directories.servers = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "servers", NULL);

	maki_servers(&maki);

	maki.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(maki.loop);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	*/

	maki_shutdown(&maki);

	return 0;
}
