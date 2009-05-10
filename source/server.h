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

#ifndef H_SERVER
#define H_SERVER

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

struct maki_server;

typedef struct maki_server makiServer;

#include "sashimi.h"
#include "user.h"

struct maki_server
{
	gchar* name;
	gboolean connected;
	gboolean logged_in;
	sashimiConnection* connection;
	GHashTable* channels;
	GHashTable* users;
	GHashTable* logs;

	makiUser* user;

	GKeyFile* key_file;

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

	GMainContext* main_context;
	GMainLoop* main_loop;

	struct
	{
		struct sockaddr addr;
		socklen_t addrlen;
	}
	stun;

	struct
	{
		guint64 id;
		GSList* list;
	}
	dcc;
};

makiServer* maki_server_new (const gchar*);
gboolean maki_server_config_get_boolean (makiServer*, const gchar*, const gchar*);
void maki_server_config_set_boolean (makiServer*, const gchar*, const gchar*, gboolean);
gint maki_server_config_get_integer (makiServer*, const gchar*, const gchar*);
void maki_server_config_set_integer (makiServer*, const gchar*, const gchar*, gint);
gchar* maki_server_config_get_string (makiServer*, const gchar*, const gchar*);
void maki_server_config_set_string (makiServer*, const gchar*, const gchar*, const gchar*);
gchar** maki_server_config_get_string_list (makiServer*, const gchar*, const gchar*);
void maki_server_config_set_string_list (makiServer*, const gchar*, const gchar*, gchar**);
gboolean maki_server_config_remove_key (makiServer*, const gchar*, const gchar*);
gboolean maki_server_config_remove_group (makiServer*, const gchar*);
gchar** maki_server_config_get_keys (makiServer*, const gchar*);
gchar** maki_server_config_get_groups (makiServer*);
gboolean maki_server_config_exists (makiServer*, const gchar*, const gchar*);
guint64 maki_server_dcc_get_id (makiServer*);
makiUser* maki_server_user (makiServer*);
void maki_server_add_user (makiServer*, const gchar*, makiUser*);
makiUser* maki_server_get_user (makiServer*, const gchar*);
gboolean maki_server_remove_user (makiServer*, const gchar*);
const gchar* maki_server_name (makiServer*);
gboolean maki_server_autoconnect (makiServer*);
makiChannel* maki_server_add_channel (makiServer*, const gchar*, makiChannel*);
makiChannel* maki_server_get_channel (makiServer*, const gchar*);
gboolean maki_server_remove_channel (makiServer*, const gchar*);
guint maki_server_channels_count (makiServer*);
void maki_server_channels_iter (makiServer*, GHashTableIter*);
gboolean maki_server_queue (makiServer*, const gchar*, gboolean);
gboolean maki_server_send (makiServer*, const gchar*);
gboolean maki_server_send_printf (makiServer*, const gchar*, ...) G_GNUC_PRINTF(2, 3);
void maki_server_free (gpointer);
gboolean maki_server_connect (makiServer*);
gboolean maki_server_disconnect (makiServer*, const gchar*);
void maki_server_connect_callback (gpointer);
void maki_server_reconnect_callback (gpointer);

#endif
