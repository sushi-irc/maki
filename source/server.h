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

#ifndef H_SERVER
#define H_SERVER

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

struct maki_server;

typedef struct maki_server makiServer;

enum makiServerSupport
{
	MAKI_SERVER_SUPPORT_CHANMODES,
	MAKI_SERVER_SUPPORT_CHANTYPES,
	MAKI_SERVER_SUPPORT_PREFIX_MODES,
	MAKI_SERVER_SUPPORT_PREFIX_PREFIXES
};

typedef enum makiServerSupport makiServerSupport;

#include <glib.h>

#include "channel.h"
#include "log.h"
#include "sashimi.h"
#include "user.h"

makiServer* maki_server_new (gchar const*);
makiServer* maki_server_ref (makiServer*);
void maki_server_unref (gpointer);

gboolean maki_server_config_get_boolean (makiServer*, gchar const*, gchar const*);
void maki_server_config_set_boolean (makiServer*, gchar const*, gchar const*, gboolean);
gint maki_server_config_get_integer (makiServer*, gchar const*, gchar const*);
void maki_server_config_set_integer (makiServer*, gchar const*, gchar const*, gint);
gchar* maki_server_config_get_string (makiServer*, gchar const*, gchar const*);
void maki_server_config_set_string (makiServer*, gchar const*, gchar const*, gchar const*);
gchar** maki_server_config_get_string_list (makiServer*, gchar const*, gchar const*);
void maki_server_config_set_string_list (makiServer*, gchar const*, gchar const*, gchar**);
gboolean maki_server_config_remove_key (makiServer*, gchar const*, gchar const*);
gboolean maki_server_config_remove_group (makiServer*, gchar const*);
gchar** maki_server_config_get_keys (makiServer*, gchar const*);
gchar** maki_server_config_get_groups (makiServer*);
gboolean maki_server_config_exists (makiServer*, gchar const*, gchar const*);

gchar const* maki_server_name (makiServer*);
gboolean maki_server_autoconnect (makiServer*);
gboolean maki_server_connected (makiServer*);
gboolean maki_server_logged_in (makiServer*);
void maki_server_set_logged_in (makiServer*, gboolean);
makiUser* maki_server_user (makiServer*);

gchar const* maki_server_support (makiServer*, makiServerSupport);
void maki_server_set_support (makiServer*, makiServerSupport, gchar const*);

makiUser* maki_server_add_user (makiServer*, gchar const*);
makiUser* maki_server_get_user (makiServer*, gchar const*);
gboolean maki_server_remove_user (makiServer*, gchar const*);
gboolean maki_server_rename_user (makiServer*, gchar const*, gchar const*);

void maki_server_add_channel (makiServer*, gchar const*, makiChannel*);
makiChannel* maki_server_get_channel (makiServer*, gchar const*);
gboolean maki_server_remove_channel (makiServer*, gchar const*);
guint maki_server_channels_count (makiServer*);
void maki_server_channels_iter (makiServer*, GHashTableIter*);

void maki_server_log (makiServer*, const gchar*, const gchar*, ...) G_GNUC_PRINTF(3, 4);

gboolean maki_server_queue (makiServer*, gchar const*, gboolean);
gboolean maki_server_send (makiServer*, gchar const*);
gboolean maki_server_send_printf (makiServer*, gchar const*, ...) G_GNUC_PRINTF(2, 3);

gboolean maki_server_connect (makiServer*);
gboolean maki_server_disconnect (makiServer*, gchar const*);

#endif
