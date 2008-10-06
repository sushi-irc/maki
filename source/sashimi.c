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

#ifdef __FreeBSD__
#define _XOPEN_SOURCE
#else
#define _XOPEN_SOURCE 500
#endif

#include "sashimi.h"

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

	gchar* address;
	gint port;

	glong last_activity;
	guint timeout;

	GAsyncQueue* message_queue;
	gpointer message_data;

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
		void (*callback) (gpointer);
		gpointer data;
	}
	reconnect;
};

sashimiMessage* sashimi_message_new (gchar* message, gpointer data)
{
	sashimiMessage* msg;

	msg = g_new(sashimiMessage, 1);
	msg->message = message;
	msg->data = data;

	return msg;
}

void sashimi_message_free (gpointer data)
{
	sashimiMessage* msg = data;

	g_free(msg->message);
	g_free(msg);
}

static void sashimi_remove_sources (sashimiConnection* conn, gint source)
{
	gint i;

	for (i = 0; i < s_last; i++)
	{
		if (i != source && conn->sources[i] != 0)
		{
			g_source_remove(conn->sources[i]);
			conn->sources[i] = 0;
		}
	}

	if (source >= 0)
	{
		conn->sources[source] = 0;
	}
}

static gboolean sashimi_read (GIOChannel* source, GIOCondition condition, gpointer data)
{
	GIOStatus status;
	GTimeVal time;
	gchar* buffer;
	sashimiConnection* conn = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto reconnect;
	}

	g_get_current_time(&time);

	while ((status = g_io_channel_read_line(conn->channel, &buffer, NULL, NULL, NULL)) == G_IO_STATUS_NORMAL)
	{
		conn->last_activity = time.tv_sec;

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

		g_async_queue_push(conn->message_queue, sashimi_message_new(buffer, conn->message_data));
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
	GTimeVal time;
	sashimiConnection* conn = data;

	g_get_current_time(&time);

	/* If we did not hear anything from the server, send a PING. */
	if (conn->timeout > 0 && (time.tv_sec - conn->last_activity) > conn->timeout)
	{
		gchar* ping;

		ping = g_strdup_printf("PING :%ld", time.tv_sec);
		sashimi_send(conn, ping);
		g_free(ping);

		conn->last_activity = time.tv_sec;
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
	GTimeVal time;
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

	g_get_current_time(&time);
	conn->last_activity = time.tv_sec;

	conn->sources[s_read] = g_io_add_watch(conn->channel, G_IO_IN | G_IO_HUP | G_IO_ERR, sashimi_read, conn);
	conn->sources[s_ping] = g_timeout_add_seconds(1, sashimi_ping, conn);
	conn->sources[s_queue] = g_timeout_add_seconds(1, sashimi_queue_runner, conn);

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

sashimiConnection* sashimi_new (const gchar* address, gushort port, GAsyncQueue* message_queue, gpointer message_data)
{
	gint i;
	sashimiConnection* conn;

	g_return_val_if_fail(address != NULL, NULL);
	g_return_val_if_fail(message_queue != NULL, NULL);

	conn = g_new(sashimiConnection, 1);

	conn->address = g_strdup(address);
	conn->port = port;

	conn->message_queue = g_async_queue_ref(message_queue);
	conn->message_data = message_data;

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

gboolean sashimi_connect (sashimiConnection* conn)
{
	int fd;
	struct hostent* hostinfo;
	struct sockaddr_in name;

	g_return_val_if_fail(conn != NULL, FALSE);

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		return FALSE;
	}

	fcntl(fd, F_SETFL, O_NONBLOCK);

	if ((hostinfo = gethostbyname(conn->address)) == NULL)
	{
		return FALSE;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(conn->port);
	name.sin_addr = *(struct in_addr*)hostinfo->h_addr_list[0];

	if (connect(fd, (struct sockaddr*)&name, sizeof(name)) < 0)
	{
		if (errno != EINPROGRESS)
		{
			close(fd);
			return FALSE;
		}
	}

	conn->channel = g_io_channel_unix_new(fd);
	g_io_channel_set_flags(conn->channel, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(conn->channel, TRUE);
	g_io_channel_set_encoding(conn->channel, NULL, NULL);

	conn->sources[s_write] = g_io_add_watch(conn->channel, G_IO_OUT | G_IO_HUP | G_IO_ERR, sashimi_write, conn);

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

	/* Clean up the queue. */
	while (!g_queue_is_empty(conn->queue))
	{
		g_free(g_queue_pop_head(conn->queue));
	}

	g_queue_free(conn->queue);
	g_mutex_free(conn->queue_mutex);

	g_async_queue_unref(conn->message_queue);

	g_free(conn->address);

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

	if ((status = g_io_channel_write_chars(conn->channel, tmp, -1, NULL, NULL)) == G_IO_STATUS_NORMAL)
	{
		g_io_channel_flush(conn->channel, NULL);
		g_print("OUT: %s", tmp);
	}
	else
	{
		g_print("WRITE_STATUS %d\n", status);
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
	if (g_queue_is_empty(conn->queue))
	{
		return sashimi_send(conn, message);
	}
	else
	{
		return sashimi_queue(conn, message);
	}
}
