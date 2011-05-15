/*
 * Copyright (c) 2008-2011 Michael Kuhn
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
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <string.h>

#include <ilib.h>

#include "dbus.h"

#include "dbus_server.h"
#include "instance.h"
#include "maki.h"
#include "misc.h"
#include "out.h"
#include "server.h"

makiDBus* dbus = NULL;

struct maki_dbus
{
	guint id;
	GDBusConnection* connection;
	GDBusNodeInfo* introspection;
};

static void
maki_dbus_on_bus_acquired (GDBusConnection* connection, const gchar* name, gpointer data)
{
	GDBusInterfaceVTable vtable = {
		maki_dbus_server_message_handler,
		NULL,
		NULL
	};

	makiDBus* d = data;

	g_dbus_connection_register_object(connection, SUSHI_DBUS_PATH, g_dbus_node_info_lookup_interface(d->introspection, SUSHI_DBUS_INTERFACE), &vtable, d, NULL, NULL);
}

static void
maki_dbus_on_name_acquired (GDBusConnection* connection, const gchar* name, gpointer data)
{
	makiDBus* d = data;

	d->connection = connection;
}

static void
maki_dbus_on_name_lost (GDBusConnection* connection, const gchar* name, gpointer data)
{
	makiDBus* d = data;

	d->connection = NULL;
}

makiDBus*
maki_dbus_new (void)
{
	makiDBus* d = NULL;
	gchar* path;
	gchar* introspection_xml;

	path = g_build_filename(MAKI_SHARE_DIRECTORY, "dbus.xml", NULL);

	if (g_file_get_contents(path, &introspection_xml, NULL, NULL))
	{
		d = g_new(makiDBus, 1);
		d->id = g_bus_own_name(G_BUS_TYPE_SESSION, SUSHI_DBUS_SERVICE, G_BUS_NAME_OWNER_FLAGS_NONE,
				maki_dbus_on_bus_acquired, maki_dbus_on_name_acquired, maki_dbus_on_name_lost,
				d, NULL);
		d->connection = NULL;
		d->introspection = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

		g_free(introspection_xml);
	}

	g_free(path);

	return d;
}

void
maki_dbus_free (makiDBus* d)
{
	g_bus_unown_name(d->id);

	g_dbus_node_info_unref(d->introspection);

	g_free(d);
}

void
maki_dbus_emit (makiDBus* d, const gchar* name, const gchar* format, va_list ap)
{
	g_return_if_fail(d != NULL);

	if (d->connection != NULL)
	{
		va_list aq;

		va_copy(aq, ap);
		g_dbus_connection_emit_signal(d->connection, NULL, SUSHI_DBUS_PATH, SUSHI_DBUS_INTERFACE, name, g_variant_new_va(format, NULL, &aq), NULL);
		va_end(aq);
	}
}

static void
maki_dbus_emit_helper (const gchar* name, const gchar* format, ...)
{
	va_list ap;

	va_start(ap, format);

	if (dbus != NULL)
	{
		maki_dbus_emit(dbus, name, format, ap);
	}

	if (dbus_server != NULL)
	{
		maki_dbus_server_emit(dbus_server, name, format, ap);
	}

	va_end(ap);
}

void maki_dbus_emit_action (const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("action", "(xssss)",
		timestamp,
		server,
		nick,
		target,
		message);
}

void maki_dbus_emit_away (const gchar* server)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("away", "(xs)",
		timestamp,
		server);
}

void maki_dbus_emit_away_message (const gchar* server, const gchar* nick, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("away_message", "(xsss)",
		timestamp,
		server,
		nick,
		message);
}

void maki_dbus_emit_back (const gchar* server)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("back", "(xs)",
		timestamp,
		server);
}

void maki_dbus_emit_banlist (const gchar* server, const gchar* channel, const gchar* mask, const gchar* who, gint64 when)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("banlist", "(xssssx)",
		timestamp,
		server,
		channel,
		mask,
		who,
		when);
}

void maki_dbus_emit_cannot_join (const gchar* server, const gchar* channel, const gchar* reason)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("cannot_join", "(xsss)",
		timestamp,
		server,
		channel,
		reason);
}

void maki_dbus_emit_connect (const gchar* server)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("connect", "(xs)",
		timestamp,
		server);
}

void maki_dbus_emit_connected (const gchar* server)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("connected", "(xs)",
		timestamp,
		server);
}

void maki_dbus_emit_ctcp (const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("ctcp", "(xssss)",
		timestamp,
		server,
		nick,
		target,
		message);
}

void maki_dbus_emit_dcc_send (guint64 id, const gchar* server, const gchar* from, const gchar* filename, guint64 size, guint64 progress, guint64 speed, guint64 status)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("dcc_send", "(xtssstttt)",
		timestamp,
		id,
		server,
		from,
		filename,
		size,
		progress,
		speed,
		status);
}

void maki_dbus_emit_error (const gchar* server, const gchar* domain, const gchar* reason, gchar** arguments)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("error", "(xsss^as)",
		timestamp,
		server,
		domain,
		reason,
		arguments);
}

void maki_dbus_emit_invite (const gchar* server, const gchar* nick, const gchar* channel, const gchar* who)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("invite", "(xssss)",
		timestamp,
		server,
		nick,
		channel,
		who);
}

void maki_dbus_emit_join (const gchar* server, const gchar* nick, const gchar* channel)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("join", "(xsss)",
		timestamp,
		server,
		nick,
		channel);
}

void maki_dbus_emit_kick (const gchar* server, const gchar* nick, const gchar* channel, const gchar* who, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("kick", "(xsssss)",
		timestamp,
		server,
		nick,
		channel,
		who,
		message);
}

void maki_dbus_emit_list (const gchar* server, const gchar* channel, gint64 users, const gchar* topic)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("list", "(xssxs)",
		timestamp,
		server,
		channel,
		users,
		topic);
}

void maki_dbus_emit_message (const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("message", "(xssss)",
		timestamp,
		server,
		nick,
		target,
		message);
}

void maki_dbus_emit_mode (const gchar* server, const gchar* nick, const gchar* target, const gchar* mode, const gchar* parameter)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("mode", "(xsssss)",
		timestamp,
		server,
		nick,
		target,
		mode,
		parameter);
}

void maki_dbus_emit_motd (const gchar* server, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("motd", "(xss)",
		timestamp,
		server,
		message);
}

void maki_dbus_emit_names (const gchar* server, const gchar* channel, gchar** nicks, gchar** prefixes)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("names", "(xss^as^as)",
		timestamp,
		server,
		channel,
		nicks,
		prefixes);
}

void maki_dbus_emit_nick (const gchar* server, const gchar* nick, const gchar* new_nick)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("nick", "(xsss)",
		timestamp,
		server,
		nick,
		new_nick);
}

void maki_dbus_emit_no_such (const gchar* server, const gchar* target, const gchar* type)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("no_such", "(xsss)",
		timestamp,
		server,
		target,
		type);
}

void maki_dbus_emit_notice (const gchar* server, const gchar* nick, const gchar* target, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("notice", "(xssss)",
		timestamp,
		server,
		nick,
		target,
		message);
}

void maki_dbus_emit_oper (const gchar* server)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("oper", "(xs)",
		timestamp,
		server);
}

void maki_dbus_emit_part (const gchar* server, const gchar* nick, const gchar* channel, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("part", "(xssss)",
		timestamp,
		server,
		nick,
		channel,
		message);
}

void maki_dbus_emit_quit (const gchar* server, const gchar* nick, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("quit", "(xsss)",
		timestamp,
		server,
		nick,
		message);
}

void maki_dbus_emit_shutdown (void)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("shutdown", "(x)",
		timestamp);
}

void maki_dbus_emit_topic (const gchar* server, const gchar* nick, const gchar* channel, const gchar* topic)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("topic", "(xssss)",
		timestamp,
		server,
		nick,
		channel,
		topic);
}

void maki_dbus_emit_user_away (const gchar* server, const gchar* from, gboolean away)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("user_away", "(xssb)",
		timestamp,
		server,
		from,
		away);
}

void maki_dbus_emit_whois (const gchar* server, const gchar* nick, const gchar* message)
{
	GTimeVal timeval;
	gint64 timestamp;

	g_get_current_time(&timeval);
	timestamp = timeval.tv_sec;

	maki_dbus_emit_helper("whois", "(xsss)",
		timestamp,
		server,
		nick,
		message);
}

gboolean maki_dbus_action (const gchar* server, const gchar* channel, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar* tmp;

		tmp = g_strdup(message);
		g_strdelimit(tmp, "\r\n", ' ');

		maki_server_send_printf(serv, "PRIVMSG %s :\001ACTION %s\001", channel, tmp);

		maki_server_log(serv, channel, "%s %s", maki_user_nick(maki_server_user(serv)), tmp);

		maki_dbus_emit_action(server, maki_user_from(maki_server_user(serv)), channel, tmp);

		g_free(tmp);
	}

	return TRUE;
}

gboolean maki_dbus_away (const gchar* server, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_out_away(serv, message);
		maki_user_set_away_message(maki_server_user(serv), message);
	}

	return TRUE;
}

gboolean maki_dbus_back (const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send(serv, "AWAY");
	}

	return TRUE;
}

gboolean maki_dbus_channel_nicks (const gchar* server, const gchar* channel, gchar*** nicks, gchar*** prefixes, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*nicks = NULL;
	*prefixes = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			gchar** nick;
			gchar** prefix;
			const gchar* prefix_prefixes;
			gchar prefix_str[2];
			gsize length;
			GHashTableIter iter;
			gpointer key, value;

			nick = *nicks = g_new(gchar*, maki_channel_users_count(chan) + 1);
			prefix = *prefixes = g_new(gchar*, maki_channel_users_count(chan) + 1);
			maki_channel_users_iter(chan, &iter);

			prefix_prefixes = maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES);
			length = strlen(prefix_prefixes);
			prefix_str[1] = '\0';

			while (g_hash_table_iter_next(&iter, &key, &value))
			{
				guint pos;
				makiChannelUser* cuser = value;

				*nick = g_strdup(maki_user_nick(maki_channel_user_user(cuser)));
				nick++;

				prefix_str[0] = '\0';

				for (pos = 0; pos < length; pos++)
				{
					if (maki_channel_user_prefix_get(cuser, pos))
					{
						prefix_str[0] = prefix_prefixes[pos];
						break;
					}
				}

				*prefix = g_strdup(prefix_str);
				prefix++;
			}

			*nick = NULL;
			*prefix = NULL;
		}
	}

	maki_ensure_string_array(nicks);
	maki_ensure_string_array(prefixes);

	return TRUE;
}

gboolean maki_dbus_channel_topic (const gchar* server, const gchar* channel, gchar** topic, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*topic = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			*topic = g_strdup(maki_channel_topic(chan));
		}
	}

	maki_ensure_string(topic);

	return TRUE;
}

gboolean maki_dbus_channels (const gchar* server, gchar*** channels, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*channels = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** channel;
		GHashTableIter iter;
		gpointer key, value;

		channel = *channels = g_new(gchar*, maki_server_channels_count(serv) + 1);
		maki_server_channels_iter(serv, &iter);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			const gchar* chan_name = key;
			makiChannel* chan = value;

			if (maki_channel_joined(chan))
			{
				*channel = g_strdup(chan_name);
				++channel;
			}
		}

		*channel = NULL;
	}

	maki_ensure_string_array(channels);

	return TRUE;
}

gboolean maki_dbus_config_get (const gchar* group, const gchar* key, gchar** value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	*value = maki_instance_config_get_string(inst, group, key);

	maki_ensure_string(value);

	return TRUE;
}

gboolean maki_dbus_config_set (const gchar* group, const gchar* key, const gchar* value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_config_set_string(inst, group, key, value);

	return TRUE;
}

gboolean maki_dbus_connect (const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		/* Disconnect, because strange things happen if we call maki_server_connect() while still connected. */
		maki_server_disconnect(serv, "");
		maki_server_connect(serv);
	}
	else
	{
		if ((serv = maki_server_new(server)) != NULL)
		{
			maki_instance_add_server(inst, maki_server_name(serv), serv);

			if (!maki_server_autoconnect(serv))
			{
				maki_server_connect(serv);
			}
		}
	}

	return TRUE;
}

