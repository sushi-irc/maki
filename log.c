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

struct maki_log* maki_log_new (const gchar* directory, const gchar* server, const gchar* name)
{
	gchar* dirname;
	gchar* filename;
	gchar* path;
	struct maki_log* log;

	log = g_new(struct maki_log, 1);
	log->name = g_strdup(name);

	dirname = g_build_filename(directory, server, NULL);
	filename = g_strconcat(log->name, ".txt", NULL);
	path = g_build_filename(dirname, filename, NULL);

	g_mkdir_with_parents(dirname, S_IRUSR | S_IWUSR | S_IXUSR);

	if ((log->fd = open(path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
	{
		g_free(log->name);
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
	struct maki_log* log = data;

	close(log->fd);

	g_free(log->name);
	g_free(log);
}

void maki_log (struct maki_server* conn, const gchar* name, const gchar* format, ...)
{
	gchar buf[1024];
	gchar* tmp;
	time_t t;
	struct maki_log* log;
	va_list args;
	struct maki* m = maki();

	if (!m->config->logging.enabled)
	{
		return;
	}

	t = time(NULL);
	strftime(buf, 1024, m->config->logging.time_format, localtime(&t));

	if ((log = g_hash_table_lookup(conn->logs, name)) == NULL)
	{
		if ((log = maki_log_new(m->config->directories.logs, conn->server, name)) == NULL)
		{
			return;
		}

		g_hash_table_replace(conn->logs, log->name, log);
	}

	va_start(args, format);
	tmp = g_strdup_vprintf(format, args);
	va_end(args);

	maki_write(log->fd, buf);
	maki_write(log->fd, " ");
	maki_write(log->fd, tmp);
	maki_write(log->fd, "\n");

	g_free(tmp);
}
