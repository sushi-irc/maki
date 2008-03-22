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
#include "maki_marshal.h"

enum
{
	s_message,
	s_last
};

guint signals[s_last];

void maki_dbus_emit_message (makiDBus* self, glong time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_message], 0, time, server, channel, nick, message);
}

gboolean maki_dbus_say (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	gchar* buffer;
	GTimeVal time;
	struct maki_connection* connection;

	connection = g_hash_table_lookup(self->maki->connections, server);

	buffer = g_strdup_printf("PRIVMSG %s :%s", channel, message);
	g_print("%s\n", buffer);

	sashimi_send(connection->connection, buffer);

	g_free(buffer);

	g_get_current_time(&time);
	maki_dbus_emit_message(self, time.tv_sec, server, channel, sashimi_nick(connection->connection), message);

	return TRUE;
}

#include "maki_dbus.h"

G_DEFINE_TYPE(makiDBus, maki_dbus, G_TYPE_OBJECT)

static void maki_dbus_init (makiDBus* obj)
{
	DBusGProxy* proxy;
	GError* error = NULL;
	guint request_name_result;

	obj->bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	dbus_g_connection_register_g_object(obj->bus, "/de/ikkoku/sushi", G_OBJECT(obj));

	proxy = dbus_g_proxy_new_for_name(obj->bus, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	org_freedesktop_DBus_request_name(proxy, "de.ikkoku.sushi", 0, &request_name_result, &error);
}

static void maki_dbus_class_init (makiDBusClass* klass)
{
	dbus_g_object_type_install_info(MAKI_DBUS_TYPE, &dbus_glib_maki_dbus_object_info);

	signals[s_message] = g_signal_new("message", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING, G_TYPE_NONE, 5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}