gboolean maki_dbus_ctcp (const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "PRIVMSG %s :\001%s\001", target, message);

		maki_dbus_emit_ctcp(server, maki_user_from(maki_server_user(serv)), target, message);
		maki_server_log(serv, target, "=%s= %s", maki_user_nick(maki_server_user(serv)), message);
	}

	return TRUE;
}

gboolean maki_dbus_dcc_send (const gchar* server, const gchar* target, const gchar* path, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiDCCSend* dcc;
		makiUser* user;

		user = maki_server_add_user(serv, target);

		if ((dcc = maki_dcc_send_new_out(serv, user, path)) != NULL)
		{
			maki_instance_add_dcc_send(inst, dcc);
		}

		maki_server_remove_user(serv, maki_user_nick(user));
	}

	return TRUE;
}

gboolean maki_dbus_dcc_sends (GArray** ids, gchar*** servers, gchar*** froms, gchar*** filenames, GArray** sizes, GArray** progresses, GArray** speeds, GArray** statuses, GError** error)
{
	guint len;
	makiInstance* inst = maki_instance_get_default();

	len = maki_instance_dcc_sends_count(inst);

	*ids = g_array_sized_new(FALSE, FALSE, sizeof(guint64), len);
	*servers = g_new(gchar*, len + 1);
	*froms = g_new(gchar*, len + 1);
	*filenames = g_new(gchar*, len + 1);
	*sizes = g_array_sized_new(FALSE, FALSE, sizeof(guint64), len);
	*progresses = g_array_sized_new(FALSE, FALSE, sizeof(guint64), len);
	*speeds = g_array_sized_new(FALSE, FALSE, sizeof(guint64), len);
	*statuses = g_array_sized_new(FALSE, FALSE, sizeof(guint64), len);

	(*servers)[len] = NULL;
	(*froms)[len] = NULL;
	(*filenames)[len] = NULL;

	maki_instance_dcc_sends_xxx(inst, ids, servers, froms, filenames, sizes, progresses, speeds, statuses);

	return TRUE;
}

