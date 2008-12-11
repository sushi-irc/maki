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
#include <unistd.h>

gboolean maki_config_is_empty (const gchar* value)
{
	gboolean ret = TRUE;

	if (value != NULL && value[0] != '\0')
	{
		ret = FALSE;
	}

	return ret;
}

gboolean maki_config_is_empty_list (gchar** list)
{
	gboolean ret = TRUE;

	if (list != NULL && g_strv_length(list) > 0)
	{
		ret = FALSE;
	}

	return ret;
}

void maki_debug (const gchar* format, ...)
{
	static int fd = -1;
	gchar* message;
	va_list args;
	makiInstance* inst = maki_instance_get_default();

	if (!opt_debug)
	{
		return;
	}

	if (G_UNLIKELY(fd == -1))
	{
		gchar* filename;
		gchar* path;
		gchar* logs_dir;

		filename = g_strconcat("maki", ".txt", NULL);
		logs_dir = maki_instance_config_get_string(inst, "directories", "logs");
		path = g_build_filename(logs_dir, filename, NULL);

		g_mkdir_with_parents(logs_dir, 0777);

		if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1)
		{
			g_print("%s\n", _("Could not open debug log file."));
		}

		g_free(filename);
		g_free(logs_dir);
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

void maki_log (makiServer* serv, const gchar* name, const gchar* format, ...)
{
	gchar* tmp;
	makiLog* log;
	va_list args;
	makiInstance* inst = maki_instance_get_default();

	if (!maki_instance_config_get_boolean(inst, "logging", "enabled"))
	{
		return;
	}

	if ((log = g_hash_table_lookup(serv->logs, name)) == NULL)
	{
		if ((log = maki_log_new(inst, serv->server, name)) == NULL)
		{
			return;
		}

		g_hash_table_insert(serv->logs, g_strdup(name), log);
	}

	va_start(args, format);
	tmp = g_strdup_vprintf(format, args);
	va_end(args);

	maki_log_write(log, tmp);

	g_free(tmp);
}
