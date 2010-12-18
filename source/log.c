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

#include <gio/gio.h>

struct maki_log
{
	GDataOutputStream* stream;
};

makiLog* maki_log_new (makiInstance* inst, const gchar* server, const gchar* name)
{
	makiLog* log = NULL;
	GFile* file;
	GFileOutputStream* file_output;
	gchar* dirname;
	gchar* filename;
	gchar* path;
	gchar* logs_dir;

	logs_dir = maki_instance_config_get_string(inst, "directories", "logs");
	filename = g_strconcat(name, ".txt", NULL);
	path = g_build_filename(logs_dir, server, filename, NULL);
	dirname = g_path_get_dirname(path);

	g_mkdir_with_parents(dirname, 0777);
	file = g_file_new_for_path(path);

	g_free(logs_dir);
	g_free(filename);
	g_free(path);
	g_free(dirname);

	if ((file_output = g_file_append_to(file, G_FILE_CREATE_PRIVATE, NULL, NULL)) != NULL)
	{
		log = g_new(makiLog, 1);
		log->stream = g_data_output_stream_new(G_OUTPUT_STREAM(file_output));

		g_object_unref(file_output);
	}

	g_object_unref(file);

	return log;
}

void maki_log_free (gpointer data)
{
	makiLog* log = data;

	if (log->stream != NULL)
	{
		g_object_unref(log->stream);
	}

	g_free(log);
}

void maki_log_write (makiLog* log, const gchar* message)
{
	gchar* time_str;

	if ((time_str = i_get_current_time_string("%Y-%m-%d %H:%M:%S")) != NULL)
	{
		g_data_output_stream_put_string(log->stream, time_str, NULL, NULL);
		g_data_output_stream_put_string(log->stream, " ", NULL, NULL);
		g_free(time_str);
	}

	g_data_output_stream_put_string(log->stream, message, NULL, NULL);
	g_data_output_stream_put_string(log->stream, "\n", NULL, NULL);

	g_output_stream_flush(G_OUTPUT_STREAM(log->stream), NULL, NULL);
}