gboolean maki_dbus_dcc_send_accept (guint64 id, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_accept_dcc_send(inst, id);

	return TRUE;
}

gboolean maki_dbus_dcc_send_get (guint64 id, const gchar* key, gchar** value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	*value = maki_instance_dcc_send_get(inst, id, key);

	maki_ensure_string(value);

	return TRUE;
}

gboolean maki_dbus_dcc_send_remove (guint64 id, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_remove_dcc_send(inst, id);

	return TRUE;
}

gboolean maki_dbus_dcc_send_resume (guint64 id, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_resume_dcc_send(inst, id);

	return TRUE;
}

gboolean maki_dbus_dcc_send_set (guint64 id, const gchar* key, const gchar* value, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	maki_instance_dcc_send_set(inst, id, key, value);

	return TRUE;
}

gboolean maki_dbus_ignore (const gchar* server, const gchar* pattern, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** ignores;

		ignores = maki_server_config_get_string_list(serv, "server", "ignores");

		if (ignores != NULL)
		{
			guint length;

			length = g_strv_length(ignores);
			ignores = g_renew(gchar*, ignores, length + 2);
			ignores[length] = g_strdup(pattern);
			ignores[length + 1] = NULL;
		}
		else
		{
			ignores = g_new(gchar*, 2);
			ignores[0] = g_strdup(pattern);
			ignores[1] = NULL;
		}

		maki_server_config_set_string_list(serv, "server", "ignores", ignores);

		g_strfreev(ignores);
	}

	return TRUE;
}

