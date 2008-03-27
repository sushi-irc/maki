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
#include "maki_misc.h"
#include "maki_servers.h"

enum
{
	s_action,
	s_away,
	s_away_message,
	s_back,
	s_connect,
	s_connected,
	s_join,
	s_kick,
	s_message,
	s_nick,
	s_part,
	s_quit,
	s_reconnect,
	s_last
};

guint signals[s_last];

void maki_dbus_emit_action (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_action], 0, time, server, channel, nick, message);
}

void maki_dbus_emit_away (makiDBus* self, gint64 time, const gchar* server)
{
	g_signal_emit(self, signals[s_away], 0, time, server);
}

void maki_dbus_emit_away_message (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_away_message], 0, time, server, nick, message);
}

void maki_dbus_emit_back (makiDBus* self, gint64 time, const gchar* server)
{
	g_signal_emit(self, signals[s_back], 0, time, server);
}

void maki_dbus_emit_connect (makiDBus* self, gint64 time, const gchar* server)
{
	g_signal_emit(self, signals[s_connect], 0, time, server);
}

void maki_dbus_emit_connected (makiDBus* self, gint64 time, const gchar* server, const gchar* nick)
{
	g_signal_emit(self, signals[s_connected], 0, time, server, nick);
}

void maki_dbus_emit_join (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick)
{
	g_signal_emit(self, signals[s_join], 0, time, server, channel, nick);
}

void maki_dbus_emit_kick (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* who, const gchar* message)
{
	g_signal_emit(self, signals[s_kick], 0, time, server, channel, nick, who, message);
}

void maki_dbus_emit_message (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_message], 0, time, server, channel, nick, message);
}

void maki_dbus_emit_nick (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* new_nick)
{
	g_signal_emit(self, signals[s_nick], 0, time, server, nick, new_nick);
}

void maki_dbus_emit_part (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_part], 0, time, server, channel, nick, message);
}

void maki_dbus_emit_quit (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_quit], 0, time, server, nick, message);
}

void maki_dbus_emit_reconnect (makiDBus* self, gint64 time, const gchar* server)
{
	g_signal_emit(self, signals[s_reconnect], 0, time, server);
}

gboolean maki_dbus_action (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	gchar* buffer;
	GTimeVal time;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		g_get_current_time(&time);

		g_strdelimit(message, "\r\n", ' ');

		buffer = g_strdup_printf("PRIVMSG %s :\1ACTION %s\1", channel, message);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		maki_dbus_emit_action(self, time.tv_sec, server, channel, m_conn->nick, message);
	}

	return TRUE;
}

gboolean maki_dbus_away (makiDBus* self, gchar* server, gchar* message, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		gchar* buffer;

		buffer = g_strdup_printf("AWAY :%s", message);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_back (makiDBus* self, gchar* server, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		sashimi_send(m_conn->connection, "AWAY");
	}

	return TRUE;
}

gboolean maki_dbus_channels (makiDBus* self, gchar* server, gchar*** channels, GError** error)
{
	gchar** channel;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		GHashTableIter iter;
		gpointer key;
		gpointer value;

		channel = *channels = g_new(gchar*, g_hash_table_size(m_conn->channels) + 1);

		g_hash_table_iter_init(&iter, m_conn->channels);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			struct maki_channel* m_chan = value;

			if (m_chan->joined)
			{
				*channel = g_strdup(m_chan->name);
				++channel;
			}
		}
	}
	else
	{
		channel = *channels = g_new(gchar*, 1);
	}

	*channel = NULL;

	return TRUE;
}

gboolean maki_dbus_connect (makiDBus* self, gchar* server, GError** error)
{
	maki_server_new(self->maki, server);

	return TRUE;
}

