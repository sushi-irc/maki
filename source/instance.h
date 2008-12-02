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

#ifndef H_INSTANCE
#define H_INSTANCE

struct maki_instance;

typedef struct maki_instance makiInstance;

makiInstance* maki_instance_get_default (void);
makiInstance* maki_instance_new (void);
gboolean maki_instance_config_get_boolean (makiInstance*, const gchar*, const gchar*);
void maki_instance_config_set_boolean (makiInstance*, const gchar*, const gchar*, gboolean);
gint maki_instance_config_get_integer (makiInstance*, const gchar*, const gchar*);
void maki_instance_config_set_integer (makiInstance*, const gchar*, const gchar*, gint);
gchar* maki_instance_config_get_string (makiInstance*, const gchar*, const gchar*);
void maki_instance_config_set_string (makiInstance*, const gchar*, const gchar*, const gchar*);
const gchar* maki_instance_directory (makiInstance*, const gchar*);
GHashTable* maki_instance_servers (makiInstance*);
void maki_instance_free (makiInstance*);

#endif
