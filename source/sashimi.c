/*
 * Copyright (c) 2008-2009 Michael Kuhn
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

#include <string.h>

#include <ilib.h>

#include "sashimi.h"

enum
{
	c_connect,
	c_read,
	c_last
};

enum
{
	s_ping,
	s_queue,
	s_last
};

struct sashimi_connection
{
	GSocketConnection* connection;

	struct
	{
		GDataInputStream* input;
		GDataOutputStream* output;
	}
	stream;

	glong last_activity;
	guint timeout;

	GMainContext* main_context;

	GMutex* queue_mutex;
	GQueue* queue;

	GCancellable* cancellables[c_last];
	guint sources[s_last];

	struct
	{
		void (*callback) (gpointer);
		gpointer data;
	}
	connect;

	struct
	{
		void (*callback) (const gchar*, gpointer);
		gpointer data;
	}
	read;

	struct
	{
		void (*callback) (gpointer);
		gpointer data;
	}
	reconnect;
};

static void sashimi_cancel (sashimiConnection* conn)
{
	gint i;

	for (i = 0; i < c_last; i++)
	{
		g_cancellable_cancel(conn->cancellables[i]);
		g_cancellable_reset(conn->cancellables[i]);
	}

	for (i = 0; i < s_last; i++)
	{
		if (conn->sources[i] != 0)
		{
			i_source_remove(conn->sources[i], conn->main_context);
			conn->sources[i] = 0;
		}
	}
}

static guint sashimi_timeout_add_seconds (sashimiConnection* conn, guint32 interval, GSourceFunc func)
{
	g_return_val_if_fail(conn != NULL, 0);

	return i_timeout_add_seconds(interval, func, conn, conn->main_context);
}

static void sashimi_read_cb (GObject* object, GAsyncResult* result, gpointer data)
{
	sashimiConnection* conn = data;
	GTimeVal timeval;
	GError* error = NULL;
	gchar* buffer;

	g_get_current_time(&timeval);

	if ((buffer = g_data_input_stream_read_line_finish(conn->stream.input, result, NULL, &error)) != NULL)
	{
		conn->last_activity = timeval.tv_sec;

		/* Remove whitespace at the end of the string. */
		g_strchomp(buffer);

		/* Handle PING internally. */
		if (strncmp(buffer, "PING ", 5) == 0)
		{
			gchar* tmp;

			tmp = g_strconcat("PONG ", buffer + 5, NULL);
			sashimi_send(conn, tmp);
			g_free(tmp);
		}
		else if (conn->read.callback != NULL)
		{
			conn->read.callback(buffer, conn->read.data);
		}

		g_free(buffer);
	}

	if (error != NULL)
	{
		g_error_free(error);
		goto reconnect;
	}

	g_data_input_stream_read_line_async(conn->stream.input, G_PRIORITY_DEFAULT, conn->cancellables[c_read], sashimi_read_cb, conn);

	return;

reconnect:
	sashimi_cancel(conn);

	if (conn->reconnect.callback)
	{
		conn->reconnect.callback(conn->reconnect.data);
	}
}

static gboolean sashimi_ping (gpointer data)
{
	GTimeVal timeval;
	sashimiConnection* conn = data;

	g_get_current_time(&timeval);

	/* If we did not hear anything from the server, send a PING. */
	if (conn->timeout > 0 && (gulong)(timeval.tv_sec - conn->last_activity) > conn->timeout)
	{
		gchar* ping;

		ping = g_strdup_printf("PING :%ld", timeval.tv_sec);
		sashimi_send(conn, ping);
		g_free(ping);

		conn->last_activity = timeval.tv_sec;
	}

	return TRUE;
}

static gboolean sashimi_queue_runner (gpointer data)
{
	sashimiConnection* conn = data;

	g_mutex_lock(conn->queue_mutex);

	if (!g_queue_is_empty(conn->queue))
	{
		gchar* message;

		message = g_queue_peek_head(conn->queue);

		if (sashimi_send(conn, message))
		{
			g_queue_pop_head(conn->queue);
			g_free(message);
		}
	}

	g_mutex_unlock(conn->queue_mutex);

	return TRUE;
}

static void sashimi_connect_cb (GObject* object, GAsyncResult* result, gpointer data)
{
	GSocketClient* client = G_SOCKET_CLIENT(object);
	sashimiConnection* conn = data;
	GTimeVal timeval;

	if ((conn->connection = g_socket_client_connect_to_host_finish(client, result, NULL)) == NULL)
	{
		goto reconnect;
	}

	conn->stream.input = g_data_input_stream_new(g_io_stream_get_input_stream(G_IO_STREAM(conn->connection)));
	conn->stream.output = g_data_output_stream_new(g_io_stream_get_output_stream(G_IO_STREAM(conn->connection)));

	g_get_current_time(&timeval);
	conn->last_activity = timeval.tv_sec;

	g_data_input_stream_read_line_async(conn->stream.input, G_PRIORITY_DEFAULT, conn->cancellables[c_read], sashimi_read_cb, conn);

	conn->sources[s_ping] = sashimi_timeout_add_seconds(conn, 1, sashimi_ping);
	conn->sources[s_queue] = sashimi_timeout_add_seconds(conn, 1, sashimi_queue_runner);

	if (conn->connect.callback)
	{
		conn->connect.callback(conn->connect.data);
	}

	return;

reconnect:
	sashimi_cancel(conn);

	if (conn->reconnect.callback)
	{
		conn->reconnect.callback(conn->reconnect.data);
	}
}

