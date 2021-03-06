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

#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ilib.h>

#include "dcc_send.h"

#include "dbus.h"
#include "instance.h"
#include "network.h"
#include "server.h"

enum
{
	s_new = (1 << 0),
	s_incoming = (1 << 1),
	s_resumable = (1 << 2),
	s_resumed = (1 << 3),
	s_running = (1 << 4),
	s_error = (1 << 5)
};

enum
{
	s_in_read,
	s_in_write,
	s_in_num
};

enum
{
	s_out_listen,
	s_out_read,
	s_out_write,
	s_out_num
};

/* FIXME support IPv6 */
struct maki_dcc_send
{
	makiServer* server;
	guint64 id;

	makiUser* user;

	struct
	{
		GIOChannel* connection;
		GIOChannel* file;
	}
	channel;

	gchar* path;

	goffset position;
	goffset size;
	goffset resume;

	guint32 address;
	guint16 port;

	guint32 token;

	guint64 status;

	GTimeVal start_time;

	union
	{
		struct
		{
			gboolean accept;
			gboolean resume;

			guint sources[s_in_num];
		}
		in;

		struct
		{
			struct
			{
				guint32 position;
				guint32 buffer;
				gsize offset;
			}
			ack;

			gboolean wait;

			guint sources[s_out_num];

			gboolean upnp;
		}
		out;
	}
	d;
};

static void maki_dcc_send_close (makiDCCSend* dcc)
{
	if (!(dcc->status & s_incoming) && dcc->d.out.upnp)
	{
		makiInstance* inst = maki_instance_get_default();
		makiNetwork* net = maki_instance_network(inst);

		maki_network_upnp_remove_port(net, dcc->port);
		dcc->d.out.upnp = FALSE;
	}

	if (dcc->channel.connection != NULL)
	{
		g_io_channel_unref(dcc->channel.connection);
		dcc->channel.connection = NULL;
	}

	if (dcc->channel.file != NULL)
	{
		g_io_channel_unref(dcc->channel.file);
		dcc->channel.file = NULL;
	}
}

static gboolean maki_dcc_send_in_read (GIOChannel* source, GIOCondition condition, gpointer data)
{
	gchar buffer[1024];
	gsize bytes_read;
	GIOStatus status;
	makiDCCSend* dcc = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto error;
	}

	while ((status = g_io_channel_read_chars(source, buffer, 1024, &bytes_read, NULL)) == G_IO_STATUS_NORMAL)
	{
		guint32 pos;

		dcc->position += bytes_read;
		pos = htonl(dcc->position);

		i_io_channel_write_chars(dcc->channel.file, buffer, bytes_read, NULL, NULL);
		g_io_channel_flush(dcc->channel.file, NULL);

		i_io_channel_write_chars(source, (gchar*)&pos, sizeof(pos), NULL, NULL);
		g_io_channel_flush(source, NULL);

		if (dcc->size > 0 && dcc->position >= dcc->size)
		{
			goto finish;
		}
	}

	if (status == G_IO_STATUS_ERROR)
	{
		goto error;
	}

	return TRUE;

error:
	dcc->status |= s_error;
finish:
	dcc->status &= ~s_running;

	maki_dcc_send_close(dcc);

	dcc->d.in.sources[s_in_read] = 0;

	maki_dcc_send_emit(dcc);

	return FALSE;
}

static gboolean maki_dcc_send_in_write (GIOChannel* source, GIOCondition condition, gpointer data)
{
	gint val;
	socklen_t len = sizeof(val);
	makiDCCSend* dcc = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto error;
	}

	if (getsockopt(g_io_channel_unix_get_fd(source), SOL_SOCKET, SO_ERROR, &val, &len) == -1
	    || val != 0)
	{
		goto error;
	}

	g_get_current_time(&dcc->start_time);

	dcc->status |= s_running;

	dcc->d.in.sources[s_in_write] = 0;
	dcc->d.in.sources[s_in_read] = g_io_add_watch(source, G_IO_IN | G_IO_HUP | G_IO_ERR, maki_dcc_send_in_read, dcc);

	maki_dcc_send_emit(dcc);

	return FALSE;

