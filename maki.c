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
	struct maki_connection* maki_connection = data;

	g_get_current_time(&time);

	g_print("%ld %s %s\n", time.tv_sec, maki_connection->server, message);

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

		if (from && from_nick && type && to && msg)
		{
			from_nick = maki_remove_colon(from_nick);
			to = maki_remove_colon(to);
			msg = maki_remove_colon(msg);

			if (g_ascii_strncasecmp(type, "PRIVMSG", 7) == 0)
			{
				maki_dbus_emit_message(maki_connection->maki->bus, time.tv_sec, maki_connection->server, to, from_nick, msg);
			}
		}

		g_strfreev(from);
		g_strfreev(parts);
	}


	g_free(message);
}

int main (int argc, char* argv[])
{
	GMainLoop* loop;
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

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	sashimi_disconnect(connection);
	sashimi_free(connection);
	*/

	return 0;
}