gboolean maki_dbus_ignores (const gchar* server, gchar*** ignores, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*ignores = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*ignores = maki_server_config_get_string_list(serv, "server", "ignores");
	}

	maki_ensure_string_array(ignores);

	return TRUE;
}

gboolean maki_dbus_invite (const gchar* server, const gchar* channel, const gchar* who, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "INVITE %s %s", who, channel);
	}

	return TRUE;
}

gboolean maki_dbus_join (const gchar* server, const gchar* channel, const gchar* key, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			gchar* channel_key;

			channel_key = maki_channel_key(chan);

			if (channel_key != NULL
			    && key[0] == '\0')
			{
				/* The channel has a key set and none was supplied. */
				maki_out_join(serv, channel, channel_key);
			}
			else
			{
				maki_out_join(serv, channel, key);
			}

			g_free(channel_key);
		}
		else
		{
			maki_out_join(serv, channel, key);
		}
	}

	return TRUE;
}

gboolean maki_dbus_kick (const gchar* server, const gchar* channel, const gchar* who, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (message[0])
		{
			maki_server_send_printf(serv, "KICK %s %s :%s", channel, who, message);
		}
		else
		{
			maki_server_send_printf(serv, "KICK %s %s", channel, who);
		}
	}

	return TRUE;
}

