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

#ifndef H_DCC
#define H_DCC

struct maki_dcc_send;

typedef struct maki_dcc_send makiDCCSend;

#include <glib.h>

#include "server.h"
#include "user.h"

gchar* maki_dcc_send_get_file_name (const gchar*, gsize*);

makiDCCSend* maki_dcc_send_new_in (makiServer*, makiUser*, const gchar*, guint32, guint16, goffset, guint32);
makiDCCSend* maki_dcc_send_new_out (makiServer*, makiUser*, const gchar*);
void maki_dcc_send_free (makiDCCSend*);

gboolean maki_dcc_send_accept (makiDCCSend*);
gboolean maki_dcc_send_resume (makiDCCSend*);
gboolean maki_dcc_send_resume_accept (makiDCCSend*, const gchar*, guint16, goffset, guint32, gboolean);

guint64 maki_dcc_send_id (makiDCCSend*);
goffset maki_dcc_send_size (makiDCCSend*);
goffset maki_dcc_send_progress (makiDCCSend*);
guint64 maki_dcc_send_speed (makiDCCSend*);
guint maki_dcc_send_status (makiDCCSend*);
makiServer* maki_dcc_send_server (makiDCCSend*);
makiUser* maki_dcc_send_user (makiDCCSend*);
const gchar* maki_dcc_send_path (makiDCCSend*);
gboolean maki_dcc_send_set_path (makiDCCSend*, const gchar*);
gchar* maki_dcc_send_filename (makiDCCSend*);

void maki_dcc_send_emit (makiDCCSend*);

#endif
