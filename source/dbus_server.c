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

static DBusHandlerResult
maki_dbus_server_message_handler (DBusConnection* connection, DBusMessage* message, void* data)
{
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	makiDBusServer* dserv = data;

	g_print("METHOD %s: %s %s\n", dbus_message_get_path(message), dbus_message_get_interface(message), dbus_message_get_member(message));

	if (strcmp(dbus_message_get_path(message), DBUS_PATH_LOCAL) == 0)
	{
		if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected"))
		{
			dserv->connections = g_slist_remove(dserv->connections, connection);
		}

		return DBUS_HANDLER_RESULT_HANDLED;
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

	for (list = dserv->connections; list != NULL; list = list->next)
	{
		DBusConnection* connection = list->data;

		dbus_connection_close(connection);
		dbus_connection_unref(connection);
	}

	g_slist_free(dserv->connections);

	g_main_loop_quit(dserv->main_loop);
	g_thread_join(dserv->thread);

	g_main_loop_unref(dserv->main_loop);
	g_main_context_unref(dserv->main_context);

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

