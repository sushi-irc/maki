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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "maki.h"

struct maki* maki_new (void)
{
	const gchar* home_dir;
	gchar* path;
	GKeyFile* key_file;
	struct maki* maki;

	maki = g_new(struct maki, 1);

	if ((home_dir = g_getenv("HOME")) == NULL)
	{
		home_dir = g_get_home_dir();
	}

	maki->bus = g_object_new(MAKI_DBUS_TYPE, NULL);
	maki->bus->maki = maki;

	maki->config.reconnect.retries = 3;
	maki->config.reconnect.timeout = 5;

	maki->connections = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_connection_free);

	maki->directories.sushi = g_build_filename(home_dir, ".sushi", NULL);
	maki->directories.config = g_build_filename(maki->directories.sushi, "config", NULL);
	maki->directories.logs = g_build_filename(maki->directories.sushi, "logs", NULL);
	maki->directories.servers = g_build_filename(maki->directories.sushi, "servers", NULL);

	path = g_build_filename(maki->directories.config, "maki", NULL);
	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		gint retries;
		gint timeout;
		GError* error = NULL;

		retries = g_key_file_get_integer(key_file, "reconnect", "retries", &error);

		if (error)
		{
			g_error_free(error);
			error = NULL;
		}
		else
		{
			maki->config.reconnect.retries = retries;
		}

		timeout = g_key_file_get_integer(key_file, "reconnect", "timeout", &error);

		if (error)
		{
			g_error_free(error);
			error = NULL;
		}
		else if (timeout >= 0)
		{
			maki->config.reconnect.timeout = timeout;
		}
	}

	g_key_file_free(key_file);
	g_free(path);

	maki->message_queue = g_async_queue_new();

	maki->threads.terminate = FALSE;
	maki->threads.messages = g_thread_create(maki_irc_parser, maki, TRUE, NULL);

	maki->loop = g_main_loop_new(NULL, FALSE);

	return maki;
}

void maki_free (struct maki* maki)
{
	maki->threads.terminate = TRUE;
	g_thread_join(maki->threads.messages);
	g_async_queue_unref(maki->message_queue);

	g_hash_table_destroy(maki->connections);

	g_free(maki->directories.config);
	g_free(maki->directories.logs);
	g_free(maki->directories.servers);
	g_free(maki->directories.sushi);

	dbus_g_connection_unref(maki->bus->bus);
	g_object_unref(maki->bus);

	g_main_loop_quit(maki->loop);
	g_main_loop_unref(maki->loop);

	g_free(maki);
}

int maki_daemonize (void)
{
	int fd;
	pid_t pid;

	pid = fork();

	if (pid > 0)
	{
		exit(0);
	}
	else if (pid == -1)
	{
		return 1;
	}

	if (setsid() == -1)
	{
		return 1;
	}

	fd = open("/dev/null", O_RDWR);

	if (fd == -1)
	{
		return 1;
	}

	if (dup2(fd, 0) == -1 || dup2(fd, 1) == -1 || dup2(fd, 2) == -1)
	{
		return 1;
	}

	if (fd > 2)
	{
		close(fd);
	}

	return 0;
}

int main (int argc, char* argv[])
{
	struct maki* maki;

	gboolean opt_daemon = FALSE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon, "Run as daemon", NULL },
		{ NULL }
	};

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	if (opt_daemon)
	{
		if (maki_daemonize() != 0)
		{
			return 1;
		}
	}

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	maki = maki_new();

	maki_servers(maki);

	g_main_loop_run(maki->loop);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	*/

	return 0;
}
