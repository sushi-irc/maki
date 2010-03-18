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

#include "instance.h"
#include "log.h"
#include "server.h"

#include "ilib.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct maki_log
{
	GIOChannel* channel;
};

makiLog* maki_log_new (makiInstance* inst, const gchar* server, const gchar* name)
{
	gchar* dirname;
	gchar* filename;
	gchar* path;
	gchar* logs_dir;
	makiLog* log;

	log = g_new(makiLog, 1);

	logs_dir = maki_instance_config_get_string(inst, "directories", "logs");
	filename = g_strconcat(name, ".txt", NULL);
	path = g_build_filename(logs_dir, server, filename, NULL);
	dirname = g_path_get_dirname(path);

	g_mkdir_with_parents(dirname, 0777);

	g_free(dirname);
	g_free(filename);
	g_free(logs_dir);

	if ((log->channel = g_io_channel_new_file(path, "a", NULL)) == NULL)
	{
		maki_log_free(log);
		g_free(path);

		return NULL;
	}

	g_free(path);

	g_io_channel_set_close_on_unref(log->channel, TRUE);
	/*
	g_io_channel_set_encoding(log->channel, NULL, NULL);
	g_io_channel_set_buffered(log->channel, FALSE);
	*/

	return log;
}

void maki_log_free (gpointer data)
{
	makiLog* log = data;

	if (log->channel != NULL)
	{
		g_io_channel_unref(log->channel);
	}

	g_free(log);
}

void maki_log_write (makiLog* log, const gchar* message)
{
	gchar* time_str;

	if ((time_str = i_get_current_time_string("%Y-%m-%d %H:%M:%S")) != NULL)
	{
		i_io_channel_write_chars(log->channel, time_str, -1, NULL, NULL);
		i_io_channel_write_chars(log->channel, " ", -1, NULL, NULL);
		g_free(time_str);
	}

	i_io_channel_write_chars(log->channel, message, -1, NULL, NULL);
	i_io_channel_write_chars(log->channel, "\n", -1, NULL, NULL);

	g_io_channel_flush(log->channel, NULL);
}
