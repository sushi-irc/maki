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
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

gboolean opt_debug = TRUE;

makiDBus* dbus = NULL;
GMainLoop* main_loop = NULL;

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

	if (dup2(fd, STDIN_FILENO) == -1 || dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1)
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
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	g_hash_table_iter_init(&iter, maki_instance_servers(inst));

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, "");
	}

	g_get_current_time(&time);
	maki_dbus_emit_shutdown(time.tv_sec);

	maki_instance_free(inst);

	g_main_loop_quit(main_loop);

	signal(signo, SIG_DFL);
	raise(signo);
}

int main (int argc, char* argv[])
{
	const gchar* file;
	GDir* servers;
	makiInstance* inst;

	gboolean opt_daemon = FALSE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon, N_("Run as daemon"), NULL },
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug, N_("Output debug messages"), NULL },
		{ NULL }
	};

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	setlocale(LC_ALL, "");
	bindtextdomain("maki", LOCALEDIR);
	textdomain("maki");

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	if ((inst = maki_instance_get_default()) == NULL)
	{
		/* FIXME error message */
		return 1;
	}

	dbus = g_object_new(MAKI_DBUS_TYPE, NULL);

	if (!maki_dbus_connected(dbus))
	{
		g_warning("%s\n", _("Could not connect to DBus. maki may already be running."));
		maki_instance_free(inst);
		g_object_unref(dbus);
		return 1;
	}

	if (opt_daemon
	    && maki_daemonize() != 0)
	{
		/* FIXME error message */
		maki_instance_free(inst);
		g_object_unref(dbus);
		return 1;
	}

	signal(SIGINT, maki_signal);
	signal(SIGHUP, maki_signal);
	signal(SIGTERM, maki_signal);
	signal(SIGQUIT, maki_signal);

	/* FIXME move to maki_instance_new()? */
	if (g_mkdir_with_parents(maki_instance_directory(inst, "config"), S_IRUSR | S_IWUSR | S_IXUSR) != 0
	    || g_mkdir_with_parents(maki_instance_directory(inst, "servers"), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	{
		/* FIXME error message */
		maki_instance_free(inst);
		g_object_unref(dbus);
		return 1;
	}

	servers = g_dir_open(maki_instance_directory(inst, "servers"), 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		makiServer* serv;

		if ((serv = maki_server_new(inst, file)) != NULL)
		{
			g_hash_table_replace(maki_instance_servers(inst), serv->server, serv);
		}

	}

	g_dir_close(servers);

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);

	g_object_unref(dbus);

	return 0;
}
