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

#ifndef _SERVER_H_
#define _SERVER_H_

struct maki_server;

typedef struct maki_server makiServer;

#include "cache.h"
#include "instance.h"
#include "sashimi.h"
#include "user.h"

struct maki_server
{
	makiInstance* instance;
	gchar* server;
	gchar* initial_nick;
	gchar* name;
	gboolean autoconnect;
	gboolean connected;
	sashimiConnection* connection;
	GHashTable* channels;
	makiCache* users;
	GHashTable* logs;

	makiUser* user;

	struct
	{
		gboolean ghost;
		gchar* password;
	}
	nickserv;

	gchar** commands;
	gchar** ignores;

	struct
	{
		gint retries;
		guint source;
	}
	reconnect;

	struct
	{
		gchar* chanmodes;
		gchar* chantypes;

		struct
		{
			gchar* modes;
			gchar* prefixes;
		}
		prefix;
	}
	support;
};

makiServer* maki_server_new (makiInstance*, const gchar*);
makiChannel* maki_server_add_channel (makiServer*, const gchar*, makiChannel*);
makiChannel* maki_server_get_channel (makiServer*, const gchar*);
void maki_server_remove_channel (makiServer*, const gchar*);
guint maki_server_channels_count (makiServer*);
void maki_server_channels_iter (makiServer*, GHashTableIter*);
void maki_server_free (gpointer);
gboolean maki_server_connect (makiServer*);
gboolean maki_server_disconnect (makiServer*, const gchar*);
void maki_server_reconnect_callback (gpointer);

#endif
