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

#include <string.h>
#include <unistd.h>

#include "maki.h"
#include "maki_irc.h"
#include "maki_misc.h"
#include "maki_servers.h"

void maki_shutdown (struct maki* maki)
{
	GTimeVal time;
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_hash_table_iter_init(&iter, maki->connections);

	while (g_hash_table_iter_next(&iter, &key, &value))
	{
		struct maki_connection* m_conn = value;

		m_conn->reconnect = FALSE;

		sashimi_send(m_conn->connection, "QUIT :" IRC_QUIT_MESSAGE);

		g_get_current_time(&time);
		maki_dbus_emit_quit(maki->bus, time.tv_sec, m_conn->server, m_conn->nick, IRC_QUIT_MESSAGE);
	}

	g_usleep(1000000);

	g_get_current_time(&time);
	maki_dbus_emit_shutdown(maki->bus, time.tv_sec);

	maki->threads.terminate = TRUE;
	g_thread_join(maki->threads.messages);
	g_async_queue_unref(maki->message_queue);

	g_hash_table_destroy(maki->connections);

	g_free(maki->directories.logs);
	g_free(maki->directories.servers);

	dbus_g_connection_unref(maki->bus->bus);
	g_object_unref(maki->bus);

	g_main_loop_quit(maki->loop);
	g_main_loop_unref(maki->loop);
}

int main (int argc, char* argv[])
{
	struct maki maki;

	if (!g_thread_supported())
	{
		g_thread_init(NULL);
	}

	dbus_g_thread_init();
	g_type_init();

	maki.bus = g_object_new(MAKI_DBUS_TYPE, NULL);
	maki.bus->maki = &maki;

	maki.config.reconnect.retries = 3;
	maki.config.reconnect.timeout = 5;

	maki.connections = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_connection_destroy);

	maki.directories.logs = g_build_filename(g_get_home_dir(), ".sushi", "logs", NULL);
	maki.directories.servers = g_build_filename(g_get_home_dir(), ".sushi", "servers", NULL);

	maki.message_queue = g_async_queue_new();

	maki.threads.messages = g_thread_create(maki_irc_parser, &maki, TRUE, NULL);
	maki.threads.terminate = FALSE;

	maki_servers(&maki);

	maki.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(maki.loop);

	/*
	g_mkdir_with_parents(logs_dir, 0755);
	*/

	return 0;
}