gboolean maki_dbus_join (makiDBus* self, gchar* server, gchar* channel, gchar* key, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		if (key[0])
		{
			buffer = g_strdup_printf("JOIN %s %s", channel, key);
		}
		else
		{
			buffer = g_strdup_printf("JOIN %s", channel);
		}

		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_kick (makiDBus* self, gchar* server, gchar* channel, gchar* who, gchar* message, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		if (message[0])
		{
			buffer = g_strdup_printf("KICK %s %s :%s", channel, who, message);
		}
		else
		{
			buffer = g_strdup_printf("KICK %s %s", channel, who);
		}

		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_nick (makiDBus* self, gchar* server, gchar* nick, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		buffer = g_strdup_printf("NICK %s", nick);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_nicks (makiDBus* self, gchar* server, gchar* channel, gchar*** nicks, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		struct maki_channel* m_chan;

		if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL)
		{
			gchar** nick;
			GHashTableIter iter;
			gpointer key;
			gpointer value;

			nick = *nicks = g_new(gchar*, g_hash_table_size(m_chan->users) + 1);
			g_hash_table_iter_init(&iter, m_chan->users);

			while (g_hash_table_iter_next(&iter, &key, &value))
			{
				*nick = g_strdup(key);
				++nick;
			}

			*nick = NULL;
		}
	}

	return TRUE;
}

gboolean maki_dbus_own_nick (makiDBus* self, gchar* server, gchar** nick, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		*nick = g_strdup(m_conn->nick);
	}

	return TRUE;
}

gboolean maki_dbus_part (makiDBus* self, gchar* server, gchar* channel, gchar* message, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		if (message[0])
		{
			buffer = g_strdup_printf("PART %s :%s", channel, message);
		}
		else
		{
			buffer = g_strdup_printf("PART %s", channel);
		}

		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);
	}

	return TRUE;
}

gboolean maki_dbus_quit (makiDBus* self, gchar* server, gchar* message, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		GTimeVal time;

		m_conn->reconnect = FALSE;

		if (message[0])
		{
			gchar* buffer;

			buffer = g_strdup_printf("QUIT :%s", message);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);
		}
		else
		{
			sashimi_send(m_conn->connection, "QUIT :sushi – http://sushi.ikkoku.de/");
		}

		g_get_current_time(&time);
		maki_dbus_emit_quit(self, time.tv_sec, server, m_conn->nick, message);

		g_timeout_add_seconds(1, maki_disconnect_timeout, m_conn);
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
		gchar** messages = NULL;

		g_get_current_time(&time);

		for (buffer = message; *buffer != '\0'; ++buffer)
		{
			if (*buffer == '\r' || *buffer == '\n')
			{
				messages = g_strsplit(message, "\n", 0);
				break;
			}
		}

		if (messages == NULL)
		{
			buffer = g_strdup_printf("PRIVMSG %s :%s", channel, message);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);

			maki_dbus_emit_message(self, time.tv_sec, server, channel, m_conn->nick, message);
		}
		else
		{
			gchar** tmp;

			for (tmp = messages; *tmp != NULL; ++tmp)
			{
				g_strchomp(*tmp);
				buffer = g_strdup_printf("PRIVMSG %s :%s", channel, *tmp);
				sashimi_queue(m_conn->connection, buffer);
				g_free(buffer);

				maki_dbus_emit_message(self, time.tv_sec, server, channel, m_conn->nick, *tmp);
			}

			g_strfreev(messages);
		}
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
		struct maki_connection* m_conn = value;

		if (m_conn->connected)
		{
			*server = g_strdup(key);
			++server;
		}
	}

	*server = NULL;

	return TRUE;
}

gboolean maki_dbus_shutdown (makiDBus* self, GError** error)
{
	maki_shutdown(self->maki);

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

#include "maki_dbus_glue.h"

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

	signals[s_action] =
		g_signal_new("action",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_away] =
		g_signal_new("away",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_away_message] =
		g_signal_new("away_message",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_back] =
		g_signal_new("back",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_connect] =
		g_signal_new("connect",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
	signals[s_connected] =
		g_signal_new("connected",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING,
		             G_TYPE_NONE, 3,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_join] =
		g_signal_new("join",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_kick] =
		g_signal_new("kick",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 6,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_message] =
		g_signal_new("message",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_nick] =
		g_signal_new("nick",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_part] =
		g_signal_new("part",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_quit] =
		g_signal_new("quit",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_reconnect] =
		g_signal_new("reconnect",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING,
		             G_TYPE_NONE, 2,
		             G_TYPE_INT64, G_TYPE_STRING);
}
