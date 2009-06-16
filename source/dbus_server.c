/*
 * Copyright (c) 2009 Michael Kuhn
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

struct maki_dbus_server
{
	DBusServer* server;

	gchar* address;

	GMainContext* main_context;
	GMainLoop* main_loop;
	GThread* thread;

	GSList* connections;
};

static gpointer
maki_dbus_server_thread (gpointer data)
{
	makiDBusServer* dserv = data;

	g_main_loop_run(dserv->main_loop);

	return NULL;
}

static void
maki_dbus_server_reply (DBusConnection* connection, DBusMessage* message, gint type, ...)
{
	va_list ap;
	DBusMessage* reply;

	va_start(ap, type);

	reply = dbus_message_new_method_return(message);

	if (type != DBUS_TYPE_INVALID)
	{
		dbus_message_append_args_valist(reply, type, ap);
	}

	dbus_connection_send(connection, reply, NULL);
	dbus_message_unref(reply);

	va_end(ap);
}

static DBusHandlerResult
maki_dbus_server_message_handler (DBusConnection* connection, DBusMessage* msg, void* data)
{
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	makiDBusServer* dserv = data;

	g_print("METHOD %s: %s %s\n", dbus_message_get_path(msg), dbus_message_get_interface(msg), dbus_message_get_member(msg));

	if (strcmp(dbus_message_get_path(msg), DBUS_PATH_LOCAL) == 0)
	{
		if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
		{
			dserv->connections = g_slist_remove(dserv->connections, connection);
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	/* FIXME error handling */
	if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "action"))
	{
		gchar* server;
		gchar* channel;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_action(dbus, server, channel, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "away"))
	{
		gchar* server;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_away(dbus, server, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "back"))
	{
		gchar* server;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_back(dbus, server, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "channel_nicks"))
	{
		gchar* server;
		gchar* channel;

		gchar** nicks;
		gchar** prefixes;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		maki_dbus_channel_nicks(dbus, server, channel, &nicks, &prefixes, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &nicks, g_strv_length(nicks),
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &prefixes, g_strv_length(prefixes),
			DBUS_TYPE_INVALID);

		g_strfreev(nicks);
		g_strfreev(prefixes);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "channels"))
	{
		gchar* server;

		gchar** channels;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_channels(dbus, server, &channels, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &channels, g_strv_length(channels),
			DBUS_TYPE_INVALID);

		g_strfreev(channels);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "config_get"))
	{
		gchar* group;
		gchar* key;

		gchar* value;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		maki_dbus_config_get(dbus, group, key, &value, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		g_free(key);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "config_set"))
	{
		gchar* group;
		gchar* key;
		gchar* value;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		maki_dbus_config_set(dbus, group, key, value, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "connect"))
	{
		gchar* server;

		dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &server, DBUS_TYPE_INVALID);

		maki_dbus_connect(dbus, server, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "ctcp"))
	{
		gchar* server;
		gchar* target;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_ctcp(dbus, server, target, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "dcc_send"))
	{
		gchar* server;
		gchar* target;
		gchar* path;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &path,
			DBUS_TYPE_INVALID);

		maki_dbus_dcc_send(dbus, server, target, path, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "dcc_sends"))
	{
		GArray* ids;
		gchar** servers;
		gchar** froms;
		gchar** filenames;
		GArray* sizes;
		GArray* progresses;
		GArray* speeds;
		GArray* statuses;

		maki_dbus_dcc_sends(dbus, &ids, &servers, &froms, &filenames, &sizes, &progresses, &speeds, &statuses, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(ids->data), ids->len,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &servers, g_strv_length(servers),
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &froms, g_strv_length(froms),
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &filenames, g_strv_length(filenames),
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(sizes->data), sizes->len,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(progresses->data), progresses->len,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(speeds->data), speeds->len,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(statuses->data), statuses->len,
			DBUS_TYPE_INVALID);

		g_array_free(ids, TRUE);
		g_strfreev(servers);
		g_strfreev(froms);
		g_strfreev(filenames);
		g_array_free(sizes, TRUE);
		g_array_free(progresses, TRUE);
		g_array_free(speeds, TRUE);
		g_array_free(statuses, TRUE);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "dcc_send_accept"))
	{
		guint64 id;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_INVALID);

		maki_dbus_dcc_send_accept(dbus, id, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "dcc_send_remove"))
	{
		guint64 id;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_INVALID);

		maki_dbus_dcc_send_remove(dbus, id, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "ignore"))
	{
		gchar* server;
		gchar* pattern;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &pattern,
			DBUS_TYPE_INVALID);

		maki_dbus_ignore(dbus, server, pattern, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "ignores"))
	{
		gchar* server;

		gchar** ignores;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_ignores(dbus, server, &ignores, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &ignores, g_strv_length(ignores),
			DBUS_TYPE_INVALID);

		g_strfreev(ignores);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "invite"))
	{
		gchar* server;
		gchar* channel;
		gchar* who;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &who,
			DBUS_TYPE_INVALID);

		maki_dbus_invite(dbus, server, channel, who, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "join"))
	{
		gchar* server;
		gchar* channel;
		gchar* key;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		maki_dbus_join(dbus, server, channel, key, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "kick"))
	{
		gchar* server;
		gchar* channel;
		gchar* who;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &who,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_kick(dbus, server, channel, who, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "list"))
	{
		gchar* server;
		gchar* channel;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		maki_dbus_list(dbus, server, channel, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "log"))
	{
		gchar* server;
		gchar* target;
		guint64 lines;

		gchar** log;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_UINT64, &lines,
			DBUS_TYPE_INVALID);

		maki_dbus_log(dbus, server, target, lines, &log, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &log, g_strv_length(log),
			DBUS_TYPE_INVALID);

		g_strfreev(log);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "message"))
	{
		gchar* server;
		gchar* target;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_message(dbus, server, target, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "mode"))
	{
		gchar* server;
		gchar* target;
		gchar* mode;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &mode,
			DBUS_TYPE_INVALID);

		maki_dbus_mode(dbus, server, target, mode, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "names"))
	{
		gchar* server;
		gchar* channel;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		maki_dbus_names(dbus, server, channel, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "nick"))
	{
		gchar* server;
		gchar* nick;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		maki_dbus_nick(dbus, server, nick, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "nickserv"))
	{
		gchar* server;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_nickserv(dbus, server, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "notice"))
	{
		gchar* server;
		gchar* target;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_notice(dbus, server, target, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "oper"))
	{
		gchar* server;
		gchar* name;
		gchar* password;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &name,
			DBUS_TYPE_STRING, &password,
			DBUS_TYPE_INVALID);

		maki_dbus_oper(dbus, server, name, password, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "part"))
	{
		gchar* server;
		gchar* channel;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_part(dbus, server, channel, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "quit"))
	{
		gchar* server;
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_quit(dbus, server, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "raw"))
	{
		gchar* server;
		gchar* command;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &command,
			DBUS_TYPE_INVALID);

		maki_dbus_raw(dbus, server, command, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_get"))
	{
		gchar* server;
		gchar* group;
		gchar* key;

		gchar* value;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		maki_dbus_server_get(dbus, server, group, key, &value, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		g_free(key);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_get_list"))
	{
		gchar* server;
		gchar* group;
		gchar* key;

		gchar** list;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		maki_dbus_server_get_list(dbus, server, group, key, &list, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &list, g_strv_length(list),
			DBUS_TYPE_INVALID);

		g_strfreev(list);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_list"))
	{
		gchar* server;
		gchar* group;

		gchar** result;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_INVALID);

		maki_dbus_server_list(dbus, server, group, &result, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &result, g_strv_length(result),
			DBUS_TYPE_INVALID);

		g_strfreev(result);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_remove"))
	{
		gchar* server;
		gchar* group;
		gchar* key;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		maki_dbus_server_remove(dbus, server, group, key, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_rename"))
	{
		gchar* old;
		gchar* new;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &old,
			DBUS_TYPE_STRING, &new,
			DBUS_TYPE_INVALID);

		maki_dbus_server_rename(dbus, old, new, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_set"))
	{
		gchar* server;
		gchar* group;
		gchar* key;
		gchar* value;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		maki_dbus_server_set(dbus, server, group, key, value, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "server_set_list"))
	{
		gchar* server;
		gchar* group;
		gchar* key;
		gchar** list;
		gint list_len;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &list, &list_len,
			DBUS_TYPE_INVALID);

		maki_dbus_server_set_list(dbus, server, group, key, list, NULL);

		dbus_free_string_array(list);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "servers"))
	{
		gchar** servers;

		maki_dbus_servers(dbus, &servers, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &servers, g_strv_length(servers),
			DBUS_TYPE_INVALID);

		g_strfreev(servers);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "shutdown"))
	{
		gchar* message;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		maki_dbus_shutdown(dbus, message, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "support_chantypes"))
	{
		gchar* server;

		gchar* chantypes;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_support_chantypes(dbus, server, &chantypes, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &chantypes,
			DBUS_TYPE_INVALID);

		g_free(chantypes);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "support_prefix"))
	{
		gchar* server;

		gchar** prefix;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		maki_dbus_support_prefix(dbus, server, &prefix, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &prefix, g_strv_length(prefix),
			DBUS_TYPE_INVALID);

		g_strfreev(prefix);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "topic"))
	{
		gchar* server;
		gchar* channel;
		gchar* topic;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &topic,
			DBUS_TYPE_INVALID);

		maki_dbus_topic(dbus, server, channel, topic, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "unignore"))
	{
		gchar* server;
		gchar* pattern;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &pattern,
			DBUS_TYPE_INVALID);

		maki_dbus_unignore(dbus, server, pattern, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "user_away"))
	{
		gchar* server;
		gchar* nick;

		gboolean away;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		maki_dbus_user_away(dbus, server, nick, &away, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_BOOLEAN, &away,
			DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "user_channel_mode"))
	{
		gchar* server;
		gchar* channel;
		gchar* nick;

		gchar* mode;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		maki_dbus_user_channel_mode(dbus, server, channel, nick, &mode, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &mode,
			DBUS_TYPE_INVALID);

		g_free(mode);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "user_channel_prefix"))
	{
		gchar* server;
		gchar* channel;
		gchar* nick;

		gchar* prefix;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		maki_dbus_user_channel_prefix(dbus, server, channel, nick, &prefix, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &prefix,
			DBUS_TYPE_INVALID);

		g_free(prefix);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "version"))
	{
		GArray* version;

		maki_dbus_version(dbus, &version, NULL);

		maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(version->data), version->len,
			DBUS_TYPE_INVALID);

		g_array_free(version, TRUE);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "who"))
	{
		gchar* server;
		gchar* mask;
		gboolean operators_only;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &mask,
			DBUS_TYPE_BOOLEAN, &operators_only,
			DBUS_TYPE_INVALID);

		maki_dbus_who(dbus, server, mask, operators_only, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, "de.ikkoku.sushi", "whois"))
	{
		gchar* server;
		gchar* mask;

		dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &mask,
			DBUS_TYPE_INVALID);

		maki_dbus_whois(dbus, server, mask, NULL);

		maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}

	return ret;
}