error:
	dcc->status |= s_error;
	dcc->status &= ~s_running;

	maki_dcc_send_close(dcc);

	dcc->d.in.sources[s_in_write] = 0;

	maki_dcc_send_emit(dcc);

	return FALSE;
}

static gboolean maki_dcc_send_out_write (GIOChannel* source, GIOCondition condition, gpointer data)
{
	gchar buffer[1024];
	gsize bytes_read;
	GIOStatus status;
	makiDCCSend* dcc = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto error;
	}

	if ((status = g_io_channel_read_chars(dcc->channel.file, buffer, 1024, &bytes_read, NULL)) != G_IO_STATUS_ERROR)
	{
		if (bytes_read > 0)
		{
			dcc->position += bytes_read;

			i_io_channel_write_chars(source, buffer, bytes_read, NULL, NULL);
			g_io_channel_flush(source, NULL);
		}

		if (dcc->position >= dcc->size || status == G_IO_STATUS_EOF)
		{
			if (dcc->d.out.ack.position < dcc->size)
			{
				dcc->d.out.wait = TRUE;
			}

			goto finish;
		}
	}

	if (status == G_IO_STATUS_ERROR)
	{
		goto error;
	}

	return TRUE;

error:
	dcc->status |= s_error;
finish:
	if (!dcc->d.out.wait)
	{
		dcc->status &= ~s_running;

		maki_dcc_send_close(dcc);

		maki_dcc_send_emit(dcc);
	}

	dcc->d.out.sources[s_out_write] = 0;

	return FALSE;
}

static gboolean maki_dcc_send_out_read (GIOChannel* source, GIOCondition condition, gpointer data)
{
	gsize bytes_read;
	GIOStatus status;
	makiDCCSend* dcc = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto error;
	}

	while ((status = g_io_channel_read_chars(source, ((gchar*)&dcc->d.out.ack.buffer) + dcc->d.out.ack.offset, sizeof(dcc->d.out.ack.buffer) - dcc->d.out.ack.offset, &bytes_read, NULL)) == G_IO_STATUS_NORMAL)
	{
		dcc->d.out.ack.offset += bytes_read;

		if (dcc->d.out.ack.offset != sizeof(dcc->d.out.ack.buffer))
		{
			continue;
		}

		dcc->d.out.ack.position = ntohl(dcc->d.out.ack.buffer);
		dcc->d.out.ack.offset = 0;

		if (dcc->d.out.ack.position >= dcc->size)
		{
			goto finish;
		}
	}

	if (status == G_IO_STATUS_ERROR)
	{
		goto error;
	}

	return TRUE;

error:
	dcc->status |= s_error;
finish:
	if (dcc->d.out.wait)
	{
		dcc->status &= ~s_running;

		maki_dcc_send_close(dcc);

		maki_dcc_send_emit(dcc);
	}

	dcc->d.out.sources[s_out_read] = 0;

	return FALSE;
}

static gboolean maki_dcc_send_out_listen (GIOChannel* source, GIOCondition condition, gpointer data)
{
	gint fd;
	makiDCCSend* dcc = data;

	if (condition & (G_IO_HUP | G_IO_ERR))
	{
		goto error;
	}

	fd = g_io_channel_unix_get_fd(source);
	fd = accept(fd, NULL, NULL);

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	dcc->channel.connection = g_io_channel_unix_new(fd);

	g_io_channel_set_flags(dcc->channel.connection, g_io_channel_get_flags(dcc->channel.connection) | G_IO_FLAG_NONBLOCK, NULL);
	g_io_channel_set_close_on_unref(dcc->channel.connection, TRUE);
	g_io_channel_set_encoding(dcc->channel.connection, NULL, NULL);

	g_get_current_time(&dcc->start_time);

	dcc->status |= s_running;

	dcc->d.out.sources[s_out_listen] = 0;
	dcc->d.out.sources[s_out_read] = g_io_add_watch(dcc->channel.connection, G_IO_IN | G_IO_HUP | G_IO_ERR, maki_dcc_send_out_read, dcc);
	dcc->d.out.sources[s_out_write] = g_io_add_watch(dcc->channel.connection, G_IO_OUT | G_IO_HUP | G_IO_ERR, maki_dcc_send_out_write, dcc);

	g_io_channel_unref(source);

	maki_dcc_send_emit(dcc);

	return FALSE;

error:
	dcc->status |= s_error;
	dcc->status &= ~s_running;

	maki_dcc_send_close(dcc);

	dcc->d.out.sources[s_out_listen] = 0;

	maki_dcc_send_emit(dcc);

	return FALSE;
}

