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

#include <glib.h>

#include <sashimi.h>

#include "maki_dbus.h"

struct maki
{
	makiDBus* bus;

	struct
	{
		struct
		{
			gint retries;
			guint timeout;
		}
		reconnect;
	}
	config;

	GHashTable* connections;

	struct
	{
		gchar* logs;
		gchar* servers;
	}
	directories;

	GMainLoop* loop;

	GAsyncQueue* message_queue;

	struct
	{
		GThread* messages;

		gboolean terminate;
	}
	threads;
};

struct maki_connection
{
	struct maki* maki;
	gchar* server;
	gchar* initial_nick;
	gchar* nick;
	gchar* name;
	gboolean connected;
	gboolean reconnect;
	gint retries;
	struct sashimi_connection* connection;
	GHashTable* channels;
};

struct maki_channel
{
	gchar* name;
	gboolean joined;
	gchar* key;
	GHashTable* users;
};

struct maki_user
{
	gchar* nick;
};

void maki_shutdown (struct maki*);
