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
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "maki.h"

static int maki_daemonize (void)
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

static void maki_signal (int signo)
{
	GTimeVal time;
	GList* list;
	GList* tmp;
	makiInstance* inst = maki_instance_get_default();

	list = g_hash_table_get_values(inst->servers);

	for (tmp = list; tmp != NULL; tmp = g_list_next(tmp))
	{
		makiServer* serv = tmp->data;

		maki_server_disconnect(serv, SUSHI_QUIT_MESSAGE);
	}

	g_list_free(list);

	g_get_current_time(&time);
	maki_dbus_emit_shutdown(time.tv_sec);

	maki_instance_free(inst);

	signal(signo, SIG_DFL);
	raise(signo);
}

int main (int argc, char* argv[])
{
	const gchar* file;
	GDir* servers;
	makiInstance* inst;

	gboolean opt_daemon = FALSE;
	gboolean opt_debug = TRUE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon, N_("Run as daemon"), NULL },
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug, N_("Output debug messages"), NULL },
		{ NULL }
	};

	setlocale(LC_ALL, "");
	bindtextdomain(MAKI_NAME, LOCALEDIR);
	textdomain(MAKI_NAME);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	if (opt_daemon
	    && maki_daemonize() != 0)
	{
		return 1;
	}

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	if ((inst = maki_instance_get_default()) == NULL)
	{
		return 1;
	}

	inst->opt.debug = opt_debug;

	signal(SIGINT, maki_signal);
	signal(SIGHUP, maki_signal);
	signal(SIGTERM, maki_signal);
	signal(SIGQUIT, maki_signal);

	if (g_mkdir_with_parents(inst->directories.config, S_IRUSR | S_IWUSR | S_IXUSR) != 0
	    || g_mkdir_with_parents(inst->directories.servers, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		return 1;
	}

	servers = g_dir_open(inst->directories.servers, 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		makiServer* serv;

		if ((serv = maki_server_new(file)) != NULL)
		{
			g_hash_table_replace(inst->servers, serv->server, serv);
		}

	}

	g_dir_close(servers);

	g_main_loop_run(inst->loop);

	return 0;
}
