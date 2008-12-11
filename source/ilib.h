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

#ifndef H_ILIB
#define H_ILIB

#include <glib.h>

gboolean i_daemon (gboolean, gboolean);

guint i_io_add_watch (GIOChannel*, GIOCondition, GIOFunc, gpointer, GMainContext*);
guint i_idle_add (GSourceFunc, gpointer, GMainContext*);
guint i_timeout_add_seconds (guint, GSourceFunc, gpointer, GMainContext*);
gboolean i_source_remove (guint, GMainContext*);

gboolean i_key_file_to_file (GKeyFile*, const gchar*, gsize*, GError**);

gboolean i_ascii_str_case_equal (gconstpointer, gconstpointer);
guint i_ascii_str_case_hash (gconstpointer);

gchar* i_get_current_time_string (void);


struct i_cache;
typedef struct i_cache iCache;

typedef gpointer (*iCacheNewFunc) (gpointer, gpointer);
typedef void (*iCacheFreeFunc) (gpointer);

iCache* i_cache_new (iCacheNewFunc, iCacheFreeFunc, gpointer, GHashFunc, GEqualFunc);
void i_cache_free (iCache*);
gpointer i_cache_insert (iCache*, const gchar*);
gpointer i_cache_lookup (iCache*, const gchar*);
void i_cache_remove (iCache*, const gchar*);

#endif
