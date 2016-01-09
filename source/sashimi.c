/*
 * Copyright (c) 2008-2012 Michael Kuhn
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

	gboolean connected;
	glong last_activity;
	guint timeout;

	GMainContext* main_context;

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
		void (*callback) (gpointer);
		gpointer data;
	}
	disconnect;

	struct
	{
		void (*callback) (gchar const*, gpointer);
		gpointer data;
	}
	read;

	GMutex mutex[1];
};

static
void
sashimi_close (sashimiConnection* conn)
{
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
}

static
void
sashimi_cancel (sashimiConnection* conn, gboolean destroy)
{
	guint i;

	for (i = 0; i < c_last; i++)
	{
		g_cancellable_cancel(conn->cancellables[i]);
		g_object_unref(conn->cancellables[i]);
		conn->cancellables[i] = NULL;

		if (!destroy)
		{
			conn->cancellables[i] = g_cancellable_new();
		}
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

static
gboolean
sashimi_real_send (sashimiConnection* conn, gchar const* message)
{
	GError* error = NULL;
	gchar* tmp;

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

static
guint
sashimi_timeout_add_seconds (sashimiConnection* conn, guint32 interval, GSourceFunc func)
{
	return i_timeout_add_seconds(interval, func, conn, conn->main_context);
}

static
void
sashimi_on_read (GObject* object, GAsyncResult* result, gpointer data)
{
	GDataInputStream* stream = G_DATA_INPUT_STREAM(object);
	sashimiConnection* conn = data;
	GError* error = NULL;
	gchar* buffer;

	g_mutex_lock(conn->mutex);

	if ((buffer = g_data_input_stream_read_line_finish(stream, result, NULL, &error)) != NULL)
	{
		GTimeVal timeval;

		g_get_current_time(&timeval);
		conn->last_activity = timeval.tv_sec;

		/* Remove whitespace at the end of the string. */
		g_strchomp(buffer);

		/* Handle PING internally. */
		if (strncmp(buffer, "PING ", 5) == 0)
		{
			gchar* tmp;

			tmp = g_strconcat("PONG ", buffer + 5, NULL);
			sashimi_real_send(conn, tmp);
			g_free(tmp);
		}
		else if (conn->read.callback != NULL)
		{
			g_mutex_unlock(conn->mutex);
			/* FIXME */
			conn->read.callback(buffer, conn->read.data);
			g_mutex_lock(conn->mutex);
		}

		g_free(buffer);
	}

	if (error != NULL)
	{
		gboolean canceled;

		canceled = (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED);

		g_printerr("READ_ERROR: %s\n", error->message);
		g_error_free(error);

		if (canceled)
		{
			g_mutex_unlock(conn->mutex);
			return;
		}

		goto disconnect;
	}

	g_data_input_stream_read_line_async(stream, G_PRIORITY_DEFAULT, conn->cancellables[c_read], sashimi_on_read, conn);

	g_mutex_unlock(conn->mutex);

	return;

disconnect:
	g_mutex_unlock(conn->mutex);

	sashimi_disconnect(conn);

	/* FIXME */
	if (conn->disconnect.callback)
	{
		conn->disconnect.callback(conn->disconnect.data);
	}
}

static
gboolean
sashimi_ping (gpointer data)
{
	GTimeVal timeval;
	sashimiConnection* conn = data;

	g_mutex_lock(conn->mutex);

	g_get_current_time(&timeval);

	/* If we did not hear anything from the server, send a PING. */
	if (conn->timeout > 0 && (gulong)(timeval.tv_sec - conn->last_activity) > conn->timeout)
	{
		gchar* ping;

		ping = g_strdup_printf("PING :%ld", timeval.tv_sec);
		sashimi_real_send(conn, ping);
		g_free(ping);

		conn->last_activity = timeval.tv_sec;
	}

	g_mutex_unlock(conn->mutex);

	return TRUE;
}

