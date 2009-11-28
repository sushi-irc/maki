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

#include "maki.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_NICE
#include <interfaces.h>
#endif

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
	/* FIXME leak */
	static GIOChannel* channel = NULL;
	gchar* message;
	va_list args;

	if (!opt_verbose)
	{
		return;
	}

	if (G_UNLIKELY(channel == NULL))
	{
		gchar* cache_dir;
		gchar* path;

		cache_dir = g_build_filename(g_get_user_cache_dir(), "sushi", NULL);
		path = g_build_filename(cache_dir, "maki.txt", NULL);

		g_mkdir_with_parents(cache_dir, 0777);

		if ((channel = g_io_channel_new_file(path, "w", NULL)) == NULL)
		{
			g_printerr("%s\n", _("Could not open debug log file."));
		}

		g_free(cache_dir);
		g_free(path);
	}

	va_start(args, format);
	message = g_strdup_vprintf(format, args);
	va_end(args);

	g_printerr("%s", message);

	if (G_LIKELY(channel != NULL))
	{
		i_io_channel_write_chars(channel, message, -1, NULL, NULL);
		g_io_channel_flush(channel, NULL);
	}

	g_free(message);
}

void maki_log (makiServer* serv, const gchar* name, const gchar* format, ...)
{
	gchar* file;
	gchar* file_tmp;
	gchar* file_format;
	gchar* tmp;
	makiLog* log;
	va_list args;
	makiInstance* inst = maki_instance_get_default();

	if (!maki_instance_config_get_boolean(inst, "logging", "enabled"))
	{
		return;
	}

	file_format = maki_instance_config_get_string(inst, "logging", "format");
	file_tmp = i_strreplace(file_format, "$n", name, 0);
	file = i_get_current_time_string(file_tmp);

	g_free(file_format);
	g_free(file_tmp);

	if (file == NULL)
	{
		return;
	}

	if ((log = g_hash_table_lookup(serv->logs, file)) == NULL)
	{
		if ((log = maki_log_new(inst, maki_server_name(serv), file)) == NULL)
		{
			return;
		}

		g_hash_table_insert(serv->logs, g_strdup(file), log);
	}

	g_free(file);

	va_start(args, format);
	tmp = g_strdup_vprintf(format, args);
	va_end(args);

	maki_log_write(log, tmp);

	g_free(tmp);
}

void maki_ensure_string (gchar** string)
{
	if (*string == NULL)
	{
		*string = g_strdup("");
	}
}

void maki_ensure_string_array (gchar*** string_array)
{
	if (*string_array == NULL)
	{
		*string_array = g_new(gchar*, 1);
		(*string_array)[0] = NULL;
	}
}
