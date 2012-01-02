/*
 * Copyright (c) 2009-2012 Michael Kuhn
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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "dbus_server.h"

#include "dbus.h"
#include "misc.h"

makiDBusServer* dbus_server = NULL;

struct maki_dbus_server
{
	GDBusServer* server;
	GDBusNodeInfo* introspection;

	GSList* connections;
};

void
maki_dbus_server_message_handler (GDBusConnection* connection, const gchar* sender, const gchar* path, const gchar* interface, const gchar* method, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer data)
{
	gchar* tmp;

	tmp = g_variant_print(parameters, TRUE);
	maki_debug("METHOD %s: %s %s %s\n", path, interface, method, tmp);
	g_free(tmp);

#if 0
	if (g_strcmp0(path, "/org/freedesktop/DBus") == 0
	    && g_strcmp0(interface, "org.freedesktop.DBus") == 0)
	{
		if (g_strcmp0(method, "AddMatch"))
		{
			const gchar* rule;

			g_variant_get(parameters, "(&s)", &rule);
			g_dbus_method_invocation_return_value(invocation, NULL);
		}
		else if (g_strcmp0(method, "Hello"))
		{
			const gchar* id = "dummy";

			g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", id));
		}

		return;
	}

	if (g_strcmp0(interface, SUSHI_DBUS_INTERFACE) != 0)
	{
		return;
	}
#endif

	if (g_strcmp0(method, "action") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &message);
		maki_dbus_action(server, channel, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "away") == 0)
	{
		const gchar* server;
		const gchar* message;

		g_variant_get(parameters, "(&s&s)", &server, &message);
		maki_dbus_away(server, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "back") == 0)
	{
		const gchar* server;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_back(server, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "channel_nicks") == 0)
	{
		const gchar* server;
		const gchar* channel;

		gchar** nicks;
		gchar** prefixes;

		g_variant_get(parameters, "(&s&s)", &server, &channel);
		maki_dbus_channel_nicks(server, channel, &nicks, &prefixes, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as^as)", nicks, prefixes));

		g_strfreev(nicks);
		g_strfreev(prefixes);
	}
	else if (g_strcmp0(method, "channel_topic") == 0)
	{
		const gchar* server;
		const gchar* channel;

		gchar* topic;

		g_variant_get(parameters, "(&s&s)", &server, &channel);
		maki_dbus_channel_topic(server, channel, &topic, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", topic));

		g_free(topic);
	}
	else if (g_strcmp0(method, "channels") == 0)
	{
		const gchar* server;

		gchar** channels;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_channels(server, &channels, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", channels));

		g_strfreev(channels);
	}
	else if (g_strcmp0(method, "config_get") == 0)
	{
		const gchar* group;
		const gchar* key;

		gchar* value;

		g_variant_get(parameters, "(&s&s)", &group, &key);
		maki_dbus_config_get(group, key, &value, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", value));

		g_free(value);
	}
	else if (g_strcmp0(method, "config_set") == 0)
	{
		const gchar* group;
		const gchar* key;
		const gchar* value;

		g_variant_get(parameters, "(&s&s&s)", &group, &key, &value);
		maki_dbus_config_set(group, key, value, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "connect") == 0)
	{
		const gchar* server;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_connect(server, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "ctcp") == 0)
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s)", &server, &target, &message);
		maki_dbus_ctcp(server, target, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "dcc_send") == 0)
	{
		const gchar* server;
		const gchar* target;
		const gchar* path_;

		g_variant_get(parameters, "(&s&s&s)", &server, &target, &path_);
		maki_dbus_dcc_send(server, target, path_, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "dcc_sends") == 0)
	{
		GVariantBuilder* builder[5];

		GArray* ids;
		gchar** servers;
		gchar** froms;
		gchar** filenames;
		GArray* sizes;
		GArray* progresses;
		GArray* speeds;
		GArray* statuses;

		maki_dbus_dcc_sends(&ids, &servers, &froms, &filenames, &sizes, &progresses, &speeds, &statuses, NULL);
		builder[0] = maki_variant_builder_array_uint64(ids);
		builder[1] = maki_variant_builder_array_uint64(sizes);
		builder[2] = maki_variant_builder_array_uint64(progresses);
		builder[3] = maki_variant_builder_array_uint64(speeds);
		builder[4] = maki_variant_builder_array_uint64(statuses);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(at^as^as^asatatatat)", builder[0], servers, froms, filenames, builder[1], builder[2], builder[3], builder[4]));
		g_variant_builder_unref(builder[0]);
		g_variant_builder_unref(builder[1]);
		g_variant_builder_unref(builder[2]);
		g_variant_builder_unref(builder[3]);
		g_variant_builder_unref(builder[4]);

		g_array_free(ids, TRUE);
		g_strfreev(servers);
		g_strfreev(froms);
		g_strfreev(filenames);
		g_array_free(sizes, TRUE);
		g_array_free(progresses, TRUE);
		g_array_free(speeds, TRUE);
		g_array_free(statuses, TRUE);
	}
	else if (g_strcmp0(method, "dcc_send_accept") == 0)
	{
		guint64 id;

		g_variant_get(parameters, "(t)", &id);
		maki_dbus_dcc_send_accept(id, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "dcc_send_get") == 0)
	{
		guint64 id;
		const gchar* key;

		gchar* value;

		g_variant_get(parameters, "(t&s)", &id, &key);
		maki_dbus_dcc_send_get(id, key, &value, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", value));

		g_free(value);
	}
	else if (g_strcmp0(method, "dcc_send_remove") == 0)
	{
		guint64 id;

		g_variant_get(parameters, "(t)", &id);
		maki_dbus_dcc_send_remove(id, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "dcc_send_resume") == 0)
	{
		guint64 id;

		g_variant_get(parameters, "(t)", &id);
		maki_dbus_dcc_send_resume(id, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "dcc_send_set") == 0)
	{
		guint64 id;
		const gchar* key;
		const gchar* value;

		g_variant_get(parameters, "(t&s&s)", &id, &key, &value);
		maki_dbus_dcc_send_set(id, key, value, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "ignore") == 0)
	{
		const gchar* server;
		const gchar* pattern;

		g_variant_get(parameters, "(&s&s)", &server, &pattern);
		maki_dbus_ignore(server, pattern, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "ignores") == 0)
	{
		const gchar* server;

		gchar** ignores;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_ignores(server, &ignores, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", ignores));

		g_strfreev(ignores);
	}
	else if (g_strcmp0(method, "invite") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* who;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &who);
		maki_dbus_invite(server, channel, who, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "join") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* key;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &key);
		maki_dbus_join(server, channel, key, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "kick") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* who;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s&s)", &server, &channel, &who, &message);
		maki_dbus_kick(server, channel, who, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "list") == 0)
	{
		const gchar* server;
		const gchar* channel;

		g_variant_get(parameters, "(&s&s)", &server, &channel);
		maki_dbus_list(server, channel, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "log") == 0)
	{
		const gchar* server;
		const gchar* target;
		guint64 lines;

		gchar** log;

		g_variant_get(parameters, "(&s&st)", &server, &target, &lines);
		maki_dbus_log(server, target, lines, &log, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", log));

		g_strfreev(log);
	}
	else if (g_strcmp0(method, "message") == 0)
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s)", &server, &target, &message);
		maki_dbus_message(server, target, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "mode") == 0)
	{
		const gchar* server;
		const gchar* target;
		const gchar* mode;

		g_variant_get(parameters, "(&s&s&s)", &server, &target, &mode);
		maki_dbus_mode(server, target, mode, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "names") == 0)
	{
		const gchar* server;
		const gchar* channel;

		g_variant_get(parameters, "(&s&s)", &server, &channel);
		maki_dbus_names(server, channel, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "nick") == 0)
	{
		const gchar* server;
		const gchar* nick;

		g_variant_get(parameters, "(&s&s)", &server, &nick);
		maki_dbus_nick(server, nick, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "nickserv") == 0)
	{
		const gchar* server;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_nickserv(server, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "notice") == 0)
	{
		const gchar* server;
		const gchar* target;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s)", &server, &target, &message);
		maki_dbus_notice(server, target, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "oper") == 0)
	{
		const gchar* server;
		const gchar* name;
		const gchar* password;

		g_variant_get(parameters, "(&s&s&s)", &server, &name, &password);
		maki_dbus_oper(server, name, password, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "part") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* message;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &message);
		maki_dbus_part(server, channel, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "quit") == 0)
	{
		const gchar* server;
		const gchar* message;

		g_variant_get(parameters, "(&s&s)", &server, &message);
		maki_dbus_quit(server, message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "raw") == 0)
	{
		const gchar* server;
		const gchar* command;

		g_variant_get(parameters, "(&s&s)", &server, &command);
		maki_dbus_raw(server, command, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "server_get") == 0)
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		gchar* value;

		g_variant_get(parameters, "(&s&s&s)", &server, &group, &key);
		maki_dbus_server_get(server, group, key, &value, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", value));

		g_free(value);
	}
	else if (g_strcmp0(method, "server_get_list") == 0)
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		gchar** list;

		g_variant_get(parameters, "(&s&s&s)", &server, &group, &key);
		maki_dbus_server_get_list(server, group, key, &list, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", list));

		g_strfreev(list);
	}
	else if (g_strcmp0(method, "server_list") == 0)
	{
		const gchar* server;
		const gchar* group;

		gchar** result;

		g_variant_get(parameters, "(&s&s)", &server, &group);
		maki_dbus_server_list(server, group, &result, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", result));

		g_strfreev(result);
	}
	else if (g_strcmp0(method, "server_remove") == 0)
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;

		g_variant_get(parameters, "(&s&s&s)", &server, &group, &key);
		maki_dbus_server_remove(server, group, key, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "server_rename") == 0)
	{
		const gchar* old;
		const gchar* new;

		g_variant_get(parameters, "(&s&s)", &old, &new);
		maki_dbus_server_rename(old, new, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "server_set") == 0)
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;
		const gchar* value;

		g_variant_get(parameters, "(&s&s&s&s)", &server, &group, &key, &value);
		maki_dbus_server_set(server, group, key, value, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "server_set_list") == 0)
	{
		const gchar* server;
		const gchar* group;
		const gchar* key;
		gchar** list;

		g_variant_get(parameters, "(&s&s&s^a&s)", &server, &group, &key, &list);
		maki_dbus_server_set_list(server, group, key, list, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);

		g_free(list);
	}
	else if (g_strcmp0(method, "servers") == 0)
	{
		gchar** servers;

		maki_dbus_servers(&servers, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", servers));

		g_strfreev(servers);
	}
	else if (g_strcmp0(method, "shutdown") == 0)
	{
		const gchar* message;

		g_variant_get(parameters, "(&s)", &message);
		maki_dbus_shutdown(message, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "support_chantypes") == 0)
	{
		const gchar* server;

		gchar* chantypes;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_support_chantypes(server, &chantypes, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", chantypes));

		g_free(chantypes);
	}
	else if (g_strcmp0(method, "support_prefix") == 0)
	{
		const gchar* server;

		gchar** prefix;

		g_variant_get(parameters, "(&s)", &server);
		maki_dbus_support_prefix(server, &prefix, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)", prefix));

		g_strfreev(prefix);
	}
	else if (g_strcmp0(method, "topic") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* topic;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &topic);
		maki_dbus_topic(server, channel, topic, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "unignore") == 0)
	{
		const gchar* server;
		const gchar* pattern;

		g_variant_get(parameters, "(&s&s)", &server, &pattern);
		maki_dbus_unignore(server, pattern, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "user_away") == 0)
	{
		const gchar* server;
		const gchar* nick;

		gboolean away;

		g_variant_get(parameters, "(&s&s)", &server, &nick);
		maki_dbus_user_away(server, nick, &away, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", away));
	}
	else if (g_strcmp0(method, "user_channel_mode") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* nick;

		gchar* mode;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &nick);
		maki_dbus_user_channel_mode(server, channel, nick, &mode, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", mode));

		g_free(mode);
	}
	else if (g_strcmp0(method, "user_channel_prefix") == 0)
	{
		const gchar* server;
		const gchar* channel;
		const gchar* nick;

		gchar* prefix;

		g_variant_get(parameters, "(&s&s&s)", &server, &channel, &nick);
		maki_dbus_user_channel_prefix(server, channel, nick, &prefix, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", prefix));

		g_free(prefix);
	}
	else if (g_strcmp0(method, "user_from") == 0)
	{
		const gchar* server;
		const gchar* nick;

		gchar* from;

		g_variant_get(parameters, "(&s&s)", &server, &nick);
		maki_dbus_user_from(server, nick, &from, NULL);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", from));

		g_free(from);
	}
	else if (g_strcmp0(method, "version") == 0)
	{
		GVariantBuilder* builder;

		GArray* version;

		maki_dbus_version(&version, NULL);
		builder = maki_variant_builder_array_uint64(version);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(at)", builder));
		g_variant_builder_unref(builder);

		g_array_free(version, TRUE);
	}
	else if (g_strcmp0(method, "who") == 0)
	{
		const gchar* server;
		const gchar* mask;
		gboolean operators_only;

		g_variant_get(parameters, "(&s&sb)", &server, &mask, &operators_only);
		maki_dbus_who(server, mask, operators_only, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else if (g_strcmp0(method, "whois") == 0)
	{
		const gchar* server;
		const gchar* mask;

		g_variant_get(parameters, "(&s&s)", &server, &mask);
		maki_dbus_whois(server, mask, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	else
	{
		g_dbus_method_invocation_return_dbus_error(invocation, "de.sushi.ikkoku.error", "No such method.");
	}
}

static void
maki_dbus_server_connection_closed_cb (GDBusConnection* connection, gboolean peer_vanished, GError* error, gpointer data)
{
	makiDBusServer* dserv = data;

	maki_debug("disconnected %p\n", (gpointer)connection);

	dserv->connections = g_slist_remove(dserv->connections, connection);
	g_object_unref(connection);
}

static gboolean
maki_dbus_server_new_connection_cb (GDBusServer* server, GDBusConnection* connection, gpointer data)
{
	GDBusInterfaceVTable vtable = {
		maki_dbus_server_message_handler,
		NULL,
		NULL
	};

	makiDBusServer* dserv = data;

	maki_debug("new connection %p\n", (gpointer)connection);

	g_object_ref(connection);

	g_dbus_connection_register_object(connection, SUSHI_DBUS_PATH, g_dbus_node_info_lookup_interface(dserv->introspection, SUSHI_DBUS_INTERFACE), &vtable, dserv, NULL, NULL);
#if 0
	g_dbus_connection_register_object(connection, "/org/freedesktop/DBus", dserv->introspection->interfaces[0], &vtable, dserv, NULL, NULL);
#endif

	dserv->connections = g_slist_prepend(dserv->connections, connection);

	g_signal_connect(connection, "closed", G_CALLBACK(maki_dbus_server_connection_closed_cb), dserv);

	return TRUE;
}

makiDBusServer*
maki_dbus_server_new (void)
{
	gchar* path;
	gchar* guid;
	gchar* introspection_xml;
	GDBusServer* server;
	GDBusServerFlags server_flags;
	makiDBusServer* dserv = NULL;

	path = g_build_filename(MAKI_SHARE_DIRECTORY, "dbus.xml", NULL);
	guid = g_dbus_generate_guid();

	server_flags = G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
#if GLIB_CHECK_VERSION(2,28,7)
	server_flags |= G_DBUS_SERVER_FLAGS_RUN_IN_THREAD;
#endif

	if ((server = g_dbus_server_new_sync("tcp:host=0.0.0.0", server_flags, guid, NULL, NULL, NULL)) != NULL
	    && g_file_get_contents(path, &introspection_xml, NULL, NULL))
	{
		dserv = g_new(makiDBusServer, 1);
		dserv->server = server;
		dserv->introspection = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
		dserv->connections = NULL;

		g_free(introspection_xml);

		maki_debug("server at %s\n", g_dbus_server_get_client_address(dserv->server));

		g_signal_connect(dserv->server, "new-connection", G_CALLBACK(maki_dbus_server_new_connection_cb), dserv);
		g_dbus_server_start(dserv->server);
	}

	g_free(path);
	g_free(guid);

	return dserv;
}

void
maki_dbus_server_free (makiDBusServer* dserv)
{
	GSList* list;

	for (list = dserv->connections; list != NULL; list = list->next)
	{
		GDBusConnection* connection = list->data;

		g_dbus_connection_close_sync(connection, NULL, NULL);
	}

	g_slist_free(dserv->connections);

	g_dbus_server_stop(dserv->server);
	g_object_unref(dserv->server);

	g_dbus_node_info_unref(dserv->introspection);

	g_free(dserv);
}

const gchar*
maki_dbus_server_address (makiDBusServer* dserv)
{
	return g_dbus_server_get_client_address(dserv->server);
}

void
maki_dbus_server_emit (makiDBusServer* dserv, const gchar* name, GVariant* variant)
{
	GDBusMessage* message;
	GSList* list;

	message = g_dbus_message_new_signal(SUSHI_DBUS_PATH, SUSHI_DBUS_INTERFACE, name);
	g_dbus_message_set_sender(message, SUSHI_DBUS_SERVICE);
	g_dbus_message_set_body(message, variant);

	for (list = dserv->connections; list != NULL; list = list->next)
	{
		GDBusConnection* connection = list->data;

		g_dbus_connection_send_message(connection, message, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
	}

	g_object_unref(message);
}
