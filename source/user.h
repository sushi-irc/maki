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

#ifndef H_USER
#define H_USER

struct maki_user;

typedef struct maki_user makiUser;

#include <glib.h>

#include "server.h"

makiUser* maki_user_new (gchar const*);
makiUser* maki_user_ref (makiUser*);
void maki_user_unref (gpointer);

guint maki_user_ref_count (makiUser*);

gchar const* maki_user_from (makiUser*);
gchar const* maki_user_nick (makiUser*);
void maki_user_set_nick (makiUser*, gchar const*);
void maki_user_set_user (makiUser*, gchar const*);
void maki_user_set_host (makiUser*, gchar const*);
gboolean maki_user_away (makiUser*);
void maki_user_set_away (makiUser*, gboolean);
gchar const* maki_user_away_message (makiUser*);
void maki_user_set_away_message (makiUser*, gchar const*);

#endif
