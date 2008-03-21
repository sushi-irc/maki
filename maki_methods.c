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

gpointer maki_methods (gpointer data)
{
	DBusError error;
	DBusMessage* bus_message;
	struct maki* maki = data;

	dbus_error_init(&error);
	dbus_bus_request_name(maki->bus, "de.ikkoku.sushi", DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
	dbus_error_free(&error);

	for (;;)
	{
		dbus_connection_read_write(maki->bus, 0);
		bus_message = dbus_connection_pop_message(maki->bus);

		if (bus_message == NULL)
		{
			g_usleep(1000000);
			continue;
		}

		g_print("METHOD_CALL!\n");

		if (dbus_message_is_method_call(bus_message, "de.ikkoku.sushi", "say"))
		{
			g_print("METHOD_CALL_SAY!\n");
			maki_method_say(maki, bus_message);
		}

		dbus_message_unref(bus_message);
	}

	return NULL;
}

void maki_method_say (struct maki* maki, DBusMessage* message)
{
	dbus_uint32_t serial = 0;
	DBusMessage* reply;
	DBusMessageIter args;
	struct maki_connection* connection;

	gchar* buffer;
	gchar* server;
	gchar* channel;
	gchar* msg;

	dbus_message_iter_init(message, &args);
	dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_STRING;
	dbus_message_iter_get_basic(&args, &server);
	dbus_message_iter_next(&args);
	dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_STRING;
	dbus_message_iter_get_basic(&args, &channel);
	dbus_message_iter_next(&args);
	dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_STRING;
	dbus_message_iter_get_basic(&args, &msg);

	connection = g_hash_table_lookup(maki->connections, server);

	buffer = g_strdup_printf("PRIVMSG %s :%s", channel, msg);
	g_print("%s\n", buffer);

	sashimi_send(connection->connection, buffer);

	g_free(buffer);

	reply = dbus_message_new_method_return(message);
	dbus_connection_send(maki->bus, reply, &serial);
	dbus_connection_flush(maki->bus);
	dbus_message_unref(reply);
}