gchar* maki_dcc_send_get_file_name (const gchar* string, gsize* length)
{
	gchar* file_name;
	gchar* tmp;
	gsize len = 0;

	tmp = NULL;

	if (string[len] == '"')
	{
		len++;

		while (string[len] != '\0' && string[len] != '"')
		{
			len++;
		}

		if (string[len] != '\0')
		{
			len++;
		}

		if (len > 2)
		{
			tmp = g_strndup(string + 1, len - 2);
		}
	}
	else
	{
		while (string[len] != '\0' && string[len] != ' ')
		{
			len++;
		}

		if (len > 0)
		{
			tmp = g_strndup(string, len);
		}
	}

	if (string[len] == '\0' || tmp == NULL)
	{
		g_free(tmp);
		return NULL;
	}

	file_name = g_path_get_basename(tmp);
	g_free(tmp);

	*length = len;

	return file_name;
}

makiDCCSend* maki_dcc_send_new_in (makiServer* serv, makiUser* user, const gchar* file_name, guint32 address, guint16 port, goffset file_size, guint32 token)
{
	guint i;
	gchar* downloads_dir;
	struct stat stbuf;
	makiInstance* inst = maki_instance_get_default();
	makiDCCSend* dcc;

	downloads_dir = maki_instance_config_get_string(inst, "directories", "downloads");

	dcc = g_new(makiDCCSend, 1);

	dcc->server = serv;
	dcc->id = maki_instance_get_dcc_send_id(inst);

	dcc->user = maki_user_ref(user);

	dcc->channel.file = NULL;
	dcc->channel.connection = NULL;

	dcc->path = g_build_filename(downloads_dir, maki_server_name(dcc->server), maki_user_nick(dcc->user), file_name, NULL);
	dcc->position = 0;
	dcc->size = file_size;
	dcc->resume = 0;

	dcc->address = address;
	dcc->port = port;

	dcc->token = token;

	dcc->status = s_new | s_incoming;

	dcc->d.in.accept = FALSE;
	dcc->d.in.resume = FALSE;

	for (i = 0; i < s_in_num; i++)
	{
		dcc->d.in.sources[i] = 0;
	}

	g_free(downloads_dir);

	if (stat(dcc->path, &stbuf) == 0 && stbuf.st_size > 0 && (stbuf.st_size < file_size || file_size == 0))
	{
		dcc->status |= s_resumable;
	}

	if (dcc->status & s_resumable)
	{
		if (maki_instance_config_get_boolean(inst, "dcc", "accept_resume"))
		{
			dcc->status &= ~s_new;

			maki_dcc_send_resume(dcc);
		}
	}
	else
	{
		if (maki_instance_config_get_boolean(inst, "dcc", "accept_send"))
		{
			dcc->status &= ~s_new;

			maki_dcc_send_accept(dcc);
		}
	}

	maki_dcc_send_emit(dcc);

	dcc->status &= ~s_new;

	return dcc;
}

