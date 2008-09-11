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

#include <glib.h>

#include "sashimi.h"

enum
{
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

static gboolean sashimi_read (GIOChannel* source, GIOCondition condition, gpointer data)
{
	GIOStatus status;
	GTimeVal time;
	gchar* buffer;
	sashimiConnection* connection = data;

	if (condition & G_IO_HUP)
	{
		goto reconnect;
	}

	g_get_current_time(&time);

	while ((status = g_io_channel_read_line(connection->channel, &buffer, NULL, NULL, NULL)) == G_IO_STATUS_NORMAL)
	{
		sashimiMessage* message;

		connection->last_activity = time.tv_sec;

		/*
		 * Remove whitespace at the end of the string.
		 */
		g_strchomp(buffer);

		/*
		 * Handle PING internally.
		 */
		if (strncmp(buffer, "PING", 4) == 0)
		{
			gchar* tmp;

			tmp = g_strconcat("PONG ", buffer + 5, NULL);
			sashimi_send(connection, tmp);
			g_free(tmp);
			g_free(buffer);

			continue;
		}

		message = sashimi_message_new(buffer, connection->message_data);

		g_async_queue_push(connection->message_queue, message);
	}

	if (status == G_IO_STATUS_EOF)
	{
		goto reconnect;
	}

	return TRUE;

reconnect:
	g_source_remove(connection->sources[s_ping]);
	g_source_remove(connection->sources[s_queue]);
	connection->sources[s_read] = 0;
	connection->sources[s_ping] = 0;
	connection->sources[s_queue] = 0;

	if (connection->reconnect.callback)
	{
		connection->reconnect.callback(connection->reconnect.data);
	}

	return FALSE;
}

static gboolean sashimi_ping (gpointer data)
{
	GTimeVal time;
	sashimiConnection* connection = data;

	g_get_current_time(&time);

	/*
	 * If we did not hear anything from the server, send a PING.
	 */
	if (connection->timeout > 0 && (time.tv_sec - connection->last_activity) > connection->timeout)
	{
		gchar* ping;

		ping = g_strdup_printf("PING :%ld", time.tv_sec);
		sashimi_send(connection, ping);
		g_free(ping);

		connection->last_activity = time.tv_sec;
	}

	return TRUE;
}

static gboolean sashimi_queue_runner (gpointer data)
{
	sashimiConnection* connection = data;

	if (!g_queue_is_empty(connection->queue))
	{
		gchar* message;

		message = g_queue_peek_head(connection->queue);

		if (sashimi_send(connection, message))
		{
			g_mutex_lock(connection->queue_mutex);
			g_queue_pop_head(connection->queue);
			g_mutex_unlock(connection->queue_mutex);
			g_free(message);
		}
	}

	return TRUE;
}

sashimiConnection* sashimi_new (const gchar* address, gushort port, GAsyncQueue* message_queue, gpointer message_data)
{
	gint i;
	sashimiConnection* connection;

	g_return_val_if_fail(address != NULL, NULL);
	g_return_val_if_fail(message_queue != NULL, NULL);

	connection = g_new(sashimiConnection, 1);

	connection->address = g_strdup(address);
	connection->port = port;

	connection->message_queue = g_async_queue_ref(message_queue);
	connection->message_data = message_data;

	connection->queue_mutex = g_mutex_new();
	connection->queue = g_queue_new();

	connection->channel = NULL;
	connection->last_activity = 0;
	connection->timeout = 0;

	for (i = 0; i < s_last; ++i)
	{
		connection->sources[i] = 0;
	}

	connection->reconnect.callback = NULL;
	connection->reconnect.data = NULL;

	return connection;
}

void sashimi_reconnect (sashimiConnection* connection, void (*callback) (gpointer), gpointer data)
{
	g_return_if_fail(connection != NULL);

	connection->reconnect.callback = callback;
	connection->reconnect.data = data;
}

void sashimi_timeout (sashimiConnection* connection, guint timeout)
{
	g_return_if_fail(connection != NULL);

	connection->timeout = timeout;
}

gboolean sashimi_connect (sashimiConnection* connection)
{
	int fd;
	GTimeVal time;
	struct hostent* hostinfo;
	struct sockaddr_in name;

	g_return_val_if_fail(connection != NULL, FALSE);

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		return FALSE;
	}

