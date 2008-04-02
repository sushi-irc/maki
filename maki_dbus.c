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
#include "maki_irc.h"
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
	s_ctcp,
	s_join,
	s_kick,
	s_message,
	s_mode,
	s_motd,
	s_nick,
	s_notice,
	s_part,
	s_quit,
	s_reconnect,
	s_shutdown,
	s_topic,
	s_last
};

guint signals[s_last];

void maki_dbus_emit_action (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(self, signals[s_action], 0, time, server, nick, target, message);
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

void maki_dbus_emit_ctcp (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(self, signals[s_ctcp], 0, time, server, nick, target, message);
}

void maki_dbus_emit_join (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* channel)
{
	g_signal_emit(self, signals[s_join], 0, time, server, nick, channel);
}

void maki_dbus_emit_kick (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* who, const gchar* message)
{
	g_signal_emit(self, signals[s_kick], 0, time, server, nick, channel, who, message);
}

void maki_dbus_emit_message (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(self, signals[s_message], 0, time, server, nick, target, message);
}

void maki_dbus_emit_mode (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* mode, const gchar* parameter)
{
	g_signal_emit(self, signals[s_mode], 0, time, server, nick, target, mode, parameter);
}

void maki_dbus_emit_motd (makiDBus* self, gint64 time, const gchar* server, const gchar* message)
{
	g_signal_emit(self, signals[s_motd], 0, time, server, message);
}

void maki_dbus_emit_nick (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* new_nick)
{
	g_signal_emit(self, signals[s_nick], 0, time, server, nick, new_nick);
}

void maki_dbus_emit_notice (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	g_signal_emit(self, signals[s_notice], 0, time, server, nick, target, message);
}

void maki_dbus_emit_part (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* channel, const gchar* message)
{
	g_signal_emit(self, signals[s_part], 0, time, server, nick, channel, message);
}

void maki_dbus_emit_quit (makiDBus* self, gint64 time, const gchar* server, const gchar* nick, const gchar* message)
{
	g_signal_emit(self, signals[s_quit], 0, time, server, nick, message);
}

void maki_dbus_emit_reconnect (makiDBus* self, gint64 time, const gchar* server)
{
	g_signal_emit(self, signals[s_reconnect], 0, time, server);
}

void maki_dbus_emit_shutdown (makiDBus* self, gint64 time)
{
	g_signal_emit(self, signals[s_shutdown], 0, time);
}

void maki_dbus_emit_topic (makiDBus* self, gint64 time, const gchar* server, const gchar* channel, const gchar* topic)
{
	g_signal_emit(self, signals[s_topic], 0, time, server, channel, topic);
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

		maki_dbus_emit_action(self, time.tv_sec, server, m_conn->nick, channel, message);
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
	struct maki_connection* m_conn;

	*channels = NULL;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		gchar** channel;
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

		*channel = NULL;
	}

	return TRUE;
}

gboolean maki_dbus_connect (makiDBus* self, gchar* server, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		/*
		 * Disconnect, because strange things happen if we call maki_connect() while still connected.
		 */
		maki_disconnect(m_conn);

		if (maki_connect(m_conn) != 0)
		{
			maki_reconnect_callback(m_conn);
		}
	}
	else
	{
		m_conn = maki_server_new(self->maki, server);

		if (m_conn != NULL && !m_conn->autoconnect)
		{
			if (maki_connect(m_conn) != 0)
			{
				maki_reconnect_callback(m_conn);
			}
		}
	}

	return TRUE;
}

gboolean maki_dbus_ctcp (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		gchar* buffer;
		GTimeVal time;

		buffer = g_strdup_printf("PRIVMSG %s :\1%s\1", target, message);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		g_get_current_time(&time);
		maki_dbus_emit_ctcp(self, time.tv_sec, server, m_conn->nick, target, message);
	}

	return TRUE;
}

gboolean maki_dbus_join (makiDBus* self, gchar* server, gchar* channel, gchar* key, GError** error)
{
	gchar* buffer;
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		struct maki_channel* m_chan;

		if ((m_chan = g_hash_table_lookup(m_conn->channels, channel)) != NULL
		    && m_chan->key != NULL
		    && !key[0])
		{
			key = m_chan->key;
		}

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

gboolean maki_dbus_message (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
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
			buffer = g_strdup_printf("PRIVMSG %s :%s", target, message);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);

			maki_dbus_emit_message(self, time.tv_sec, server, m_conn->nick, target, message);
		}
		else
		{
			gchar** tmp;

			for (tmp = messages; *tmp != NULL; ++tmp)
			{
				g_strchomp(*tmp);

				if ((*tmp)[0])
				{
					buffer = g_strdup_printf("PRIVMSG %s :%s", target, *tmp);
					sashimi_queue(m_conn->connection, buffer);
					g_free(buffer);

					maki_dbus_emit_message(self, time.tv_sec, server, m_conn->nick, target, *tmp);
				}
			}

			g_strfreev(messages);
		}
	}

	return TRUE;
}

