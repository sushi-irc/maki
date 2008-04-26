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
	struct maki_log* m_log;

	m_log = g_new(struct maki_log, 1);
	m_log->name = g_strdup(name);

	dirname = g_build_filename(directory, server, NULL);
	filename = g_strconcat(m_log->name, ".txt", NULL);
	path = g_build_filename(dirname, filename, NULL);

	g_mkdir_with_parents(dirname, 0755);

	if ((m_log->fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644)) == -1)
	{
		g_free(m_log->name);
		g_free(m_log);

		m_log = NULL;
	}

	g_free(path);
	g_free(filename);
	g_free(dirname);

	return m_log;
}

void maki_log_free (gpointer data)
{
	struct maki_log* m_log = data;

	close(m_log->fd);

	g_free(m_log->name);
	g_free(m_log);
}

void maki_log (struct maki_connection* m_conn, const gchar* name, const gchar* format, ...)
{
	gchar buf[1024];
	gchar* tmp;
	time_t t;
	struct maki_log* m_log;
	va_list args;

	if (!m_conn->maki->config.general.logging)
	{
		return;
	}

	t = time(NULL);
	strftime(buf, 1024, "[%Y-%m-%d %H:%M:%S] ", localtime(&t));

	if ((m_log = g_hash_table_lookup(m_conn->logs, name)) == NULL)
	{
		if ((m_log = maki_log_new(m_conn->maki->directories.logs, m_conn->server, name)) == NULL)
		{
			return;
		}

		g_hash_table_replace(m_conn->logs, m_log->name, m_log);
	}

	va_start(args, format);
	tmp = g_strdup_vprintf(format, args);
	va_end(args);

	write(m_log->fd, buf, strlen(buf));
	write(m_log->fd, tmp, strlen(tmp));
	write(m_log->fd, "\n", 1);

	g_free(tmp);
}