makiDCCSend* maki_dcc_send_new_out (makiServer* serv, makiUser* user, const gchar* path)
{
	guint i;
	gchar* basename;
	GInetAddress* inet_address;
	struct stat stbuf;
	makiInstance* inst = maki_instance_get_default();
	makiNetwork* net = maki_instance_network(inst);
	makiDCCSend* dcc;

	dcc = g_new(makiDCCSend, 1);

	dcc->server = serv;
	dcc->id = maki_instance_get_dcc_send_id(inst);

	dcc->user = maki_user_ref(user);

	dcc->channel.file = NULL;
	dcc->channel.connection = NULL;

	dcc->path = g_strdup(path);
	dcc->position = 0;
	dcc->size = 0;
	dcc->resume = 0;

	dcc->address = 0;
	dcc->port = 0;

	dcc->token = 0;

	dcc->status = 0;

	dcc->d.out.ack.position = 0;
	dcc->d.out.ack.offset = 0;

	dcc->d.out.wait = FALSE;

	for (i = 0; i < s_out_num; i++)
	{
		dcc->d.out.sources[i] = 0;
	}

	dcc->d.out.upnp = FALSE;

	if (stat(dcc->path, &stbuf) != 0)
	{
		goto error;
	}

	dcc->size = stbuf.st_size;

	for (dcc->port = maki_instance_config_get_integer(inst, "dcc", "port_first"); dcc->port <= maki_instance_config_get_integer(inst, "dcc", "port_last"); dcc->port++)
	{
		if ((dcc->channel.connection = i_io_channel_unix_new_listen(NULL, dcc->port, TRUE)) != NULL)
		{
			break;
		}
	}

	if (dcc->channel.connection == NULL)
	{
		goto error;
	}

	g_io_channel_set_close_on_unref(dcc->channel.connection, TRUE);
	g_io_channel_set_encoding(dcc->channel.connection, NULL, NULL);

	if ((inet_address = maki_network_external_address(net)) != NULL)
	{
		if (g_inet_address_get_family(inet_address) == G_SOCKET_FAMILY_IPV4)
		{
			dcc->address = ntohl(*((guint32 const*)g_inet_address_to_bytes(inet_address)));
		}

		g_object_unref(inet_address);
	}
	else
	{
		makiNetworkAddress addr;
		socklen_t addrlen = sizeof(addr);

		getsockname(g_io_channel_unix_get_fd(dcc->channel.connection), &(addr.sa), &addrlen);
		dcc->address = ntohl(addr.sin.sin_addr.s_addr);
	}

	if ((dcc->channel.file = g_io_channel_new_file(dcc->path, "r", NULL)) == NULL)
	{
		goto error;
	}

	g_io_channel_set_close_on_unref(dcc->channel.file, TRUE);
	g_io_channel_set_encoding(dcc->channel.file, NULL, NULL);
	g_io_channel_set_buffered(dcc->channel.file, FALSE);

	dcc->d.out.upnp = maki_network_upnp_add_port(net, dcc->port, "maki DCC Send");

	basename = g_path_get_basename(dcc->path);

	if (strstr(basename, " ") == NULL)
	{
		maki_server_send_printf(serv, "PRIVMSG %s :\001DCC SEND %s %" G_GUINT32_FORMAT " %" G_GUINT16_FORMAT " %" G_GUINT64_FORMAT "\001", maki_user_nick(dcc->user), basename, dcc->address, dcc->port, dcc->size);
	}
	else
	{
		maki_server_send_printf(serv, "PRIVMSG %s :\001DCC SEND \"%s\" %" G_GUINT32_FORMAT " %" G_GUINT16_FORMAT " %" G_GUINT64_FORMAT "\001", maki_user_nick(dcc->user), basename, dcc->address, dcc->port, dcc->size);
	}

	g_free(basename);

	dcc->d.out.sources[s_out_listen] = g_io_add_watch(dcc->channel.connection, G_IO_IN | G_IO_HUP | G_IO_ERR, maki_dcc_send_out_listen, dcc);

	maki_dcc_send_emit(dcc);

	return dcc;

error:
	maki_dcc_send_free(dcc);

	return NULL;
}

void maki_dcc_send_free (makiDCCSend* dcc)
{
	if (dcc->status & s_incoming)
	{
		guint i;

		for (i = 0; i < s_in_num; i++)
		{
			if (dcc->d.in.sources[i] != 0)
			{
				g_source_remove(dcc->d.in.sources[i]);
			}
		}
	}
	else
	{
		guint i;

		for (i = 0; i < s_out_num; i++)
		{
			if (dcc->d.out.sources[i] != 0)
			{
				g_source_remove(dcc->d.out.sources[i]);
			}
		}
	}

	maki_dcc_send_close(dcc);

	maki_server_remove_user(dcc->server, maki_user_nick(dcc->user));

	g_free(dcc->path);
	g_free(dcc);
}