gboolean maki_dbus_mode (makiDBus* self, gchar* server, gchar* target, gchar* mode, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		gchar* buffer;

		buffer = g_strdup_printf("MODE %s %s", target, mode);
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

	*nicks = NULL;

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

gboolean maki_dbus_notice (makiDBus* self, gchar* server, gchar* target, gchar* message, GError** error)
{
	struct maki_connection* m_conn;

	if ((m_conn = g_hash_table_lookup(self->maki->connections, server)) != NULL)
	{
		gchar* buffer;
		GTimeVal time;

		buffer = g_strdup_printf("NOTICE %s :%s", target, message);
		sashimi_send(m_conn->connection, buffer);
		g_free(buffer);

		g_get_current_time(&time);
		maki_dbus_emit_notice(self, time.tv_sec, server, m_conn->nick, target, message);
	}

	return TRUE;
}

gboolean maki_dbus_own_nick (makiDBus* self, gchar* server, gchar** nick, GError** error)
{
	struct maki_connection* m_conn;

	*nick = NULL;

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
		g_get_current_time(&time);

		if (message[0])
		{
			gchar* buffer;

			buffer = g_strdup_printf("QUIT :%s", message);
			sashimi_send(m_conn->connection, buffer);
			g_free(buffer);

			maki_dbus_emit_quit(self, time.tv_sec, server, m_conn->nick, message);
		}
		else
		{
			sashimi_send(m_conn->connection, "QUIT :" SUSHI_QUIT_MESSAGE);

			maki_dbus_emit_quit(self, time.tv_sec, server, m_conn->nick, SUSHI_QUIT_MESSAGE);
		}

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

gboolean maki_dbus_sushi_get (makiDBus* self, gchar* file, gchar* group, gchar* key, gchar** value, GError** error)
{
	gchar* path;
	GKeyFile* key_file;

	*value = NULL;

	path = g_build_filename(self->maki->directories.sushi, file, NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		*value = g_key_file_get_string(key_file, group, key, NULL);
	}

	g_key_file_free(key_file);
	g_free(path);

	return TRUE;
}

gboolean maki_dbus_sushi_list (makiDBus* self, gchar* directory, gchar* file, gchar* group, gchar*** result, GError** error)
{
	gchar* path;
	GDir* dir;

	*result = NULL;

	if (file[0])
	{
		GKeyFile* key_file;

		path = g_build_filename(self->maki->directories.sushi, directory, file, NULL);
		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			if (group[0])
			{
				*result = g_key_file_get_keys(key_file, group, NULL, NULL);
			}
			else
			{
				*result = g_key_file_get_groups(key_file, NULL);

			}
		}

		g_key_file_free(key_file);
	}
	else
	{
		path = g_build_filename(self->maki->directories.sushi, directory, NULL);

		if ((dir = g_dir_open(path, 0, NULL)) != NULL)
		{
			guint i = 0;
			const gchar* name;
			gchar** tmp;

			tmp = g_new(gchar*, 1);

			while ((name = g_dir_read_name(dir)) != NULL)
			{
				tmp = g_renew(gchar*, tmp, i + 2);
				tmp[i] = g_strdup(name);
				++i;
			}

			tmp[i] = NULL;
			*result = tmp;

			g_dir_close(dir);
		}
	}

	g_free(path);

	return TRUE;
}

gboolean maki_dbus_sushi_remove (makiDBus* self, gchar* file, gchar* group, gchar* key, GError** error)
{
	gchar* path;

	path = g_build_filename(self->maki->directories.sushi, file, NULL);

	if (group[0])
	{
		GKeyFile* key_file;

		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			gchar* contents;

			if (key[0])
			{
				g_key_file_remove_key(key_file, group, key, NULL);
			}
			else
			{
				g_key_file_remove_group(key_file, group, NULL);
			}

			contents = g_key_file_to_data(key_file, NULL, NULL);
			g_file_set_contents(path, contents, -1, NULL);
			g_free(contents);
		}

		g_key_file_free(key_file);
	}
	else
	{
		unlink(path);
	}

	g_free(path);

	return TRUE;
}

gboolean maki_dbus_sushi_set (makiDBus* self, gchar* file, gchar* group, gchar* key, gchar* value, GError** error)
{
	gchar* contents;
	gchar* path;
	GKeyFile* key_file;

	path = g_build_filename(self->maki->directories.sushi, file, NULL);
	key_file = g_key_file_new();

	g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(key_file, group, key, value);
	contents = g_key_file_to_data(key_file, NULL, NULL);
	g_file_set_contents(path, contents, -1, NULL);
	g_free(contents);

	g_key_file_free(key_file);
	g_free(path);

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
	signals[s_ctcp] =
		g_signal_new("ctcp",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
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
	signals[s_mode] =
		g_signal_new("mode",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 6,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_motd] =
		g_signal_new("motd",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING,
		             G_TYPE_NONE, 3,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_nick] =
		g_signal_new("nick",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	signals[s_notice] =
		g_signal_new("notice",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING_STRING,
		             G_TYPE_NONE, 5,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
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
	signals[s_shutdown] =
		g_signal_new("shutdown",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64,
		             G_TYPE_NONE, 1,
		             G_TYPE_INT64);
	signals[s_topic] =
		g_signal_new("topic",
		             G_OBJECT_CLASS_TYPE(klass),
		             G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		             0, NULL, NULL,
		             g_cclosure_user_marshal_VOID__INT64_STRING_STRING_STRING,
		             G_TYPE_NONE, 4,
		             G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}