static
gboolean
sashimi_queue_runner (gpointer data)
{
	sashimiConnection* conn = data;

	g_mutex_lock(conn->mutex);

	if (!g_queue_is_empty(conn->queue))
	{
		gchar* message;

		message = g_queue_peek_head(conn->queue);

		if (sashimi_real_send(conn, message))
		{
			g_queue_pop_head(conn->queue);
			g_free(message);
		}
	}

	g_mutex_unlock(conn->mutex);

	return TRUE;
}

static
void
sashimi_on_connect (GObject* object, GAsyncResult* result, gpointer data)
{
	GSocketClient* client = G_SOCKET_CLIENT(object);
	sashimiConnection* conn = data;
	GTimeVal timeval;
	GError* error = NULL;

	g_mutex_lock(conn->mutex);

	if ((conn->connection = g_socket_client_connect_to_host_finish(client, result, &error)) == NULL)
	{
		if (error != NULL)
		{
			gboolean canceled;

			canceled = (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED);

			g_printerr("CONNECT_ERROR: %s\n", error->message);
			g_error_free(error);

			if (canceled)
			{
				g_mutex_unlock(conn->mutex);
				return;
			}
		}

		goto disconnect;
	}

	conn->stream.input = g_data_input_stream_new(g_io_stream_get_input_stream(G_IO_STREAM(conn->connection)));
	conn->stream.output = g_data_output_stream_new(g_io_stream_get_output_stream(G_IO_STREAM(conn->connection)));

	g_get_current_time(&timeval);
	conn->last_activity = timeval.tv_sec;

	g_data_input_stream_read_line_async(conn->stream.input, G_PRIORITY_DEFAULT, conn->cancellables[c_read], sashimi_on_read, conn);

	conn->sources[s_ping] = sashimi_timeout_add_seconds(conn, 1, sashimi_ping);
	conn->sources[s_queue] = sashimi_timeout_add_seconds(conn, 1, sashimi_queue_runner);

	g_mutex_unlock(conn->mutex);

	/* FIXME */
	if (conn->connect.callback)
	{
		conn->connect.callback(conn->connect.data);
	}

	return;

disconnect:
	g_mutex_unlock(conn->mutex);

	sashimi_disconnect(conn);

	/* FIXME */
	if (conn->disconnect.callback)
	{
		conn->disconnect.callback(conn->disconnect.data);
	}
}

static
void
sashimi_ssl_db_handler (GSocketClient* client, GSocketClientEvent event, GSocketConnectable* connectable, GIOStream* connection, gpointer user_data)
{
	GTlsClientConnection *c;
	GTlsDatabase *db;
	gchar const* anchors;

	if (event != G_SOCKET_CLIENT_TLS_HANDSHAKING)
	{
		return;
	}

	c = (GTlsClientConnection*) connection;
	anchors = (gchar const*) user_data;
	db = g_tls_file_database_new(anchors, NULL);

	g_tls_connection_set_database(G_TLS_CONNECTION(c), db);
}

sashimiConnection*
sashimi_new (GMainContext* main_context)
{
	gint i;
	sashimiConnection* conn;

	conn = g_new(sashimiConnection, 1);

	if (main_context != NULL)
	{
		g_main_context_ref(main_context);
	}

	conn->main_context = main_context;

	conn->queue = g_queue_new();

	conn->connection = NULL;
	conn->stream.input = NULL;
	conn->stream.output = NULL;

	conn->connected = FALSE;
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

	conn->disconnect.callback = NULL;
	conn->disconnect.data = NULL;

	g_mutex_init(conn->mutex);

	return conn;
}

void
sashimi_free (sashimiConnection* conn)
{
	g_return_if_fail(conn != NULL);

	sashimi_cancel(conn, TRUE);
	sashimi_close(conn);

	g_mutex_clear(conn->mutex);

	/* Clean up the queue. */
	while (!g_queue_is_empty(conn->queue))
	{
		g_free(g_queue_pop_head(conn->queue));
	}

	g_queue_free(conn->queue);

	if (conn->main_context != NULL)
	{
		g_main_context_unref(conn->main_context);
	}

	g_free(conn);
}

