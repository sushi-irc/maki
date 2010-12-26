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

#ifndef H_DBUS
#define H_DBUS

#define SUSHI_DBUS_SERVICE "de.ikkoku.sushi"
#define SUSHI_DBUS_PATH "/de/ikkoku/sushi"
#define SUSHI_DBUS_INTERFACE "de.ikkoku.sushi"

struct maki_dbus;

typedef struct maki_dbus makiDBus;

#include <glib.h>

makiDBus* maki_dbus_new (void);
void maki_dbus_free (makiDBus*);

void maki_dbus_emit (makiDBus*, const gchar*, const gchar*, va_list);

void maki_dbus_emit_action (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_away (gint64, const gchar*);
void maki_dbus_emit_away_message (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_back (gint64, const gchar*);
void maki_dbus_emit_banlist (gint64, const gchar*, const gchar*, const gchar*, const gchar*, gint64);
void maki_dbus_emit_cannot_join (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_connect (gint64, const gchar*);
void maki_dbus_emit_connected (gint64, const gchar*);
void maki_dbus_emit_ctcp (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_dcc_send (gint64, guint64, const gchar*, const gchar*, const gchar*, guint64, guint64, guint64, guint64);
void maki_dbus_emit_error (gint64, const gchar*, const gchar*, const gchar*, gchar**);
void maki_dbus_emit_invite (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_join (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_kick (gint64, const gchar*, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_list (gint64, const gchar*, const gchar*, gint64, const gchar*);
void maki_dbus_emit_message (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_mode (gint64, const gchar*, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_motd (gint64, const gchar*, const gchar*);
void maki_dbus_emit_names (gint64, const gchar*, const gchar*, gchar**, gchar**);
void maki_dbus_emit_nick (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_no_such (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_notice (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_oper (gint64, const gchar*);
void maki_dbus_emit_part (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_quit (gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_shutdown (gint64);
void maki_dbus_emit_topic (gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_user_away (gint64, const gchar*, const gchar*, gboolean);
void maki_dbus_emit_whois (gint64, const gchar*, const gchar*, const gchar*);

gboolean maki_dbus_action (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_away (const gchar*, const gchar*, GError**);
gboolean maki_dbus_back (const gchar*, GError**);
gboolean maki_dbus_channel_nicks (const gchar*, const gchar*, gchar***, gchar***, GError**);
gboolean maki_dbus_channel_topic (const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_channels (const gchar*, gchar***, GError**);
gboolean maki_dbus_config_get (const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_config_set (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_connect (const gchar*, GError**);
gboolean maki_dbus_ctcp (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_dcc_send (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_dcc_sends (GArray**, gchar***, gchar***, gchar***, GArray**, GArray**, GArray**, GArray**, GError**);
gboolean maki_dbus_dcc_send_accept (guint64, GError**);
gboolean maki_dbus_dcc_send_get (guint64, const gchar*, gchar**, GError**);
gboolean maki_dbus_dcc_send_remove (guint64, GError**);
gboolean maki_dbus_dcc_send_resume (guint64, GError**);
gboolean maki_dbus_dcc_send_set (guint64, const gchar*, const gchar*, GError**);
gboolean maki_dbus_ignore (const gchar*, const gchar*, GError**);
gboolean maki_dbus_ignores (const gchar*, gchar***, GError**);
gboolean maki_dbus_invite (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_join (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_kick (const gchar*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_list (const gchar*, const gchar*, GError**);
gboolean maki_dbus_log (const gchar*, const gchar*, guint64, gchar***, GError**);
gboolean maki_dbus_message (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_mode (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_names (const gchar*, const gchar*, GError**);
gboolean maki_dbus_nick (const gchar*, const gchar*, GError**);
gboolean maki_dbus_nickserv (const gchar*, GError**);
gboolean maki_dbus_notice (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_oper (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_part (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_quit (const gchar*, const gchar*, GError**);
gboolean maki_dbus_raw (const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_get (const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_server_get_list (const gchar*, const gchar*, const gchar*, gchar***, GError**);
gboolean maki_dbus_server_list (const gchar*, const gchar*, gchar***, GError**);
gboolean maki_dbus_server_remove (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_rename (const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_set (const gchar*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_set_list (const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_servers (gchar***, GError**);
gboolean maki_dbus_shutdown (const gchar*, GError**);
gboolean maki_dbus_support_chantypes (const gchar*, gchar**, GError**);
gboolean maki_dbus_support_prefix (const gchar*, gchar***, GError**);
gboolean maki_dbus_topic (const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_unignore (const gchar*, const gchar*, GError**);
gboolean maki_dbus_user_away (const gchar*, const gchar*, gboolean*, GError**);
gboolean maki_dbus_user_channel_mode (const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_user_channel_prefix (const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_user_from (const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_version (GArray**, GError**);
gboolean maki_dbus_who (const gchar*, const gchar*, gboolean, GError**);
gboolean maki_dbus_whois (const gchar*, const gchar*, GError**);

extern makiDBus* dbus;

#endif
