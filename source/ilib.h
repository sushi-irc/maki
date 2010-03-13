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

#ifndef H_ILIB
#define H_ILIB

#include <glib.h>

struct i_lock;

typedef struct i_lock iLock;
typedef gchar* (*IStrvNewFunc) (const gchar*);

gboolean i_daemon (gboolean, gboolean);

guint i_io_add_watch (GIOChannel*, GIOCondition, GIOFunc, gpointer, GMainContext*);
guint i_idle_add (GSourceFunc, gpointer, GMainContext*);
guint i_timeout_add_seconds (guint, GSourceFunc, gpointer, GMainContext*);
gboolean i_source_remove (guint, GMainContext*);

GIOChannel* i_io_channel_unix_new_address (const gchar*, guint, gboolean);
GIOChannel* i_io_channel_unix_new_listen (const gchar*, guint, gboolean);
GIOStatus i_io_channel_write_chars (GIOChannel*, const gchar*, gssize, gsize*, GError**);
GIOStatus i_io_channel_read_chars (GIOChannel*, gchar*, gsize, gsize*, GError**);
GIOStatus i_io_channel_flush (GIOChannel*, GError**);

gboolean i_key_file_to_file (GKeyFile*, const gchar*, gsize*, GError**);

gchar* i_strreplace (const gchar*, const gchar*, const gchar*, guint);

gboolean i_ascii_str_case_equal (gconstpointer, gconstpointer);
guint i_ascii_str_case_hash (gconstpointer);

gchar* i_get_current_time_string (const gchar*);

gchar** i_strv_new (IStrvNewFunc, ...) G_GNUC_NULL_TERMINATED;

iLock* i_lock_new (const gchar*);
gboolean i_lock_lock (iLock*, const gchar*);
gboolean i_lock_unlock (iLock*);
void i_lock_free (iLock*);

#endif
