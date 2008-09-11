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

#include <glib/gstdio.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

gboolean maki_key_file_to_file (GKeyFile* key_file, const gchar* file)
{
	gboolean ret = FALSE;
	gchar* contents;

	if ((contents = g_key_file_to_data(key_file, NULL, NULL)) != NULL)
	{
		if (g_file_set_contents(file, contents, -1, NULL))
		{
			g_chmod(file, S_IRUSR | S_IWUSR);
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
		path = g_build_filename(maki_config_get(m->config, "directories", "logs"), filename, NULL);

		if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		{
			g_print(_("Could not open debug log file.\n"));
		}

		g_free(filename);
		g_free(path);
	}

	va_start(args, format);
	message = g_strdup_vprintf(format, args);
	va_end(args);

	g_print("%s", message);

	if (G_LIKELY(fd != -1))
	{
		maki_write(fd, message);
	}

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

gboolean maki_write (gint fd, const gchar* buf)
{
	gssize size;
	gssize written;

	size = strlen(buf);

	if (G_UNLIKELY((written = write(fd, buf, size)) != size))
	{
		gssize tmp;

		while (written < size && (tmp = write(fd, buf + written, size - written)) > 0)
		{
			written += tmp;
		}
	}

	return (written >= size);
}

gint maki_send_printf (makiServer* serv, const gchar* format, ...)
{
	gint ret;
	gchar* buffer;
	va_list args;

	va_start(args, format);
	buffer = g_strdup_vprintf(format, args);
	va_end(args);

	ret = sashimi_send(serv->connection, buffer);
	g_free(buffer);

	return ret;
}