gboolean maki_dbus_list (const gchar* server, const gchar* channel, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (channel[0])
		{
			maki_server_send_printf(serv, "LIST %s", channel);
		}
		else
		{
			maki_server_send_printf(serv, "LIST");
		}
	}

	return TRUE;
}

gboolean maki_dbus_log (const gchar* server, const gchar* target, guint64 lines, gchar*** log, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*log = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar* file;
		gchar* file_format;
		gchar* file_tmp;
		gchar* filename;
		gchar* path;
		gchar* logs_dir;
		GFile* log_file;
		GFileInputStream* file_input;

		file_format = maki_instance_config_get_string(inst, "logging", "format");
		file_tmp = i_strreplace(file_format, "$n", target, 0);
		file = i_get_current_time_string(file_tmp);

		filename = g_strconcat(file, ".txt", NULL);
		logs_dir = maki_instance_config_get_string(inst, "directories", "logs");
		path = g_build_filename(logs_dir, server, filename, NULL);

		g_free(file_format);
		g_free(file_tmp);
		g_free(file);

		g_free(filename);
		g_free(logs_dir);

		log_file = g_file_new_for_path(path);

		if ((file_input = g_file_read(log_file, NULL, NULL)) != NULL)
		{
			GDataInputStream* data_input;
			guint length = 0;
			gchar* line;
			gchar** tmp;

			g_seekable_seek(G_SEEKABLE(file_input), -10 * 1024, G_SEEK_END, NULL, NULL);

			data_input = g_data_input_stream_new(G_INPUT_STREAM(file_input));
			g_object_unref(file_input);

			/* Discard the first line. */
			line = g_data_input_stream_read_line(data_input, NULL, NULL, NULL);
			g_free(line);

			tmp = g_new(gchar*, length + 1);
			tmp[length] = NULL;

			while ((line = g_data_input_stream_read_line(data_input, NULL, NULL, NULL)) != NULL)
			{
				g_strchomp(line);

				tmp = g_renew(gchar*, tmp, length + 2);
				/* The DBus specification says that strings may contain only one \0 character.
				 * Since line contains multiple \0 characters, provide a cleaned-up copy here. */
				tmp[length] = g_strdup(line);
				tmp[length + 1] = NULL;

				g_free(line);

				length++;
			}

			if (length > lines)
			{
				guint i;
				guint j;
				gchar** trunc;

				trunc = g_new(gchar*, lines + 1);

				for (i = 0; i < length - lines; i++)
				{
					g_free(tmp[i]);
				}

				for (i = length - lines, j = 0; i < length; i++, j++)
				{
					trunc[j] = tmp[i];
				}

				g_free(tmp[length]);
				g_free(tmp);

				trunc[lines] = NULL;

				tmp = trunc;
			}

			*log = tmp;

			g_object_unref(data_input);
		}

		g_object_unref(log_file);

		g_free(path);
	}

	maki_ensure_string_array(log);

	return TRUE;
}

gboolean maki_dbus_message (const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		const gchar* buffer;
		gchar** messages = NULL;

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
			maki_out_privmsg(serv, target, message, FALSE);
		}
		else
		{
			gchar** tmp;

			for (tmp = messages; *tmp != NULL; ++tmp)
			{
				g_strchomp(*tmp);

				if ((*tmp)[0])
				{
					maki_out_privmsg(serv, target, *tmp, TRUE);
				}
			}

			g_strfreev(messages);
		}
	}

	return TRUE;
}

gboolean maki_dbus_mode (const gchar* server, const gchar* target, const gchar* mode, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (mode[0])
		{
			maki_server_send_printf(serv, "MODE %s %s", target, mode);
		}
		else
		{
			maki_server_send_printf(serv, "MODE %s", target);
		}
	}

	return TRUE;
}