static void
maki_dbus_server_new_connection_cb (DBusServer* server, DBusConnection* connection, void* data)
{
	DBusObjectPathVTable vtable = {
		NULL,
		maki_dbus_server_message_handler,
		NULL, NULL, NULL, NULL
	};

	makiDBusServer* dserv = data;

	g_print("new connection %p\n", connection);

	dbus_connection_set_allow_anonymous(connection, TRUE);

	dbus_connection_register_object_path(connection, "/de/ikkoku/sushi", &vtable, dserv);
	dbus_connection_register_fallback(connection, DBUS_PATH_LOCAL, &vtable, dserv);

	dbus_connection_ref(connection);
	dbus_connection_setup_with_g_main(connection, dserv->main_context);

	dserv->connections = g_slist_prepend(dserv->connections, connection);
}

makiDBusServer*
maki_dbus_server_new (void)
{
	gchar* address;
	DBusError error;
	DBusServer* server;
	makiDBusServer* dserv;

	dserv = g_new(makiDBusServer, 1);

	dbus_error_init(&error);

	if ((server = dbus_server_listen("tcp:host=localhost,port=0", &error)) == NULL)
	{
		return NULL;
	}

	dserv->server = server;

	address = dbus_server_get_address(server);
	dserv->address = g_strdup(address);
	dbus_free(address);

	dserv->main_context = g_main_context_new();
	dserv->main_loop = g_main_loop_new(dserv->main_context, FALSE);
	dserv->thread = g_thread_create(maki_dbus_server_thread, dserv, TRUE, NULL);

	dserv->connections = NULL;

	g_print("server at %s\n", dserv->address);

	dbus_server_setup_with_g_main(server, dserv->main_context);
	dbus_server_set_new_connection_function(server, maki_dbus_server_new_connection_cb, dserv, NULL);

	return dserv;
}

