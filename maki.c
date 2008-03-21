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
	dbus_uint32_t serial = 0;
	DBusMessage* bus_message;
	DBusMessageIter bus_args;
	struct maki_callback_data* callback_data = data;

	g_get_current_time(&time);

	g_print("%d %s %s\n", time.tv_sec, callback_data->server, message);

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
			bus_message = dbus_message_new_signal("/de/ikkoku/sushi", "de.ikkoku.sushi", "message");
			dbus_message_iter_init_append(bus_message, &bus_args);
			dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_INT64, &time.tv_sec);
			dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &callback_data->server);
			dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &to);
			dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &from_nick);
			dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &msg);
			dbus_connection_send(callback_data->bus, bus_message, &serial);
			dbus_connection_flush(callback_data->bus);
			dbus_message_unref(bus_message);
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
	struct sashimi_connection* connection;
	struct maki_callback_data* callback_data;

	dbus_error_init(&error);
	bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
	dbus_error_free(&error);

	callback_data = g_new(struct maki_callback_data, 1);

	callback_data->server = "xesio.ath.cx";
	callback_data->bus = bus;

	connection = sashimi_connect("xesio.ath.cx", 6667, maki_callback, callback_data);
	sashimi_send(connection, "USER schnauf xesio.ath.cx xesio.ath.cx :schnauf");
	sashimi_send(connection, "NICK schnauf");
	sleep(1);
	sashimi_send(connection, "PRIVMSG schnauf ::]");
	sashimi_send(connection, "JOIN #test");
	sashimi_send(connection, "PRIVMSG #test :schnauf!");
	sleep(600);
	sashimi_send(connection, "QUIT :schnauf");
	sashimi_close(connection);

	g_free(callback_data);

	return 0;
}
