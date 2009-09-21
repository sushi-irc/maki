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

#define G_DISABLE_DEPRECATED
#define _XOPEN_SOURCE

#include "sashimi.h"

#include "ilib.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

enum
{
	s_write,
	s_read,
	s_ping,
	s_queue,
	s_last
};

struct sashimi_connection
{
	GIOChannel* channel;

	glong last_activity;
	guint timeout;

	GMainContext* main_context;

	GMutex* queue_mutex;
	GQueue* queue;

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

static void sashimi_remove_sources (sashimiConnection* conn, gint id)
{
	gint i;

	for (i = 0; i < s_last; i++)
	{
		if (i != id && conn->sources[i] != 0)
		{
			i_source_remove(conn->sources[i], conn->main_context);
			conn->sources[i] = 0;
		}
	}

	if (id >= 0)
	{
		conn->sources[id] = 0;
	}
}

static guint sashimi_io_add_watch (sashimiConnection* conn, GIOCondition condition, GIOFunc func)
{
	g_return_val_if_fail(conn != NULL, 0);

	return i_io_add_watch(conn->channel, condition, func, conn, conn->main_context);
}

static guint sashimi_timeout_add_seconds (sashimiConnection* conn, guint32 interval, GSourceFunc func)
{
	g_return_val_if_fail(conn != NULL, 0);

	return i_timeout_add_seconds(interval, func, conn, conn->main_context);
}

static gboolean sashimi_read (GIOChannel* source, GIOCondition condition, gpointer data)
{
	GIOStatus status;
	GTimeVal timeval;
	gchar* buffer;
	sashimiConnection* conn = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto reconnect;
	}

	g_get_current_time(&timeval);

	while ((status = g_io_channel_read_line(conn->channel, &buffer, NULL, NULL, NULL)) == G_IO_STATUS_NORMAL)
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
			g_free(buffer);

			continue;
		}

		if (conn->read.callback != NULL)
		{
			conn->read.callback(buffer, conn->read.data);
		}

		g_free(buffer);
	}

	if (status == G_IO_STATUS_EOF)
	{
		goto reconnect;
	}

	return TRUE;

reconnect:
	sashimi_remove_sources(conn, s_read);

	if (conn->reconnect.callback)
	{
		conn->reconnect.callback(conn->reconnect.data);
	}

	return FALSE;
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

	if (!g_queue_is_empty(conn->queue))
	{
		gchar* message;

		message = g_queue_peek_head(conn->queue);

		if (sashimi_send(conn, message))
		{
			g_mutex_lock(conn->queue_mutex);
			g_queue_pop_head(conn->queue);
			g_mutex_unlock(conn->queue_mutex);
			g_free(message);
		}
	}

	return TRUE;
}

static gboolean sashimi_write (GIOChannel* source, GIOCondition condition, gpointer data)
{
	GTimeVal timeval;
	int val;
	socklen_t len = sizeof(val);
	sashimiConnection* conn = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto reconnect;
	}

	if (getsockopt(g_io_channel_unix_get_fd(conn->channel), SOL_SOCKET, SO_ERROR, &val, &len) == -1
	    || val != 0)
	{
		goto reconnect;
	}

	g_get_current_time(&timeval);
	conn->last_activity = timeval.tv_sec;

	conn->sources[s_read] = sashimi_io_add_watch(conn, G_IO_IN | G_IO_HUP | G_IO_ERR, sashimi_read);
	conn->sources[s_ping] = sashimi_timeout_add_seconds(conn, 1, sashimi_ping);
	conn->sources[s_queue] = sashimi_timeout_add_seconds(conn, 1, sashimi_queue_runner);

	conn->sources[s_write] = 0;

	if (conn->connect.callback)
	{
		conn->connect.callback(conn->connect.data);
	}

	return FALSE;

reconnect:
	sashimi_remove_sources(conn, s_write);

	if (conn->reconnect.callback)
	{
		conn->reconnect.callback(conn->reconnect.data);
	}

	return FALSE;
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

	conn->channel = NULL;
	conn->last_activity = 0;
	conn->timeout = 0;

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

gboolean sashimi_connect (sashimiConnection* conn, const gchar* address, guint port)
{
	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(address != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);

	sashimi_disconnect(conn);

	if ((conn->channel = i_io_channel_unix_new_address(address, port, TRUE)) == NULL)
	{
		return FALSE;
	}

	g_io_channel_set_close_on_unref(conn->channel, TRUE);
	g_io_channel_set_encoding(conn->channel, NULL, NULL);

	conn->sources[s_write] = sashimi_io_add_watch(conn, G_IO_OUT | G_IO_HUP | G_IO_ERR, sashimi_write);

	return TRUE;
}

gboolean sashimi_disconnect (sashimiConnection* conn)
{
	g_return_val_if_fail(conn != NULL, FALSE);

	sashimi_remove_sources(conn, -1);

	if (conn->channel != NULL)
	{
		g_io_channel_shutdown(conn->channel, FALSE, NULL);
		g_io_channel_unref(conn->channel);
		conn->channel = NULL;
	}

	return TRUE;
}

void sashimi_free (sashimiConnection* conn)
{
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

	g_free(conn);
}

gboolean sashimi_send (sashimiConnection* conn, const gchar* message)
{
	GIOStatus status;
	gint ret = TRUE;
	gchar* tmp;

	g_return_val_if_fail(conn != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	if (conn->channel == NULL)
	{
		return FALSE;
	}

	tmp = g_strconcat(message, "\r\n", NULL);

	if ((status = i_io_channel_write_chars(conn->channel, tmp, -1, NULL, NULL)) == G_IO_STATUS_NORMAL)
	{
		g_io_channel_flush(conn->channel, NULL);
		g_printerr("OUT: %s", tmp);
	}
	else
	{
		g_printerr("WRITE_STATUS %d\n", status);
		ret = FALSE;
	}

	g_free(tmp);

	return ret;
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

	if (g_queue_is_empty(conn->queue))
	{
		return sashimi_send(conn, message);
	}
	else
	{
		return sashimi_queue(conn, message);
	}
}
