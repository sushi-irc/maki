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

makiInstance* maki_instance_get_default (void)
{
	static makiInstance* m = NULL;

	if (G_UNLIKELY(m == NULL))
	{
		m = maki_instance_new();
	}

	return m;
}

makiInstance* maki_instance_new (void)
{
	makiInstance* m;

	if ((m = g_new(makiInstance, 1)) == NULL)
	{
		return NULL;
	}

	m->bus = g_object_new(MAKI_DBUS_TYPE, NULL);

	m->servers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_server_free);

	m->directories.config = g_build_filename(g_get_user_config_dir(), "sushi", NULL);
	m->directories.servers = g_build_filename(g_get_user_config_dir(), "sushi", "servers", NULL);

	m->config = maki_config_new(m);

	m->message_queue = g_async_queue_new_full(sashimi_message_free);

	m->opt.debug = FALSE;

	m->threads.messages = g_thread_create(maki_in_runner, m, TRUE, NULL);

	m->loop = g_main_loop_new(NULL, FALSE);

	return m;
}

void maki_instance_free (makiInstance* m)
{
	sashimiMessage* msg;

	/* Send a bogus message so the messages thread wakes up. */
	msg = sashimi_message_new(NULL, NULL);

	g_async_queue_push(m->message_queue, msg);

	g_thread_join(m->threads.messages);
	g_async_queue_unref(m->message_queue);

	g_hash_table_destroy(m->servers);

	g_free(m->directories.config);
	g_free(m->directories.servers);

	g_object_unref(m->bus);

	maki_config_free(m->config);

	g_main_loop_quit(m->loop);
	g_main_loop_unref(m->loop);

	g_free(m);
}
