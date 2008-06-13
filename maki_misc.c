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

#include "maki.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

gpointer maki_user_new (gpointer key, gpointer data)
{
	gchar* nick = key;
	struct maki_connection* connection = data;
	struct maki_user* m_user;

	m_user = g_new(struct maki_user, 1);
	m_user->connection = connection;
	m_user->nick = nick;
	m_user->away = FALSE;

	return m_user;
}

void maki_user_copy (struct maki_user* src, struct maki_user* dst)
{
	if (src != dst)
	{
		dst->away = src->away;
	}
}

/**
 * This function gets called when a user is removed from the users hash table.
 */
void maki_user_free (gpointer value)
{
	struct maki_user* m_user = value;

	g_free(m_user);
}

struct maki_channel_user* maki_channel_user_new (struct maki_user* m_user)
{
	struct maki_channel_user* m_cuser;

	m_cuser = g_new(struct maki_channel_user, 1);
	m_cuser->user = m_user;
	m_cuser->prefix = 0;

	return m_cuser;
}

void maki_channel_user_copy (struct maki_channel_user* src, struct maki_channel_user* dst)
{
	if (src != dst)
	{
		dst->prefix = src->prefix;
	}
}

void maki_channel_user_free (gpointer data)
{
	struct maki_channel_user* m_cuser = data;

	maki_cache_remove(m_cuser->user->connection->users, m_cuser->user->nick);

	g_free(m_cuser);
}

gboolean maki_key_file_to_file (GKeyFile* key_file, const gchar* file)
{
	gboolean ret = FALSE;
	gchar* contents;

	if ((contents = g_key_file_to_data(key_file, NULL, NULL)) != NULL)
	{
		if (g_file_set_contents(file, contents, -1, NULL))
		{
			ret = TRUE;
		}

		g_free(contents);
	}

	return ret;
}

void maki_debug (const gchar* format, ...)
{
	static int fd = -1;
	gchar* message;
	va_list args;
	struct maki* m = maki();

	if (!m->opt.debug)
	{
		return;
	}

	if (G_UNLIKELY(fd == -1))
	{
		gchar* filename;
		gchar* path;

		filename = g_strconcat("maki", ".txt", NULL);
		path = g_build_filename(m->directories.logs, filename, NULL);

		if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1)
		{
			g_free(filename);
			g_free(path);
			return;
		}

		g_free(filename);
		g_free(path);
	}

	va_start(args, format);
	message = g_strdup_vprintf(format, args);
	va_end(args);

	write(fd, message, strlen(message));

	g_free(message);
}

gboolean maki_str_equal (gconstpointer v1, gconstpointer v2)
{
	return (g_ascii_strcasecmp(v1, v2) == 0);
}

guint maki_str_hash (gconstpointer key)
{
	guint ret;
	gchar* tmp;

	tmp = g_ascii_strdown(key, -1);
	ret = g_str_hash(tmp);
	g_free(tmp);

	return ret;
}
