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

#ifndef H_CHANNEL
#define H_CHANNEL

struct maki_channel;

typedef struct maki_channel makiChannel;

#include <glib.h>

#include "server.h"
#include "user.h"

makiChannel* maki_channel_new (makiServer*, const gchar*);
void maki_channel_free (gpointer);

gboolean maki_channel_autojoin (makiChannel*);
void maki_channel_set_autojoin (makiChannel*, gboolean);

gboolean maki_channel_joined (makiChannel*);
void maki_channel_set_joined (makiChannel*, gboolean);

gchar* maki_channel_key (makiChannel*);
void maki_channel_set_key (makiChannel*, const gchar*);

const gchar* maki_channel_topic (makiChannel*);
void maki_channel_set_topic (makiChannel*, const gchar*);

makiUser* maki_channel_add_user (makiChannel*, const gchar*);
makiUser* maki_channel_get_user (makiChannel*, const gchar*);
makiUser* maki_channel_rename_user (makiChannel*, const gchar*, const gchar*);
void maki_channel_remove_user (makiChannel*, const gchar*);
void maki_channel_remove_users (makiChannel*);
guint maki_channel_users_count (makiChannel*);
void maki_channel_users_iter (makiChannel*, GHashTableIter*);

gboolean maki_channel_get_user_prefix (makiChannel*, makiUser*, guint);
void maki_channel_set_user_prefix (makiChannel*, makiUser*, guint, gboolean);
void maki_channel_set_user_prefix_override (makiChannel*, makiUser*, guint);

#endif
