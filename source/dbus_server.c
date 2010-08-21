/*
 * Copyright (c) 2009-2010 Michael Kuhn
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

#include "dbus_server.h"
#include "misc.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

struct maki_dbus_server
{
	DBusServer* server;

	gchar* address;

	GMainContext* main_context;
	GMainLoop* main_loop;
	GThread* thread;

	GSList* connections;

	gchar* introspection;
};

static gpointer
maki_dbus_server_thread (gpointer data)
{
	makiDBusServer* dserv = data;

	g_main_loop_run(dserv->main_loop);

	return NULL;
}

static gboolean
maki_dbus_server_reply (DBusConnection* connection, DBusMessage* message, gint type, ...)
{
	va_list ap;
	DBusMessage* reply;

	va_start(ap, type);

	if ((reply = dbus_message_new_method_return(message)) == NULL)
	{
		goto error;
	}

	if (type != DBUS_TYPE_INVALID)
	{
		if (!dbus_message_append_args_valist(reply, type, ap))
		{
			goto error;
		}
	}

	if (!dbus_connection_send(connection, reply, NULL))
	{
		goto error;
	}

	dbus_message_unref(reply);

	va_end(ap);

	return TRUE;

error:
	if (reply != NULL)
	{
		dbus_message_unref(reply);
	}

	va_end(ap);

	return FALSE;
}

static DBusHandlerResult
maki_dbus_server_message_handler (DBusConnection* connection, DBusMessage* msg, void* data)
{
	gboolean got_args;
	gboolean sent_reply;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	makiDBusServer* dserv = data;

	maki_debug("METHOD %s: %s %s\n", dbus_message_get_path(msg), dbus_message_get_interface(msg), dbus_message_get_member(msg));

	if (strcmp(dbus_message_get_path(msg), DBUS_PATH_LOCAL) == 0)
	{
		if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
		{
			dserv->connections = g_slist_remove(dserv->connections, connection);

			dbus_connection_unref(connection);

			return DBUS_HANDLER_RESULT_HANDLED;
		}

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

#if 0
	if (strcmp(dbus_message_get_path(msg), DBUS_PATH_DBUS) == 0)
	{
		if (dbus_message_is_method_call(msg, DBUS_INTERFACE_DBUS, "AddMatch"))
		{
			const gchar* rule;

			got_args = dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &rule,
					DBUS_TYPE_INVALID);

			if (!got_args)
			{
				goto error;
			}

			sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

			if (!sent_reply)
			{
				goto error;
			}

			return DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_DBUS, "Hello"))
		{
			const gchar* id = "dummy";

			sent_reply = maki_dbus_server_reply(connection, msg,
				DBUS_TYPE_STRING, &id,
				DBUS_TYPE_INVALID);

			if (!sent_reply)
			{
				goto error;
			}

			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}
#endif

	if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")
	    && dserv->introspection != NULL)
	{
		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &(dserv->introspection),
			DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "action"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_action(dbus, server, channel, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "away"))
	{
		const gchar* server;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_away(dbus, server, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "back"))
	{
		const gchar* server;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_back(dbus, server, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "channel_nicks"))
	{
		const gchar* server;
		const gchar* channel;

		gchar** nicks;
		gchar** prefixes;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_channel_nicks(dbus, server, channel, &nicks, &prefixes, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &nicks, g_strv_length(nicks),
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &prefixes, g_strv_length(prefixes),
			DBUS_TYPE_INVALID);

		g_strfreev(nicks);
		g_strfreev(prefixes);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "channel_topic"))
	{
		const gchar* server;
		const gchar* channel;

		gchar* topic;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_channel_topic(dbus, server, channel, &topic, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &topic,
			DBUS_TYPE_INVALID);

		g_free(topic);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "channels"))
	{
		const gchar* server;

		gchar** channels;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_channels(dbus, server, &channels, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &channels, g_strv_length(channels),
			DBUS_TYPE_INVALID);

		g_strfreev(channels);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "config_get"))
	{
		const gchar* group;
		const gchar* key;

		gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_config_get(dbus, group, key, &value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		g_free(value);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "config_set"))
	{
		const gchar* group;
		const gchar* key;
		const gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_config_set(dbus, group, key, value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "connect"))
	{
		const gchar* server;

		got_args = dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &server, DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_connect(dbus, server, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "ctcp"))
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_ctcp(dbus, server, target, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send"))
	{
		const gchar* server;
		const gchar* target;
		const gchar* path;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &path,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send(dbus, server, target, path, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_sends"))
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

		sent_reply = maki_dbus_server_reply(connection, msg,
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

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send_accept"))
	{
		guint64 id;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send_accept(dbus, id, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send_get"))
	{
		guint64 id;
		const gchar* key;

		gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send_get(dbus, id, key, &value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		g_free(value);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send_remove"))
	{
		guint64 id;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send_remove(dbus, id, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send_resume"))
	{
		guint64 id;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send_resume(dbus, id, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "dcc_send_set"))
	{
		guint64 id;
		const gchar* key;
		const gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_UINT64, &id,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_dcc_send_set(dbus, id, key, value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "ignore"))
	{
		const gchar* server;
		const gchar* pattern;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &pattern,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_ignore(dbus, server, pattern, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "ignores"))
	{
		const gchar* server;

		gchar** ignores;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_ignores(dbus, server, &ignores, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &ignores, g_strv_length(ignores),
			DBUS_TYPE_INVALID);

		g_strfreev(ignores);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "invite"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* who;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &who,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_invite(dbus, server, channel, who, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "join"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* key;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_join(dbus, server, channel, key, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "kick"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* who;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &who,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_kick(dbus, server, channel, who, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "list"))
	{
		const gchar* server;
		const gchar* channel;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_list(dbus, server, channel, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "log"))
	{
		const gchar* server;
		const gchar* target;
		guint64 lines;

		gchar** log;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_UINT64, &lines,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_log(dbus, server, target, lines, &log, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &log, g_strv_length(log),
			DBUS_TYPE_INVALID);

		g_strfreev(log);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "message"))
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_message(dbus, server, target, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "mode"))
	{
		const gchar* server;
		const gchar* target;
		const gchar* mode;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &mode,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_mode(dbus, server, target, mode, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "names"))
	{
		const gchar* server;
		const gchar* channel;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_names(dbus, server, channel, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "nick"))
	{
		const gchar* server;
		const gchar* nick;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_nick(dbus, server, nick, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "nickserv"))
	{
		const gchar* server;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_nickserv(dbus, server, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "notice"))
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &target,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_notice(dbus, server, target, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "oper"))
	{
		const gchar* server;
		const gchar* name;
		const gchar* password;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &name,
			DBUS_TYPE_STRING, &password,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_oper(dbus, server, name, password, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "part"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_part(dbus, server, channel, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "quit"))
	{
		const gchar* server;
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_quit(dbus, server, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "raw"))
	{
		const gchar* server;
		const gchar* command;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &command,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_raw(dbus, server, command, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_get"))
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_get(dbus, server, group, key, &value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		g_free(value);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_get_list"))
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		gchar** list;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_get_list(dbus, server, group, key, &list, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &list, g_strv_length(list),
			DBUS_TYPE_INVALID);

		g_strfreev(list);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_list"))
	{
		const gchar* server;
		const gchar* group;

		gchar** result;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_list(dbus, server, group, &result, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &result, g_strv_length(result),
			DBUS_TYPE_INVALID);

		g_strfreev(result);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_remove"))
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_remove(dbus, server, group, key, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_rename"))
	{
		const gchar* old;
		const gchar* new;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &old,
			DBUS_TYPE_STRING, &new,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_rename(dbus, old, new, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_set"))
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;
		const gchar* value;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_STRING, &value,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_server_set(dbus, server, group, key, value, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "server_set_list"))
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;
		gchar** list = NULL;
		gint list_len;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &group,
			DBUS_TYPE_STRING, &key,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &list, &list_len,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			dbus_free_string_array(list);

			goto error;
		}

		maki_dbus_server_set_list(dbus, server, group, key, list, NULL);

		dbus_free_string_array(list);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "servers"))
	{
		gchar** servers;

		maki_dbus_servers(dbus, &servers, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &servers, g_strv_length(servers),
			DBUS_TYPE_INVALID);

		g_strfreev(servers);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "shutdown"))
	{
		const gchar* message;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &message,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_shutdown(dbus, message, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "support_chantypes"))
	{
		const gchar* server;

		gchar* chantypes;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_support_chantypes(dbus, server, &chantypes, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &chantypes,
			DBUS_TYPE_INVALID);

		g_free(chantypes);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "support_prefix"))
	{
		const gchar* server;

		gchar** prefix;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_support_prefix(dbus, server, &prefix, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &prefix, g_strv_length(prefix),
			DBUS_TYPE_INVALID);

		g_strfreev(prefix);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "topic"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* topic;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &topic,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_topic(dbus, server, channel, topic, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "unignore"))
	{
		const gchar* server;
		const gchar* pattern;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &pattern,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_unignore(dbus, server, pattern, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "user_away"))
	{
		const gchar* server;
		const gchar* nick;

		gboolean away;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_user_away(dbus, server, nick, &away, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_BOOLEAN, &away,
			DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "user_channel_mode"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* nick;

		gchar* mode;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_user_channel_mode(dbus, server, channel, nick, &mode, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &mode,
			DBUS_TYPE_INVALID);

		g_free(mode);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "user_channel_prefix"))
	{
		const gchar* server;
		const gchar* channel;
		const gchar* nick;

		gchar* prefix;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &channel,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_user_channel_prefix(dbus, server, channel, nick, &prefix, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &prefix,
			DBUS_TYPE_INVALID);

		g_free(prefix);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "user_from"))
	{
		const gchar* server;
		const gchar* nick;

		gchar* from;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &nick,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_user_from(dbus, server, nick, &from, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_STRING, &from,
			DBUS_TYPE_INVALID);

		g_free(from);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "version"))
	{
		GArray* version;

		maki_dbus_version(dbus, &version, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_UINT64, &(version->data), version->len,
			DBUS_TYPE_INVALID);

		g_array_free(version, TRUE);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "who"))
	{
		const gchar* server;
		const gchar* mask;
		gboolean operators_only;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &mask,
			DBUS_TYPE_BOOLEAN, &operators_only,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_who(dbus, server, mask, operators_only, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}
	else if (dbus_message_is_method_call(msg, SUSHI_DBUS_INTERFACE, "whois"))
	{
		const gchar* server;
		const gchar* mask;

		got_args = dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &server,
			DBUS_TYPE_STRING, &mask,
			DBUS_TYPE_INVALID);

		if (!got_args)
		{
			goto error;
		}

		maki_dbus_whois(dbus, server, mask, NULL);

		sent_reply = maki_dbus_server_reply(connection, msg, DBUS_TYPE_INVALID);

		if (!sent_reply)
		{
			goto error;
		}

		ret = DBUS_HANDLER_RESULT_HANDLED;
	}

	return ret;

error:
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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

	maki_debug("new connection %p\n", (gpointer)connection);

	dbus_connection_set_allow_anonymous(connection, TRUE);

	dbus_connection_register_object_path(connection, SUSHI_DBUS_PATH, &vtable, dserv);
	dbus_connection_register_fallback(connection, DBUS_PATH_LOCAL, &vtable, dserv);
#if 0
	dbus_connection_register_fallback(connection, DBUS_PATH_DBUS, &vtable, dserv);
#endif

	dbus_connection_ref(connection);
	dbus_connection_setup_with_g_main(connection, dserv->main_context);

	dserv->connections = g_slist_prepend(dserv->connections, connection);
}

makiDBusServer*
maki_dbus_server_new (void)
{
	gchar* address;
	gchar* path;
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

	path = g_build_filename(MAKI_SHARE_DIRECTORY, "dbus.xml", NULL);

	if (!g_file_get_contents(path, &(dserv->introspection), NULL, NULL))
	{
		dserv->introspection = NULL;
		maki_debug("Introspection disabled");
	}

	g_free(path);

	maki_debug("server at %s\n", dserv->address);

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

	g_free(dserv->introspection);
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

		maki_debug("SIGNAL /de/ikkoku/sushi: de.ikkoku.sushi %s\n", name);

		va_start(ap, type);

		message = dbus_message_new_signal(SUSHI_DBUS_PATH, SUSHI_DBUS_INTERFACE, name);
		dbus_message_set_sender(message, SUSHI_DBUS_SERVICE);
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