gboolean maki_dbus_names (const gchar* server, const gchar* channel, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (channel[0] != '\0')
		{
			maki_server_send_printf(serv, "NAMES %s", channel);
		}
		else
		{
			maki_server_send(serv, "NAMES");
		}
	}

	return TRUE;
}

gboolean maki_dbus_nick (const gchar* server, const gchar* nick, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (nick[0] != '\0')
		{
			maki_out_nick(serv, nick);
		}
		else
		{
			/* FIXME deprecated */
			maki_dbus_emit_nick(maki_server_name(serv), "", maki_user_nick(maki_server_user(serv)));
		}
	}

	return TRUE;
}


gboolean maki_dbus_nickserv (const gchar* server, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_out_nickserv(serv);
	}

	return TRUE;
}

gboolean maki_dbus_notice (const gchar* server, const gchar* target, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "NOTICE %s :%s", target, message);

		maki_dbus_emit_notice(maki_server_name(serv), maki_user_from(maki_server_user(serv)), target, message);
		maki_server_log(serv, target, "-%s- %s", maki_user_nick(maki_server_user(serv)), message);
	}

	return TRUE;
}

gboolean maki_dbus_oper (const gchar* server, const gchar* name, const gchar* password, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "OPER %s %s", name, password);
	}

	return TRUE;
}

gboolean maki_dbus_part (const gchar* server, const gchar* channel, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (message[0])
		{
			maki_server_send_printf(serv, "PART %s :%s", channel, message);
		}
		else
		{
			maki_server_send_printf(serv, "PART %s", channel);
		}
	}

	return TRUE;
}

gboolean maki_dbus_quit (const gchar* server, const gchar* message, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_disconnect(serv, message);
	}

	return TRUE;
}

gboolean maki_dbus_raw (const gchar* server, const gchar* command, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send(serv, command);
	}

	return TRUE;
}

gboolean maki_dbus_server_get (const gchar* server, const gchar* group, const gchar* key, gchar** value, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*value = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*value = maki_server_config_get_string(serv, group, key);
	}

	maki_ensure_string(value);

	return TRUE;
}

gboolean maki_dbus_server_get_list (const gchar* server, const gchar* group, const gchar* key, gchar*** list, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*list = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*list = maki_server_config_get_string_list(serv, group, key);
	}

	maki_ensure_string_array(list);

	return TRUE;
}

gboolean maki_dbus_server_list (const gchar* server, const gchar* group, gchar*** result, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*result = NULL;

	if (server[0])
	{
		if ((serv = maki_instance_get_server(inst, server)) != NULL)
		{
			if (group[0])
			{
				*result = maki_server_config_get_keys(serv, group);
			}
			else
			{
				*result = maki_server_config_get_groups(serv);

			}
		}
	}
	else
	{
		GHashTableIter iter;
		gpointer key, value;
		guint i = 0;
		gchar** tmp;

		tmp = g_new(gchar*, maki_instance_servers_count(inst) + 1);

		maki_instance_servers_iter(inst, &iter);

		while (g_hash_table_iter_next(&iter, &key, &value))
		{
			const gchar* name = key;

			tmp[i] = g_strdup(name);
			i++;
		}

		tmp[i] = NULL;
		*result = tmp;
	}

	maki_ensure_string_array(result);

	return TRUE;
}

gboolean maki_dbus_server_remove (const gchar* server, const gchar* group, const gchar* key, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (group[0])
		{
			if (key[0])
			{
				maki_server_config_remove_key(serv, group, key);
			}
			else
			{
				maki_server_config_remove_group(serv, group);
			}
		}
		else
		{
			gchar* path;

			maki_instance_remove_server(inst, server);

			path = g_build_filename(maki_instance_directory(inst, "servers"), server, NULL);
			g_unlink(path);
			g_free(path);
		}
	}

	return TRUE;
}