sashimiConnection* sashimi_new (GMainContext* main_context)
{
	gint i;
	sashimiConnection* conn;

	conn = g_new(sashimiConnection, 1);

	if (main_context != NULL)
	{
		g_main_context_ref(main_context);
	}

	conn->main_context = main_context;

	conn->queue_mutex = g_mutex_new();
	conn->queue = g_queue_new();

	conn->connection = NULL;
	conn->stream.input = NULL;
	conn->stream.output = NULL;

	conn->last_activity = 0;
	conn->timeout = 0;

	for (i = 0; i < c_last; i++)
	{
		conn->cancellables[i] = g_cancellable_new();
	}

	for (i = 0; i < s_last; ++i)
	{
		conn->sources[i] = 0;
	}

	conn->connect.callback = NULL;
	conn->connect.data = NULL;

	conn->read.callback = NULL;
	conn->read.data = NULL;

	conn->reconnect.callback = NULL;
	conn->reconnect.data = NULL;

	return conn;
}

void sashimi_connect_callback (sashimiConnection* conn, void (*callback) (gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	conn->connect.callback = callback;
	conn->connect.data = data;
}

void sashimi_read_callback (sashimiConnection* conn, void (*callback) (const gchar*, gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	conn->read.callback = callback;
	conn->read.data = data;
}

void sashimi_reconnect_callback (sashimiConnection* conn, void (*callback) (gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	conn->reconnect.callback = callback;
	conn->reconnect.data = data;
}

void sashimi_timeout (sashimiConnection* conn, guint timeout)
{
	g_return_if_fail(conn != NULL);

	conn->timeout = timeout;
}

gboolean sashimi_connect (sashimiConnection* conn, const gchar* address, guint port, gboolean ssl)
{
	GSocketClient* client;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(address != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);

	sashimi_disconnect(conn);

	client = g_socket_client_new();

	if (ssl)
	{
#if GLIB_CHECK_VERSION(2,28,0)
		g_socket_client_set_tls(client, TRUE);
#else
		g_object_unref(client);

		return FALSE;
#endif
	}

	g_socket_client_connect_to_host_async(client, address, port, conn->cancellables[c_connect], sashimi_connect_cb, conn);
	g_object_unref(client);

	return TRUE;
}

gboolean sashimi_disconnect (sashimiConnection* conn)
{
	g_return_val_if_fail(conn != NULL, FALSE);

	g_mutex_lock(conn->queue_mutex);

	/* Try to flush queue. */
	while (!g_queue_is_empty(conn->queue))
	{
		gchar* message;

		message = g_queue_pop_head(conn->queue);
		sashimi_send(conn, message);
		g_free(message);
	}

	g_mutex_unlock(conn->queue_mutex);

	sashimi_cancel(conn);

	if (conn->connection != NULL)
	{
		g_object_unref(conn->connection);
		conn->connection = NULL;
	}

	if (conn->stream.input != NULL)
	{
		g_object_unref(conn->stream.input);
		conn->stream.input = NULL;
	}

	if (conn->stream.output != NULL)
	{
		g_object_unref(conn->stream.output);
		conn->stream.output = NULL;
	}

	return TRUE;
}

void sashimi_free (sashimiConnection* conn)
{
	guint i;

	g_return_if_fail(conn != NULL);

	sashimi_disconnect(conn);

	/* Clean up the queue. */
	while (!g_queue_is_empty(conn->queue))
	{
		g_free(g_queue_pop_head(conn->queue));
	}

	g_queue_free(conn->queue);
	g_mutex_free(conn->queue_mutex);

	if (conn->main_context != NULL)
	{
		g_main_context_unref(conn->main_context);
	}

	for (i = 0; i < c_last; i++)
	{
		g_object_unref(conn->cancellables[i]);
	}

	g_free(conn);
}

gboolean sashimi_send (sashimiConnection* conn, const gchar* message)
{
	GError* error = NULL;
	gchar* tmp;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	if (conn->connection == NULL)
	{
		return FALSE;
	}

	tmp = g_strconcat(message, "\r\n", NULL);

	if (g_data_output_stream_put_string(conn->stream.output, tmp, NULL, &error))
	{
		g_output_stream_flush(G_OUTPUT_STREAM(conn->stream.output), NULL, NULL);
		g_printerr("OUT: %s", tmp);
	}

	g_free(tmp);

	if (error != NULL)
	{
		g_printerr("WRITE_ERROR %s\n", error->message);
		g_error_free(error);

		return FALSE;
	}

	return TRUE;
}

gboolean sashimi_queue (sashimiConnection* conn, const gchar* message)
{
	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(conn->queue_mutex);
	g_queue_push_tail(conn->queue, g_strdup(message));
	g_mutex_unlock(conn->queue_mutex);

	return TRUE;
}

gboolean sashimi_send_or_queue (sashimiConnection* conn, const gchar* message)
{
	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(conn->queue_mutex);

	if (g_queue_is_empty(conn->queue))
	{
		g_mutex_unlock(conn->queue_mutex);

		return sashimi_send(conn, message);
	}
	else
	{
		g_queue_push_tail(conn->queue, g_strdup(message));
		g_mutex_unlock(conn->queue_mutex);

		return TRUE;
	}
}
