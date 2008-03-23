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
	s_join,
	s_message,
	s_part,
	s_quit,
	s_last
};

guint signals[s_last];

void maki_dbus_emit_join (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick)
{
	g_signal_emit(self, signals[s_join], 0, time, server, channel, nick);
}

void maki_dbus_emit_message (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_message], 0, time, server, channel, nick, message);
}

void maki_dbus_emit_part (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick)
{
	g_signal_emit(self, signals[s_part], 0, time, server, channel, nick);
}

void maki_dbus_emit_quit (makiDBus* self, gint64 time, const gchar* server, const gchar* nick)
{
	g_signal_emit(self, signals[s_quit], 0, time, server, nick);
}

gboolean maki_dbus_channels (makiDBus* self, gchar* server, gchar*** channels, GError** error)
{
	gchar** channel;
	guint i;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		channel = *channels = g_new(gchar*, g_queue_get_length(m_conn->channels) + 1);

		for (i = 0; i < g_queue_get_length(m_conn->channels); ++i)
		{
			*channel = g_strdup(g_queue_peek_nth(m_conn->channels, i));
			++channel;
		}
	}
	else
	{
		channel = *channels = g_new(gchar*, 1);
	}

	*channel = NULL;

	return TRUE;
}

gboolean maki_dbus_join (makiDBus* self, gchar* server, gchar* channel, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		buffer = g_strdup_printf("JOIN %s", channel);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_part (makiDBus* self, gchar* server, gchar* channel, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		buffer = g_strdup_printf("PART %s", channel);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_quit (makiDBus* self, gchar* server, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		GTimeVal time;

		sashimi_disconnect(m_conn->connection);

		g_get_current_time(&time);
		maki_dbus_emit_quit(self, time.tv_sec, server, sashimi_nick(m_conn->connection));
	}

	return TRUE;
}

gboolean maki_dbus_raw (makiDBus* self, gchar* server, gchar* command, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		sashimi_send(m_conn->connection, command);
	}

	return TRUE;
}

gboolean maki_dbus_say (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	gchar* buffer;
	GTimeVal time;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		buffer = g_strdup_printf("PRIVMSG %s :%s", channel, message);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		g_get_current_time(&time);
		maki_dbus_emit_message(self, time.tv_sec, server, channel, sashimi_nick(m_conn->connection), message);
	}

	return TRUE;
}

gboolean maki_dbus_servers (makiDBus* self, gchar*** servers, GError** error)
{
	gchar** server;
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	server = *servers = g_new(gchar*, g_hash_table_size(self->maki->connections) + 1);
	g_hash_table_iter_init(&iter, self->maki->connections);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		*server = g_strdup(key);
		++server;
	}

	*server = NULL;

	return TRUE;
}

gboolean maki_dbus_shutdown (makiDBus* self, GError** error)
{
	g_main_loop_quit(self->maki->loop);

	return TRUE;
}

gboolean maki_dbus_topic (makiDBus* self, gchar* server, gchar* channel, gchar* topic, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		buffer = g_strdup_printf("TOPIC %s :%s", channel, topic);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

#include "maki_dbus.h"

G_DEFINE_TYPE(makiDBus, maki_dbus, G_TYPE_OBJECT)

static void maki_dbus_init (makiDBus* obj)
{
	DBusGProxy* proxy;
	GError* error = NULL;
	guint request_name_result;

	if ((obj->bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error)) != NULL)
	{
		dbus_g_connection_register_g_object(obj->bus, "/de/ikkoku/sushi", G_OBJECT(obj));

		if ((proxy = dbus_g_proxy_new_for_name(obj->bus, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS)) != NULL)
		{
			if (!org_freedesktop_DBus_request_name(proxy, "de.ikkoku.sushi", 0, &request_name_result, &error))
			{
				g_error_free(error);
			}

			g_object_unref(proxy);
		}
	}
	else
	{
		g_error_free(error);
	}
}

static void maki_dbus_class_init (makiDBusClass* klass)
{
	dbus_g_object_type_install_info(MAKI_DBUS_TYPE, &dbus_glib_maki_dbus_object_info);

	signals[s_join] = g_signal_new("join", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING, G_TYPE_NONE, 4, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_message] = g_signal_new("message", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING, G_TYPE_NONE, 5, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_part] = g_signal_new("part", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING, G_TYPE_NONE, 4, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_quit] = g_signal_new("quit", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_user_marshal_VOID__INT64_STRING_STRING, G_TYPE_NONE, 3, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
}