gboolean maki_dbus_server_rename (const gchar* old, const gchar* new, GError** error)
{
	makiInstance* inst = maki_instance_get_default();

	if (maki_instance_rename_server(inst, old, new))
	{
		gchar* old_path;
		gchar* new_path;

		old_path = g_build_filename(maki_instance_directory(inst, "servers"), old, NULL);
		new_path = g_build_filename(maki_instance_directory(inst, "servers"), new, NULL);

		g_rename(old_path, new_path);

		g_free(old_path);
		g_free(new_path);
	}

	return TRUE;
}

gboolean maki_dbus_server_set (const gchar* server, const gchar* group, const gchar* key, const gchar* value, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_config_set_string(serv, group, key, value);
	}
	else
	{
		if ((serv = maki_server_new(server)) != NULL)
		{
			maki_server_config_set_string(serv, group, key, value);
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}
	}

	return TRUE;
}

gboolean maki_dbus_server_set_list (const gchar* server, const gchar* group, const gchar* key, gchar** list, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_config_set_string_list(serv, group, key, list);
	}
	else
	{
		if ((serv = maki_server_new(server)) != NULL)
		{
			maki_server_config_set_string_list(serv, group, key, list);
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}
	}

	return TRUE;
}

gboolean maki_dbus_servers (gchar*** servers, GError** error)
{
	gchar** server;
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	/* FIXME wastes memory */
	server = *servers = g_new(gchar*, maki_instance_servers_count(inst) + 1);
	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		if (maki_server_connected(serv))
		{
			*server = g_strdup(maki_server_name(serv));
			server++;
		}
	}

	*server = NULL;

	return TRUE;
}

gboolean maki_dbus_shutdown (const gchar* message, GError** error)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, message);
	}

	maki_dbus_emit_shutdown();

	/* FIXME quits too soon */
	g_main_loop_quit(main_loop);

	return TRUE;
}

gboolean maki_dbus_support_chantypes (const gchar* server, gchar** chantypes, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*chantypes = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*chantypes = g_strdup(maki_server_support(serv, MAKI_SERVER_SUPPORT_CHANTYPES));
	}

	maki_ensure_string(chantypes);

	return TRUE;
}

gboolean maki_dbus_support_prefix (const gchar* server, gchar*** prefix, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*prefix = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		*prefix = g_new(gchar*, 3);
		(*prefix)[0] = g_strdup(maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_MODES));
		(*prefix)[1] = g_strdup(maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES));
		(*prefix)[2] = NULL;
	}

	maki_ensure_string_array(prefix);

	return TRUE;
}

gboolean maki_dbus_topic (const gchar* server, const gchar* channel, const gchar* topic, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (topic[0] != '\0')
		{
			maki_server_send_printf(serv, "TOPIC %s :%s", channel, topic);
		}
		else
		{
			/* FIXME deprecated */
			makiChannel* chan;

			if ((chan = maki_server_get_channel(serv, channel)) != NULL
			    && maki_channel_topic(chan) != NULL)
			{
				maki_dbus_emit_topic(maki_server_name(serv), "", channel, maki_channel_topic(chan));
			}
			else
			{
				maki_server_send_printf(serv, "TOPIC %s", channel);
			}
		}
	}

	return TRUE;
}

gboolean maki_dbus_unignore (const gchar* server, const gchar* pattern, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		gchar** ignores;

		ignores = maki_server_config_get_string_list(serv, "server", "ignores");

		if (ignores != NULL)
		{
			guint i;
			guint j;
			guint length;
			gchar** tmp;

			length = g_strv_length(ignores);
			j = 0;

			for (i = 0; i < length; ++i)
			{
				if (strcmp(ignores[i], pattern) == 0)
				{
					++j;
				}
			}

			if (length - j == 0)
			{
				maki_server_config_remove_key(serv, "server", "ignores");

				g_strfreev(ignores);

				return TRUE;
			}

			tmp = g_new(gchar*, length - j + 1);
			j = 0;

			for (i = 0; i < length; ++i)
			{
				if (strcmp(ignores[i], pattern) != 0)
				{
					tmp[j] = g_strdup(ignores[i]);
					++j;
				}
			}

			tmp[j] = NULL;

			maki_server_config_set_string_list(serv, "server", "ignores", tmp);

			g_strfreev(tmp);
		}

		g_strfreev(ignores);
	}

	return TRUE;
}