gboolean maki_dcc_send_accept (makiDCCSend* dcc)
{
	g_return_val_if_fail(dcc != NULL, FALSE);

	if (dcc->status & s_incoming)
	{
		gchar address[INET_ADDRSTRLEN];
		makiNetworkAddress addr;
		socklen_t addrlen = sizeof(addr.sin);

		if (dcc->d.in.accept)
		{
			return FALSE;
		}

		addr.sin.sin_family = AF_INET;
		addr.sin.sin_port = htons(dcc->port);
		addr.sin.sin_addr.s_addr = htonl(dcc->address);

		if (getnameinfo(&(addr.sa), addrlen, address, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
		{
			return FALSE;
		}

		if ((dcc->channel.connection = i_io_channel_unix_new_address(address, dcc->port, TRUE)) == NULL)
		{
			return FALSE;
		}

		g_io_channel_set_close_on_unref(dcc->channel.connection, TRUE);
		g_io_channel_set_encoding(dcc->channel.connection, NULL, NULL);

		if (dcc->channel.file == NULL)
		{
			gchar* dirname;

			dirname = g_path_get_dirname(dcc->path);
			g_mkdir_with_parents(dirname, 0777);
			g_free(dirname);

			if ((dcc->channel.file = g_io_channel_new_file(dcc->path, "w", NULL)) == NULL)
			{
				return FALSE;
			}

			g_io_channel_set_close_on_unref(dcc->channel.file, TRUE);
			g_io_channel_set_encoding(dcc->channel.file, NULL, NULL);
			g_io_channel_set_buffered(dcc->channel.file, FALSE);
		}

		dcc->d.in.accept = TRUE;
		dcc->d.in.sources[s_in_write] = g_io_add_watch(dcc->channel.connection, G_IO_OUT | G_IO_HUP | G_IO_ERR, maki_dcc_send_in_write, dcc);

		return TRUE;
	}

	return FALSE;
}

gboolean maki_dcc_send_resume (makiDCCSend* dcc)
{
	g_return_val_if_fail(dcc != NULL, FALSE);

	if (dcc->status & s_incoming)
	{
		gchar* basename;
		struct stat stbuf;

		if (dcc->d.in.accept || dcc->d.in.resume)
		{
			return FALSE;
		}

		if (stat(dcc->path, &stbuf) != 0 || stbuf.st_size == 0 || (stbuf.st_size >= dcc->size && dcc->size > 0))
		{
			return FALSE;
		}

		basename = g_path_get_basename(dcc->path);

		if (dcc->token > 0)
		{
			maki_server_send_printf(dcc->server, "PRIVMSG %s :\001DCC RESUME %s %" G_GUINT16_FORMAT " %" G_GINT64_FORMAT " %" G_GUINT32_FORMAT "\001", maki_user_nick(dcc->user), basename, dcc->port, stbuf.st_size, dcc->token);
		}
		else
		{
			maki_server_send_printf(dcc->server, "PRIVMSG %s :\001DCC RESUME %s %" G_GUINT16_FORMAT " %" G_GINT64_FORMAT "\001", maki_user_nick(dcc->user), basename, dcc->port, stbuf.st_size);
		}

		g_free(basename);

		dcc->d.in.resume = TRUE;

		return TRUE;
	}

	return FALSE;
}

gboolean maki_dcc_send_resume_accept (makiDCCSend* dcc, const gchar* filename, guint16 port, goffset position, guint32 token, gboolean is_incoming)
{
	g_return_val_if_fail(dcc != NULL, FALSE);

	if (dcc->status & s_incoming)
	{
		gchar* basename;
		gchar* dirname;

		if (!is_incoming)
		{
			return FALSE;
		}

		basename = g_path_get_basename(dcc->path);

		if (strcmp(basename, filename) != 0)
		{
			g_free(basename);
			return FALSE;
		}

		g_free(basename);

		if (dcc->port != port)
		{
			return FALSE;
		}

		if (dcc->d.in.accept || !dcc->d.in.resume)
		{
			return FALSE;
		}

		dirname = g_path_get_dirname(dcc->path);
		g_mkdir_with_parents(dirname, 0777);
		g_free(dirname);

		if ((dcc->channel.file = g_io_channel_new_file(dcc->path, "r+", NULL)) == NULL)
		{
			return FALSE;
		}

		g_io_channel_set_close_on_unref(dcc->channel.file, TRUE);
		g_io_channel_set_encoding(dcc->channel.file, NULL, NULL);
		g_io_channel_set_buffered(dcc->channel.file, FALSE);

		if (g_io_channel_seek_position(dcc->channel.file, position, G_SEEK_SET, NULL) != G_IO_STATUS_NORMAL)
		{
			return FALSE;
		}

		dcc->position = position;
		dcc->resume = position;

		dcc->status &= ~s_resumable;
		dcc->status |= s_resumed;

		maki_dcc_send_emit(dcc);

		maki_dcc_send_accept(dcc);
	}
	else
	{
		gchar* basename;

		if (is_incoming)
		{
			return FALSE;
		}

		basename = g_path_get_basename(dcc->path);

		if (strcmp(basename, filename) != 0)
		{
			g_free(basename);
			return FALSE;
		}

		g_free(basename);

		if (dcc->port != port)
		{
			return FALSE;
		}

		if (g_io_channel_seek_position(dcc->channel.file, position, G_SEEK_SET, NULL) != G_IO_STATUS_NORMAL)
		{
			return FALSE;
		}

		dcc->position = position;
		dcc->resume = position;
		dcc->d.out.ack.position = position;

		dcc->status &= ~s_resumable;
		dcc->status |= s_resumed;

		maki_dcc_send_emit(dcc);
	}

	return TRUE;
}

guint64 maki_dcc_send_id (makiDCCSend* dcc)
{
	return dcc->id;
}

goffset maki_dcc_send_size (makiDCCSend* dcc)
{
	return dcc->size;
}

goffset maki_dcc_send_progress (makiDCCSend* dcc)
{
	if (dcc->status & s_incoming)
	{
		return dcc->position;
	}
	else
	{
		return dcc->d.out.ack.position;
	}
}

guint64 maki_dcc_send_speed (makiDCCSend* dcc)
{
	guint64 speed = 0;

	if (dcc->status & s_running)
	{
		glong duration;
		GTimeVal timeval;

		g_get_current_time(&timeval);

		duration = MAX(timeval.tv_sec - dcc->start_time.tv_sec, 1);
		speed = (maki_dcc_send_progress(dcc) - dcc->resume) / duration;
	}

	return speed;
}

guint maki_dcc_send_status (makiDCCSend* dcc)
{
	return dcc->status;
}

makiServer* maki_dcc_send_server (makiDCCSend* dcc)
{
	return dcc->server;
}

makiUser* maki_dcc_send_user (makiDCCSend* dcc)
{
	return dcc->user;
}

const gchar* maki_dcc_send_path (makiDCCSend* dcc)
{
	return dcc->path;
}

gboolean maki_dcc_send_set_path (makiDCCSend* dcc, const gchar* path)
{
	if (dcc->status & s_incoming)
	{
		struct stat stbuf;

		if (dcc->d.in.accept || dcc->d.in.resume)
		{
			return FALSE;
		}

		g_free(dcc->path);
		dcc->path = g_strdup(path);

		if (stat(dcc->path, &stbuf) == 0 && stbuf.st_size > 0 && (stbuf.st_size < dcc->size || dcc->size == 0))
		{
			dcc->status |= s_resumable;
		}
		else
		{
			dcc->status &= ~s_resumable;
		}

		maki_dcc_send_emit(dcc);

		return TRUE;
	}

	return FALSE;
}

gchar* maki_dcc_send_filename (makiDCCSend* dcc)
{
	return g_path_get_basename(dcc->path);
}

void maki_dcc_send_emit (makiDCCSend* dcc)
{
	gchar* filename;

	filename = maki_dcc_send_filename(dcc);

	maki_dbus_emit_dcc_send(dcc->id, maki_server_name(dcc->server), maki_user_from(dcc->user), filename, dcc->size, maki_dcc_send_progress(dcc), maki_dcc_send_speed(dcc), dcc->status);

	g_free(filename);
}