	fcntl(fd, F_SETFL, O_NONBLOCK);

	if ((hostinfo = gethostbyname(connection->address)) == NULL)
	{
		return FALSE;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(connection->port);
	name.sin_addr = *(struct in_addr*)hostinfo->h_addr_list[0];

	if (connect(fd, (struct sockaddr*)&name, sizeof(name)) < 0)
	{
		if (errno != EINPROGRESS)
		{
			close(fd);
			return FALSE;
		}
		else
		{
			int val;
			socklen_t len = sizeof(val);
			struct pollfd fds[1];

			fds[0].fd = fd;
			fds[0].events = POLLOUT;

			poll(fds, 1, 3000);

			if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &val, &len) == -1
			    || val != 0)
			{
				return FALSE;
			}
		}
	}

	g_get_current_time(&time);
	connection->last_activity = time.tv_sec;

	connection->channel = g_io_channel_unix_new(fd);
	g_io_channel_set_flags(connection->channel, G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(connection->channel, TRUE);
	g_io_channel_set_encoding(connection->channel, NULL, NULL);

	connection->sources[s_read] = g_io_add_watch(connection->channel, G_IO_IN | G_IO_HUP, sashimi_read, connection);
	connection->sources[s_ping] = g_timeout_add_seconds(1, sashimi_ping, connection);
	connection->sources[s_queue] = g_timeout_add_seconds(1, sashimi_queue_runner, connection);

	return TRUE;
}

gboolean sashimi_disconnect (sashimiConnection* connection)
{
	gint i;

	g_return_val_if_fail(connection != NULL, FALSE);

	for (i = 0; i < s_last; ++i)
	{
		if (connection->sources[i] != 0)
		{
			g_source_remove(connection->sources[i]);
			connection->sources[i] = 0;
		}
	}

	if (connection->channel != NULL)
	{
		g_io_channel_shutdown(connection->channel, FALSE, NULL);
		g_io_channel_unref(connection->channel);
		connection->channel = NULL;
	}

	return TRUE;
}

void sashimi_free (sashimiConnection* connection)
{
	g_return_if_fail(connection != NULL);

	/* Clean up the queue. */
	while (!g_queue_is_empty(connection->queue))
	{
		g_free(g_queue_pop_head(connection->queue));
	}

	g_queue_free(connection->queue);
	g_mutex_free(connection->queue_mutex);

	g_async_queue_unref(connection->message_queue);

	g_free(connection->address);

	g_free(connection);
}

gboolean sashimi_send (sashimiConnection* connection, const gchar* message)
{
	GIOStatus status;
	gint ret = TRUE;
	gchar* tmp;

	g_return_val_if_fail(connection != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	if (connection->channel == NULL)
	{
		return FALSE;
	}

	tmp = g_strconcat(message, "\r\n", NULL);

	if ((status = g_io_channel_write_chars(connection->channel, tmp, -1, NULL, NULL)) == G_IO_STATUS_NORMAL)
	{
		g_io_channel_flush(connection->channel, NULL);
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

gboolean sashimi_queue (sashimiConnection* connection, const gchar* message)
{
	g_return_val_if_fail(connection != NULL, FALSE);
	g_return_val_if_fail(message != NULL, FALSE);

	g_mutex_lock(connection->queue_mutex);
	g_queue_push_tail(connection->queue, g_strdup(message));
	g_mutex_unlock(connection->queue_mutex);

	return TRUE;
}

gboolean sashimi_send_or_queue (sashimiConnection* connection, const gchar* message)
{
	if (g_queue_is_empty(connection->queue))
	{
		return sashimi_send(connection, message);
	}
	else
	{
		return sashimi_queue(connection, message);
	}
}
