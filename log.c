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

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "maki.h"

struct maki_log
{
	int fd;
};

makiLog* maki_log_new (const gchar* server, const gchar* name)
{
	gchar* dirname;
	gchar* filename;
	gchar* path;
	makiLog* log;
	makiInstance* inst = maki_instance_get_default();

	log = g_new(makiLog, 1);

	dirname = g_build_filename(maki_config_get(inst->config, "directories", "logs"), server, NULL);
	filename = g_strconcat(name, ".txt", NULL);
	path = g_build_filename(dirname, filename, NULL);

	g_mkdir_with_parents(dirname, S_IRUSR | S_IWUSR | S_IXUSR);

	if ((log->fd = open(path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
	{
		g_free(log);

		log = NULL;
	}

	g_free(path);
	g_free(filename);
	g_free(dirname);

	return log;
}

void maki_log_free (gpointer data)
{
	makiLog* log = data;

	close(log->fd);

	g_free(log);
}

void maki_log_write (makiLog* log, const gchar* message)
{
	gchar buf[1024];
	time_t t;

	t = time(NULL);
	strftime(buf, 1024, "%Y-%m-%d %H:%M:%S", localtime(&t));

	maki_write(log->fd, buf);
	maki_write(log->fd, " ");
	maki_write(log->fd, message);
	maki_write(log->fd, "\n");
}
