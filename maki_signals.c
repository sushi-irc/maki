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

#include "maki.h"

void maki_signal_send_message (DBusConnection* bus, GTimeVal time, gchar* server, gchar* to, gchar* from_nick, gchar* message)
{
	dbus_uint32_t serial = 0;
	DBusMessage* bus_message;
	DBusMessageIter bus_args;

	bus_message = dbus_message_new_signal("/de/ikkoku/sushi", "de.ikkoku.sushi", "message");
	dbus_message_iter_init_append(bus_message, &bus_args);
	dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_INT64, &time.tv_sec);
	dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &server);
	dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &to);
	dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &from_nick);
	dbus_message_iter_append_basic(&bus_args, DBUS_TYPE_STRING, &message);
	dbus_connection_send(bus, bus_message, &serial);
	dbus_connection_flush(bus);
	dbus_message_unref(bus_message);
}
