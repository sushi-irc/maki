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
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = 0;

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

		if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0)
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


struct i_cache
{
	gpointer (*value_new) (gpointer, gpointer);
	void (*value_free) (gpointer);
	gpointer value_data;

	GHashTable* hash_table;
};

struct i_cache_item
{
	gint ref_count;
	gpointer value;
};

typedef struct i_cache_item iCacheItem;

static iCacheItem* i_cache_item_new (gpointer value)
{
	iCacheItem* item;

	item = g_new(iCacheItem, 1);
	item->ref_count = 1;
	item->value = value;

	return item;
}

static void i_cache_item_free (gpointer data)
{
	iCacheItem* item = data;

	g_free(item);
}

iCache* i_cache_new (iCacheNewFunc value_new, iCacheFreeFunc value_free, gpointer value_data, GHashFunc key_hash, GEqualFunc key_equal)
{
	iCache* cache;

	g_return_val_if_fail(value_new != NULL, NULL);
	g_return_val_if_fail(value_free != NULL, NULL);
	g_return_val_if_fail(key_hash != NULL, NULL);
	g_return_val_if_fail(key_equal != NULL, NULL);

	cache = g_new(iCache, 1);
	cache->value_new = value_new;
	cache->value_free = value_free;
	cache->value_data = value_data;
	cache->hash_table = g_hash_table_new_full(key_hash, key_equal, g_free, i_cache_item_free);

	return cache;
}

void i_cache_free (iCache* cache)
{
	g_return_if_fail(cache != NULL);

	g_hash_table_destroy(cache->hash_table);
	g_free(cache);
}

gpointer i_cache_insert (iCache* cache, const gchar* key)
{
	iCacheItem* item;

	g_return_val_if_fail(cache != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);

	if ((item = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		item->ref_count++;

		return item->value;
	}
	else
	{
		gchar* new_key;
		gpointer value;

		new_key = g_strdup(key);
		value = (*cache->value_new)(new_key, cache->value_data);

		item = i_cache_item_new(value);

		g_hash_table_insert(cache->hash_table, new_key, item);

		return value;
	}
}

gpointer i_cache_lookup (iCache* cache, const gchar* key)
{
	iCacheItem* item;

	g_return_val_if_fail(cache != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);

	if ((item = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		return item->value;
	}
	else
	{
		return NULL;
	}
}

void i_cache_remove (iCache* cache, const gchar* key)
{
	iCacheItem* item;

	g_return_if_fail(cache != NULL);
	g_return_if_fail(key != NULL);

	if ((item = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		item->ref_count--;

		if (item->ref_count == 0)
		{
			(*cache->value_free)(item->value);
			g_hash_table_remove(cache->hash_table, key);
		}
	}
}
