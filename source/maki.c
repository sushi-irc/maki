/*
 * Copyright (c) 2008-2011 Michael Kuhn
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

#define _XOPEN_SOURCE

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ilib.h>

#include "maki.h"

#include "dbus.h"
#include "dbus_server.h"
#include "instance.h"

gboolean opt_verbose = FALSE;

GMainLoop* main_loop = NULL;

static void maki_signal (int signo)
{
	GHashTableIter iter;
	gpointer key, value;
	makiInstance* inst = maki_instance_get_default();

	maki_instance_servers_iter(inst, &iter);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		makiServer* serv = value;

		maki_server_disconnect(serv, "");
	}

	maki_dbus_emit_shutdown();

	if (g_main_loop_is_running(main_loop))
	{
		g_main_loop_quit(main_loop);
	}
}

int main (int argc, char* argv[])
{
	gchar* bus_address_file = NULL;
	const gchar* file;
	GDir* servers;
	iLock* lock = NULL;
	makiInstance* inst = NULL;
	struct sigaction sig;
	GError* error = NULL;

	gboolean opt_daemon = FALSE;
	gboolean opt_dbus_server = FALSE;
	gboolean opt_version = FALSE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "daemon", 'd', 0, G_OPTION_ARG_NONE, &opt_daemon, N_("Run as daemon"), NULL },
		{ "dbus-server", 0, 0, G_OPTION_ARG_NONE, &opt_dbus_server, N_("Start DBus server"), NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &opt_verbose, N_("Output debug messages"), NULL },
		{ "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, NULL, NULL },
		{ NULL }
	};

	if (!g_thread_get_initialized())
	{
		g_thread_init(NULL);
	}

	g_type_init();

	setlocale(LC_ALL, "");
	bindtextdomain("maki", LOCALEDIR);
	textdomain("maki");

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, "maki");

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

	if ((inst = maki_instance_get_default()) == NULL)
	{
		g_warning("%s\n", _("Could not create maki instance."));
		goto error;
	}

	if (!opt_dbus_server)
	{
		/* FIXME does not check whether we really got a connection */
		if ((dbus = maki_dbus_new()) == NULL)
		{
			goto error;
		}
	}
	else
	{
		if ((dbus_server = maki_dbus_server_new()) == NULL)
		{
			goto error;
		}

		bus_address_file = g_build_filename(maki_instance_directory(inst, "config"), "bus_address", NULL);

		if ((lock = i_lock_new(bus_address_file)) == NULL)
		{
			goto error;
		}

		if (!i_lock_lock(lock, maki_dbus_server_address(dbus_server)))
		{
			gchar* msg;

			msg = g_strdup_printf(_("“%s” is locked. maki may already be running."), bus_address_file);
			g_warning("%s\n", msg);
			g_free(msg);

			goto error;
		}
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

		/* FIXME this emits signals */
		/* FIXME UPnP plugin may not be ready here */
		if ((serv = maki_server_new(file)) != NULL)
		{
			maki_instance_add_server(inst, maki_server_name(serv), serv);
		}
	}

	g_dir_close(servers);

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	if (dbus != NULL)
	{
		maki_dbus_free(dbus);
		dbus = NULL;
	}

	if (bus_address_file != NULL)
	{
		g_free(bus_address_file);
	}

	if (dbus_server != NULL)
	{
		maki_dbus_server_free(dbus_server);
		dbus_server = NULL;
	}

	if (inst != NULL)
	{
		/* FIXME this may emit signals */
		maki_instance_free(inst);
	}

	if (lock != NULL)
	{
		i_lock_unlock(lock);
		i_lock_free(lock);
	}

	g_main_loop_unref(main_loop);

	return 0;

error:
	if (dbus != NULL)
	{
		maki_dbus_free(dbus);
	}

	if (bus_address_file != NULL)
	{
		g_free(bus_address_file);
	}

	if (dbus_server != NULL)
	{
		maki_dbus_server_free(dbus_server);
	}

	if (inst != NULL)
	{
		maki_instance_free(inst);
	}

	if (lock != NULL)
	{
		i_lock_unlock(lock);
		i_lock_free(lock);
	}

	return 1;
}
