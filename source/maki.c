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
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

gboolean opt_verbose = FALSE;

makiDBus* dbus = NULL;
GMainLoop* main_loop = NULL;

static void maki_signal (int signo)
{
	GTimeVal timeval;
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, "");
	}

	g_get_current_time(&timeval);
	maki_dbus_emit(s_shutdown, timeval.tv_sec);

	g_main_loop_quit(main_loop);
}

int main (int argc, char* argv[])
{
	gchar* bus_address;
	gchar* bus_address_file;
	const gchar* file;
	GDir* servers;
	makiInstance* inst = NULL;
	struct sigaction sig;
	GError* error = NULL;

	gboolean opt_daemon = FALSE;
	gboolean opt_version = FALSE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon, N_("Run as daemon"), NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &opt_verbose, N_("Output debug messages"), NULL },
		{ "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, NULL, NULL },
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

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		g_option_context_free(context);

		if (error)
		{
			g_printerr("%s\n", error->message);
			g_error_free(error);
		}

		return 1;
	}

	g_option_context_free(context);

	if (opt_version)
	{
		g_print("maki %s\n", SUSHI_VERSION);
		return 0;
	}

	if (opt_daemon
	    && !i_daemon(FALSE, FALSE))
	{
		g_warning("%s\n", _("Could not switch to daemon mode."));
		goto error;
	}

	umask(0077);

	dbus = g_object_new(MAKI_DBUS_TYPE, NULL);

	if (!maki_dbus_connected(dbus))
	{
		/* Use g_warning() here to not overwrite the debug log of the already running maki. */
		g_warning("%s\n", _("Could not connect to DBus. maki may already be running."));
		goto error;
	}

	if ((inst = maki_instance_get_default()) == NULL)
	{
		g_warning("%s\n", _("Could not create maki instance."));
		goto error;
	}

	sig.sa_handler = maki_signal;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;

	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGHUP, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);
	sigaction(SIGQUIT, &sig, NULL);

	sig.sa_handler = SIG_IGN;

	sigaction(SIGPIPE, &sig, NULL);

	servers = g_dir_open(maki_instance_directory(inst, "servers"), 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		makiServer* serv;

		if ((serv = maki_server_new(file)) != NULL)
		{
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}

	}

	g_dir_close(servers);

	bus_address_file = g_build_filename(maki_instance_directory(inst, "config"), "bus_address", NULL);
	bus_address = g_strdup(g_getenv("DBUS_SESSION_BUS_ADDRESS"));

	if (bus_address != NULL)
	{
		g_file_set_contents(bus_address_file, bus_address, -1, NULL);
	}

	g_free(bus_address);

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);

	g_unlink(bus_address_file);
	g_free(bus_address_file);

	maki_instance_free(inst);
	g_object_unref(dbus);

	return 0;

error:
	if (inst != NULL)
	{
		maki_instance_free(inst);
	}

	if (dbus != NULL)
	{
		g_object_unref(dbus);
	}

	return 1;
}