void
maki_dbus_server_free (makiDBusServer* dserv)
{
	GSList* list;

	g_main_loop_quit(dserv->main_loop);
	g_thread_join(dserv->thread);

	g_main_loop_unref(dserv->main_loop);
	g_main_context_unref(dserv->main_context);

	for (list = dserv->connections; list != NULL; list = list->next)
	{
		DBusConnection* connection = list->data;

		dbus_connection_close(connection);
		dbus_connection_unref(connection);
	}

	g_slist_free(dserv->connections);

	dbus_server_disconnect(dserv->server);
	dbus_server_unref(dserv->server);

	g_free(dserv->address);
	g_free(dserv);
}

const gchar*
maki_dbus_server_address (makiDBusServer* dserv)
{
	return dserv->address;
}

void
maki_dbus_server_emit (makiDBusServer* dserv, const gchar* name, gint type, ...)
{
	if (dserv != NULL)
	{
		va_list ap;
		DBusMessage* message;
		GSList* list;

		g_print("SIGNAL /de/ikkoku/sushi: de.ikkoku.sushi %s\n", name);

		va_start(ap, type);

		message = dbus_message_new_signal("/de/ikkoku/sushi", "de.ikkoku.sushi", name);
		dbus_message_set_sender(message, "de.ikkoku.sushi");
		dbus_message_append_args_valist(message, type, ap);

		for (list = dserv->connections; list != NULL; list = list->next)
		{
			DBusConnection* connection = list->data;

			dbus_connection_send(connection, message, NULL);
		}

		dbus_message_unref(message);

		va_end(ap);
	}
}

