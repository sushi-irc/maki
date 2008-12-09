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

#define G_DISABLE_DEPRECATED
#define _XOPEN_SOURCE

#include <glib.h>
#include <glib/gstdio.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ilib.h"

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

GIOStatus i_io_channel_write_chars (GIOChannel* channel, const gchar* buf, gssize count, gsize* bytes_written, GError** error)
{
	gsize nwritten = 0;
	gsize ntmp;
	GIOStatus ret;

	g_return_val_if_fail(channel != NULL, FALSE);
	g_return_val_if_fail(buf != NULL, FALSE);

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

			if (nwritten == count)
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

	g_return_val_if_fail(channel != NULL, FALSE);
	g_return_val_if_fail(buf != NULL, FALSE);

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

gboolean i_str_case_equal (gconstpointer v1, gconstpointer v2)
{
	return (g_ascii_strcasecmp(v1, v2) == 0);
}

guint i_str_case_hash (gconstpointer key)
{
	guint ret;
	gchar* tmp;

	tmp = g_ascii_strdown(key, -1);
	ret = g_str_hash(tmp);
	g_free(tmp);

	return ret;
}

gchar* i_get_current_time_string (void)
{
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

	if (strftime(buf, 1024, "%Y-%m-%d %H:%M:%S", &tm) > 0)
	{
		return g_strdup(buf);
	}
	else
	{
		return NULL;
	}
}
