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
	static makiInstance* inst = NULL;

	if (G_UNLIKELY(inst == NULL))
	{
		inst = maki_instance_new();
	}

	return inst;
}

makiInstance* maki_instance_new (void)
{
	makiInstance* inst;

	if ((inst = g_new(makiInstance, 1)) == NULL)
	{
		return NULL;
	}

	inst->bus = g_object_new(MAKI_DBUS_TYPE, NULL);

	inst->servers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, maki_server_free);

	inst->directories.config = g_build_filename(g_get_user_config_dir(), "sushi", NULL);
	inst->directories.servers = g_build_filename(g_get_user_config_dir(), "sushi", "servers", NULL);

	inst->config = maki_config_new(inst);

	inst->message_queue = g_async_queue_new_full(sashimi_message_free);

	inst->opt.debug = FALSE;

	inst->threads.messages = g_thread_create(maki_in_runner, inst, TRUE, NULL);

	inst->loop = g_main_loop_new(NULL, FALSE);

	return inst;
}

void maki_instance_free (makiInstance* inst)
{
	sashimiMessage* msg;

	/* Send a bogus message so the messages thread wakes up. */
	msg = sashimi_message_new(NULL, NULL);

	g_async_queue_push(inst->message_queue, msg);

	g_thread_join(inst->threads.messages);
	g_async_queue_unref(inst->message_queue);

	g_hash_table_destroy(inst->servers);

	g_free(inst->directories.config);
	g_free(inst->directories.servers);

	g_object_unref(inst->bus);

	maki_config_free(inst->config);

	g_main_loop_quit(inst->loop);
	g_main_loop_unref(inst->loop);

	g_free(inst);
}
