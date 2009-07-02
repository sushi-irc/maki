/*
 * Copyright (c) 2009 Michael Kuhn
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

#include <glib.h>

static gchar* sushi_remote_print_bus_address (void)
{
	gchar* bus_address;
	gchar* path;

	path = g_build_filename(g_get_user_config_dir(), "sushi", "bus_address", NULL);

	if (!g_file_get_contents(path, &bus_address, NULL, NULL))
	{
		bus_address = NULL;
	}

	g_free(path);

	return bus_address;
}

static gchar* sushi_remote_get_bus_address (const gchar* userhost)
{
	gchar** command;
	gchar* bus_address = NULL;
	gchar* path;
	gsize cur;

	if (strcmp(userhost, "localhost") != 0)
	{
		command = g_new(gchar*, 5);
		command[0] = g_strdup("ssh");
		command[1] = g_strdup(userhost);
		cur = 2;
	}
	else
	{
		command = g_new(gchar*, 3);
		cur = 0;
	}

	command[cur++] = g_strdup("sushi-remote");
	command[cur++] = g_strdup("--print-bus-address");
	command[cur] = NULL;

	if (!g_spawn_sync(NULL, command, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, &bus_address, NULL, NULL, NULL))
	{
		bus_address = NULL;
	}

	if (bus_address != NULL)
	{
		g_strstrip(bus_address);

		if (strlen(bus_address) == 0)
		{
			g_free(bus_address);
			bus_address = NULL;
		}
	}

	g_strfreev(command);

	return bus_address;
}

static gboolean sushi_remote_setup_forwarding (const gchar* userhost, guint64 port)
{
	gboolean ret;
	gchar** command;

	if (strcmp(userhost, "localhost") == 0)
	{
		return TRUE;
	}

	/* FIXME check for existing forward */

	command = g_new(gchar*, 7);
	command[0] = g_strdup("ssh");
	command[1] = g_strdup("-N");
	command[2] = g_strdup("-f");
	command[3] = g_strdup("-L");
	command[4] = g_strdup_printf("localhost:%" G_GUINT64_FORMAT ":localhost:%" G_GUINT64_FORMAT, port, port);
	command[5] = g_strdup(userhost);
	command[6] = NULL;

	ret = g_spawn_sync(NULL, command, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	g_strfreev(command);

	return ret;
}

static gboolean sushi_remote_execute_command (gint argc, gchar** argv)
{
	GPid pid;
	gboolean ret;
	gchar** command;
	gsize cur;

	command = g_new(gchar*, argc - 1);

	for (cur = 2; cur < argc; cur++)
	{
		command[cur - 2] = g_strdup(argv[cur]);
	}

	command[argc - 2] = NULL;

	ret = g_spawn_async(NULL, command, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
	waitpid(pid, NULL, 0);
	g_spawn_close_pid(pid);

	g_strfreev(command);

	return ret;
}

int main (int argc, char* argv[])
{
	GError* error = NULL;

	gboolean opt_print_bus_address = FALSE;
	GOptionContext* context;
	GOptionEntry entries[] =
	{
		{ "print-bus-address", 0, 0, G_OPTION_ARG_NONE, &opt_print_bus_address, NULL, NULL },
		{ NULL }
	};

	/*
	setlocale(LC_ALL, "");
	bindtextdomain("maki", LOCALEDIR);
	textdomain("maki");
	*/

	context = g_option_context_new("[user@]host command ...");
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

	if (opt_print_bus_address)
	{
		gint ret;
		gchar* bus_address;

		bus_address = sushi_remote_print_bus_address();

		if (bus_address != NULL)
		{
			g_print("%s", bus_address);
			ret = 0;
		}
		else
		{
			g_printerr("Bus address could not be read.\n");
			ret = 1;
		}

		g_free(bus_address);

		return ret;
	}
	else if (argc < 3)
	{
		return 1;
	}
	else
	{
		const gchar* userhost = argv[1];
		gchar* bus_address;
		gchar** bus_parts;
		gchar** p;
		guint64 port = 0;

		if ((bus_address = sushi_remote_get_bus_address(userhost)) == NULL)
		{
			g_printerr("Bus address could not be determined.\n");

			return 1;
		}

		if (!g_str_has_prefix(bus_address, "tcp:"))
		{
			g_printerr("Bus address has wrong format.\n");

			g_free(bus_address);

			return 1;
		}

		bus_parts = g_strsplit(bus_address, ",", 0);

		for (p = bus_parts; *p != NULL; p++)
		{
			if (g_str_has_prefix(*p, "port="))
			{
				port = g_ascii_strtoull(*p + 5, NULL, 10);
				break;
			}
		}

		if (port == 0)
		{
			g_printerr("Port could not be determined.\n");

			g_strfreev(bus_parts);
			g_free(bus_address);

			return 1;
		}

		sushi_remote_setup_forwarding(userhost, port);

		if (!g_setenv("SUSHI_REMOTE_BUS_ADDRESS", bus_address, TRUE))
		{
			g_printerr("Environment variable could not be set.\n");

			g_strfreev(bus_parts);
			g_free(bus_address);

			return 1;
		}

		sushi_remote_execute_command(argc, argv);

		g_strfreev(bus_parts);
		g_free(bus_address);
	}

	return 0;
}
