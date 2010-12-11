/*
 * Copyright (c) 2008-2010 Michael Kuhn
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
#define _XOPEN_SOURCE 500

#include <glib.h>
#include <glib/gstdio.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ilib.h"

struct i_lock
{
	gchar* path;
	gint fd;
};

gboolean i_daemon (gboolean nochdir, gboolean noclose)
{
	pid_t pid;

	pid = fork();

	if (pid > 0)
	{
		_exit(0);
	}
	else if (pid == -1)
	{
		return FALSE;
	}

	if (setsid() == -1)
	{
		return FALSE;
	}

	if (!nochdir)
	{
		if (g_chdir("/") == -1)
		{
			return FALSE;
		}
	}

	if (!noclose)
	{
		gint fd;

		fd = open("/dev/null", O_RDWR);

		if (fd == -1)
		{
			return FALSE;
		}

		if (dup2(fd, STDIN_FILENO) == -1 || dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1)
		{
			return FALSE;
		}

		if (fd > 2)
		{
			close(fd);
		}
	}

	return TRUE;
}

guint i_io_add_watch (GIOChannel* channel, GIOCondition condition, GIOFunc func, gpointer data, GMainContext* context)
{
	GSource* source;
	guint id;

	g_return_val_if_fail(channel != NULL, 0);
	g_return_val_if_fail(func != NULL, 0);

	source = g_io_create_watch(channel, condition);
	g_source_set_callback(source, (GSourceFunc)func, data, NULL);
	id = g_source_attach(source, context);
	g_source_unref(source);

	return id;
}

guint i_idle_add (GSourceFunc function, gpointer data, GMainContext* context)
{
	GSource* source;
	guint id;

	g_return_val_if_fail(function != NULL, 0);

	source = g_idle_source_new();
	g_source_set_callback(source, function, data, NULL);
	id = g_source_attach(source, context);
	g_source_unref(source);

	return id;
}

guint i_timeout_add_seconds (guint interval, GSourceFunc function, gpointer data, GMainContext* context)
{
	GSource* source;
	guint id;

	g_return_val_if_fail(function != NULL, 0);

	source = g_timeout_source_new_seconds(interval);
	g_source_set_callback(source, function, data, NULL);
	id = g_source_attach(source, context);
	g_source_unref(source);

	return id;
}

gboolean i_source_remove (guint tag, GMainContext* context)
{
	GSource* source;

	g_return_val_if_fail(tag > 0, FALSE);

	source = g_main_context_find_source_by_id(context, tag);

	if (source != NULL)
	{
		g_source_destroy(source);
	}

	return (source != NULL);
}

GIOChannel* i_io_channel_unix_new_address (const gchar* address, guint port, gboolean nonblocking)
{
	GIOChannel* channel;
	gint fd = -1;
	gchar* port_str;
	struct addrinfo* ai;
	struct addrinfo* p;
	struct addrinfo hints;

	g_return_val_if_fail(address != NULL, NULL);
	g_return_val_if_fail(port != 0, NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

	port_str = g_strdup_printf("%u", port);

	if (getaddrinfo(address, port_str, &hints, &ai) != 0)
	{
		g_free(port_str);
		return NULL;
	}

	g_free(port_str);

	for (p = ai; p != NULL; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
		{
			continue;
		}

		if (nonblocking)
		{
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
		}

		if (connect(fd, p->ai_addr, p->ai_addrlen) < 0)
		{
			if (!nonblocking || errno != EINPROGRESS)
			{
				close(fd);
				fd = -1;
				continue;
			}
		}

		break;
	}

	freeaddrinfo(ai);

	if (fd < 0)
	{
		return NULL;
	}

	channel = g_io_channel_unix_new(fd);

	if (nonblocking)
	{
		g_io_channel_set_flags(channel, g_io_channel_get_flags(channel) | G_IO_FLAG_NONBLOCK, NULL);
	}

	return channel;
}

GIOChannel* i_io_channel_unix_new_listen (const gchar* address, guint port, gboolean nonblocking)
{
	GIOChannel* channel;
	gint fd = -1;
	gchar* port_str;
	struct addrinfo* ai;
	struct addrinfo* p;
	struct addrinfo hints;

	g_return_val_if_fail(port != 0, NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_PASSIVE;

	port_str = g_strdup_printf("%u", port);

	if (getaddrinfo(address, port_str, &hints, &ai) != 0)
	{
		g_free(port_str);
		return NULL;
	}

	g_free(port_str);

	for (p = ai; p != NULL; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
		{
			continue;
		}

		if (nonblocking)
		{
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
		}

		if (bind(fd, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}

	freeaddrinfo(ai);

	if (fd < 0)
	{
		return NULL;
	}

	if (listen(fd, 128) < 0)
	{
		close(fd);
		return NULL;
	}

	channel = g_io_channel_unix_new(fd);

	if (nonblocking)
	{
		g_io_channel_set_flags(channel, g_io_channel_get_flags(channel) | G_IO_FLAG_NONBLOCK, NULL);
	}

	return channel;
}

GIOStatus i_io_channel_write_chars (GIOChannel* channel, const gchar* buf, gssize count, gsize* bytes_written, GError** error)
{
	gsize nwritten = 0;
	gsize ntmp;
	GIOStatus ret;

	g_return_val_if_fail(channel != NULL, G_IO_STATUS_ERROR);
	g_return_val_if_fail(buf != NULL, G_IO_STATUS_ERROR);

	if (count < 0)
	{
		count = strlen(buf);
	}

	while (TRUE)
	{
		ret = g_io_channel_write_chars(channel, buf + nwritten, count - nwritten, &ntmp, error);

		if (ntmp > 0)
		{
			nwritten += ntmp;

			if (nwritten == (gsize)count)
			{
				break;
			}
		}

		if (ret != G_IO_STATUS_NORMAL && ret != G_IO_STATUS_AGAIN)
		{
			break;
		}
	}

	if (bytes_written != NULL)
	{
		*bytes_written = nwritten;
	}

	return ret;
}

GIOStatus i_io_channel_read_chars (GIOChannel* channel, gchar* buf, gsize count, gsize* bytes_read, GError** error)
{
	gsize nread = 0;
	gsize ntmp;
	GIOStatus ret;

	g_return_val_if_fail(channel != NULL, G_IO_STATUS_ERROR);
	g_return_val_if_fail(buf != NULL, G_IO_STATUS_ERROR);

	while (TRUE)
	{
		ret = g_io_channel_read_chars(channel, buf + nread, count - nread, &ntmp, error);

		if (ntmp > 0)
		{
			nread += ntmp;

			if (nread == count)
			{
				break;
			}
		}

		if (ret != G_IO_STATUS_NORMAL && ret != G_IO_STATUS_AGAIN)
		{
			break;
		}
	}

	if (bytes_read != NULL)
	{
		*bytes_read = nread;
	}

	return ret;
}

GIOStatus i_io_channel_flush (GIOChannel* channel, GError** error)
{
	GIOStatus ret;

	g_return_val_if_fail(channel != NULL, G_IO_STATUS_ERROR);

	while (TRUE)
	{
		ret = g_io_channel_flush(channel, error);

		if (ret != G_IO_STATUS_AGAIN)
		{
			break;
		}
	}

	return ret;
}

gboolean i_key_file_to_file (GKeyFile* key_file, const gchar* file, gsize* length, GError** error)
{
	gboolean ret = FALSE;
	gchar* contents;
	gsize len;

	if ((contents = g_key_file_to_data(key_file, &len, error)) != NULL)
	{
		ret = g_file_set_contents(file, contents, len, error);
		g_free(contents);
	}

	if (length != NULL)
	{
		*length = len;
	}

	return ret;
}

gchar* i_strreplace (const gchar* string, const gchar* search, const gchar* replace, guint max)
{
	gchar** p;
	gchar* result;

	p = g_strsplit(string, search, (max > 0) ? max + 1 : 0);
	result = g_strjoinv(replace, p);
	g_strfreev(p);

	return result;
}

gboolean i_ascii_str_case_equal (gconstpointer v1, gconstpointer v2)
{
	return (g_ascii_strcasecmp(v1, v2) == 0);
}

guint i_ascii_str_case_hash (gconstpointer key)
{
	guint ret;
	gchar* tmp;

	tmp = g_ascii_strdown(key, -1);
	ret = g_str_hash(tmp);
	g_free(tmp);

	return ret;
}

gchar* i_get_current_time_string (const gchar* format)
{
#if GLIB_CHECK_VERSION(2,26,0)
	GDateTime* dt;
	gchar* t;

	dt = g_date_time_new_now_local();
	t = g_date_time_format(dt, format);
	g_date_time_unref(dt);

	return t;
#else
	gchar buf[1024];
	time_t t;
	struct tm tm;

	tzset();

	if (time(&t) == -1)
	{
		return NULL;
	}

	if (localtime_r(&t, &tm) == NULL)
	{
		return NULL;
	}

	if (strftime(buf, 1024, format, &tm) > 0)
	{
		return g_strdup(buf);
	}
	else
	{
		return NULL;
	}
#endif
}

gchar** i_strv_new (IStrvNewFunc func, ...)
{
	gsize n = 0;
	gchar* p;
	gchar** v;
	va_list ap;

	va_start(ap, func);

	while (va_arg(ap, gchar*) != NULL)
	{
		n++;
	}

	va_end(ap);

	v = g_new(gchar*, n + 1);
	n = 0;

	va_start(ap, func);

	while ((p = va_arg(ap, gchar*)) != NULL)
	{
		v[n] = (func != NULL) ? func(p) : p;
		n++;
	}

	va_end(ap);

	v[n] = NULL;

	return v;
}

iLock* i_lock_new (const gchar* path)
{
	iLock* lock;

	g_return_val_if_fail(path != NULL, NULL);

	lock = g_new(iLock, 1);
	lock->path = g_strdup(path);
	lock->fd = -1;

	return lock;
}

gboolean i_lock_lock (iLock* lock, const gchar* contents)
{
	gint fd;

	g_return_val_if_fail(lock != NULL, FALSE);

	if ((fd = g_open(lock->path, O_CREAT | O_WRONLY, 0600)) == -1)
	{
		goto error;
	}

	if (lockf(fd, F_TLOCK, 0) == -1)
	{
		goto error;
	}

	if (ftruncate(fd, 0) == -1)
	{
		g_unlink(lock->path);

		goto error;
	}

	if (contents != NULL)
	{
		gsize len;
		gsize nwritten = 0;
		gssize ret;

		len = strlen(contents);

		while (nwritten < len)
		{
			ret = write(fd, contents + nwritten, len - nwritten);

			if (ret > 0)
			{
				nwritten += ret;

				if (nwritten == len)
				{
					break;
				}
			}

			if (ret == 0 || ret == -1)
			{
				g_unlink(lock->path);

				goto error;
			}
		}
	}

	lock->fd = fd;

	return TRUE;

error:
	if (fd != -1)
	{
		close(fd);
	}

	return FALSE;
}

gboolean i_lock_unlock (iLock* lock)
{
	g_return_val_if_fail(lock != NULL, FALSE);

	if (lock->fd == -1)
	{
		return FALSE;
	}

	close(lock->fd);
	lock->fd = -1;
	g_unlink(lock->path);

	return TRUE;
}

void i_lock_free (iLock* lock)
{
	g_return_if_fail(lock != NULL);

	g_free(lock->path);
	g_free(lock);
}