gboolean maki_dbus_user_away (const gchar* server, const gchar* nick, gboolean* away, GError** error)
{

	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*away = FALSE;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiUser* user;

		if ((user = maki_server_get_user(serv, nick)) != NULL)
		{
			*away = maki_user_away(user);
		}
	}

	return TRUE;
}

gboolean maki_dbus_user_channel_mode (const gchar* server, const gchar* channel, const gchar* nick, gchar** mode, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*mode = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			makiChannelUser* cuser;

			if ((cuser = maki_channel_get_user(chan, nick)) != NULL)
			{
				const gchar* prefix_modes;
				gint pos;
				gint length;
				gchar tmp = '\0';

				prefix_modes = maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_MODES);
				length = strlen(prefix_modes);

				for (pos = 0; pos < length; pos++)
				{
					if (maki_channel_user_prefix_get(cuser, pos))
					{
						tmp = prefix_modes[pos];
						break;
					}
				}

				if (tmp)
				{
					*mode = g_new(gchar, 2);
					(*mode)[0] = tmp;
					(*mode)[1] = '\0';
				}
				else
				{
					*mode = g_new(gchar, 1);
					(*mode)[0] = '\0';
				}
			}
		}
	}

	maki_ensure_string(mode);

	return TRUE;
}

gboolean maki_dbus_user_channel_prefix (const gchar* server, const gchar* channel, const gchar* nick, gchar** prefix, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*prefix = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		makiChannel* chan;

		if ((chan = maki_server_get_channel(serv, channel)) != NULL)
		{
			makiChannelUser* cuser;

			if ((cuser = maki_channel_get_user(chan, nick)) != NULL)
			{
				const gchar* prefix_prefixes;
				gint pos;
				gint length;
				gchar tmp = '\0';

				prefix_prefixes = maki_server_support(serv, MAKI_SERVER_SUPPORT_PREFIX_PREFIXES);
				length = strlen(prefix_prefixes);

				for (pos = 0; pos < length; pos++)
				{
					if (maki_channel_user_prefix_get(cuser, pos))
					{
						tmp = prefix_prefixes[pos];
						break;
					}
				}

				if (tmp)
				{
					*prefix = g_new(gchar, 2);
					(*prefix)[0] = tmp;
					(*prefix)[1] = '\0';
				}
				else
				{
					*prefix = g_new(gchar, 1);
					(*prefix)[0] = '\0';
				}
			}
		}
	}

	maki_ensure_string(prefix);

	return TRUE;
}

gboolean maki_dbus_user_from (const gchar* server, const gchar* nick, gchar** from, GError** error)
{

	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	*from = NULL;

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (nick[0] != '\0')
		{
			makiUser* user;

			if ((user = maki_server_get_user(serv, nick)) != NULL)
			{
				*from = g_strdup(maki_user_from(user));
			}
		}
		else
		{
			*from = g_strdup(maki_user_from(maki_server_user(serv)));
		}
	}

	maki_ensure_string(from);

	return TRUE;
}

gboolean maki_dbus_version (GArray** version, GError** error)
{
	gchar** p;
	guint p_len;
	guint i;

	*version = g_array_new(FALSE, FALSE, sizeof(guint64));

	p = g_strsplit(SUSHI_VERSION, ".", 0);
	p_len = g_strv_length(p);

	for (i = 0; i < p_len; i++)
	{
		guint64 v;

		v = g_ascii_strtoull(p[i], NULL, 10);
		*version = g_array_append_val(*version, v);
	}

	g_strfreev(p);

	return TRUE;
}

gboolean maki_dbus_who (const gchar* server, const gchar* mask, gboolean operators_only, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		if (operators_only)
		{
			maki_server_send_printf(serv, "WHO %s o", mask);
		}
		else
		{
			maki_server_send_printf(serv, "WHO %s", mask);
		}
	}

	return TRUE;
}

gboolean maki_dbus_whois (const gchar* server, const gchar* mask, GError** error)
{
	makiServer* serv;
	makiInstance* inst = maki_instance_get_default();

	if ((serv = maki_instance_get_server(inst, server)) != NULL)
	{
		maki_server_send_printf(serv, "WHOIS %s", mask);
	}

	return TRUE;
}