void
sashimi_timeout (sashimiConnection* conn, guint timeout)
{
	g_return_if_fail(conn != NULL);

	g_mutex_lock(conn->mutex);
	conn->timeout = timeout;
	g_mutex_unlock(conn->mutex);
}

void
sashimi_connect_callback (sashimiConnection* conn, void (*callback) (gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	g_mutex_lock(conn->mutex);
	conn->connect.callback = callback;
	conn->connect.data = data;
	g_mutex_unlock(conn->mutex);
}

void
sashimi_read_callback (sashimiConnection* conn, void (*callback) (gchar const*, gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	g_mutex_lock(conn->mutex);
	conn->read.callback = callback;
	conn->read.data = data;
	g_mutex_unlock(conn->mutex);
}

void
sashimi_disconnect_callback (sashimiConnection* conn, void (*callback) (gpointer), gpointer data)
{
	g_return_if_fail(conn != NULL);

	g_mutex_lock(conn->mutex);
	conn->disconnect.callback = callback;
	conn->disconnect.data = data;
	g_mutex_unlock(conn->mutex);
}

gboolean
sashimi_connect (sashimiConnection* conn, gchar const* address, guint port, gboolean ssl, gchar const* ssl_db)
{
	GSocketClient* client = NULL;
	gboolean ret = TRUE;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(address != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);

	g_mutex_lock(conn->mutex);

	if (conn->connected)
	{
		ret = FALSE;
		goto end;
	}

	client = g_socket_client_new();

	if (ssl)
	{
		if (ssl_db != NULL && strlen(ssl_db) > 0)
		{
			g_signal_connect(client, "event", G_CALLBACK(sashimi_ssl_db_handler), (gchar*)ssl_db);
		}

		g_socket_client_set_tls(client, TRUE);
	}

	g_socket_client_connect_to_host_async(client, address, port, conn->cancellables[c_connect], sashimi_on_connect, conn);

	conn->connected = TRUE;

end:
	if (client != NULL)
	{
		g_object_unref(client);
	}

	g_mutex_unlock(conn->mutex);

	return ret;
}

gboolean
sashimi_disconnect (sashimiConnection* conn)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(conn != NULL, FALSE);

	g_mutex_lock(conn->mutex);

	if (!conn->connected)
	{
		ret = FALSE;
		goto end;
	}

	/* Try to flush queue. */
	while (!g_queue_is_empty(conn->queue))
	{
		gchar* message;

		message = g_queue_pop_head(conn->queue);
		sashimi_real_send(conn, message);
		g_free(message);
	}

	sashimi_cancel(conn, FALSE);
	sashimi_close(conn);

	conn->connected = FALSE;

end:
	g_mutex_unlock(conn->mutex);

	return ret;
}

gboolean
sashimi_send (sashimiConnection* conn, gchar const* message)
{
	gboolean ret;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(conn->mutex);
	ret = sashimi_real_send(conn, message);
	g_mutex_unlock(conn->mutex);

	return ret;
}

gboolean
sashimi_queue (sashimiConnection* conn, gchar const* message)
{
	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(conn->mutex);
	g_queue_push_tail(conn->queue, g_strdup(message));
	g_mutex_unlock(conn->mutex);

	return TRUE;
}

gboolean
sashimi_send_or_queue (sashimiConnection* conn, gchar const* message)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(conn->mutex);

	if (g_queue_is_empty(conn->queue))
	{
		ret = sashimi_real_send(conn, message);
	}
	else
	{
		g_queue_push_tail(conn->queue, g_strdup(message));
	}

	g_mutex_unlock(conn->mutex);

	return ret;
}
