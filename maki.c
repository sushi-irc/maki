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

	g_print("%d %s %s\n", time.tv_sec, maki_connection->server, message);

	if (message[0] == ':')
	{
		gchar** parts;
		gchar** from;
		gchar* from_nick;
		gchar* from_name;
		gchar* from_host;
		gchar* type;
		gchar* to;
		gchar* msg;

		parts = g_strsplit(message, " ", 4);
		from = g_strsplit_set(maki_remove_colon(parts[0]), "!@", 3);
		from_nick = from[0];
		from_name = from[1];
		from_host = (from_name != NULL) ? from[2] : NULL;
		type = parts[1];
		to = maki_remove_colon(parts[2]);
		msg = maki_remove_colon(parts[3]);

		if (g_ascii_strncasecmp(type, "PRIVMSG", 7) == 0)
		{
			maki_signal_send_message(maki_connection->maki->bus, time, maki_connection->server, to, from_nick, msg);
		}

		g_strfreev(from);
		g_strfreev(parts);
	}


	g_free(message);
}

int main (int argc, char* argv[])
{
	DBusError error;
	DBusConnection* bus;
	struct maki maki;

	dbus_error_init(&error);
	bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
	dbus_error_free(&error);

	maki.bus = bus;
	maki.connections = g_hash_table_new(g_str_hash, g_str_equal);
	maki.directories.logs = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "logs", NULL);
	maki.directories.servers = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".sushi", G_DIR_SEPARATOR_S, "servers", NULL);

	maki_servers(&maki);

	while (TRUE)
		sleep(1);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	sashimi_disconnect(connection);
	sashimi_free(connection);
	*/

	return 0;
}
