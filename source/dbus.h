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

#ifndef H_DBUS
#define H_DBUS

#include <glib-object.h>

struct maki_dbus;
struct maki_dbus_class;

typedef struct maki_dbus makiDBus;
typedef struct maki_dbus_class makiDBusClass;

GType maki_dbus_get_type (void);

#define MAKI_DBUS_TYPE              (maki_dbus_get_type())
#define MAKI_DBUS(object)           (G_TYPE_CHECK_INSTANCE_CAST((object), MAKI_DBUS_TYPE, makiDBus))
#define MAKI_DBUS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), MAKI_DBUS_TYPE, makiDBusClass))
#define IS_MAKI_DBUS(object)        (G_TYPE_CHECK_INSTANCE_TYPE((object), MAKI_DBUS_TYPE))
#define IS_MAKI_DBUS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), MAKI_DBUS_TYPE))
#define MAKI_DBUS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), MAKI_DBUS_TYPE, makiDBus))

gboolean maki_dbus_connected (makiDBus*);

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
void maki_dbus_emit_whois (gint64, const gchar*, const gchar*, const gchar*);

gboolean maki_dbus_action (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_away (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_back (makiDBus*, const gchar*, GError**);
gboolean maki_dbus_channel_nicks (makiDBus*, const gchar*, const gchar*, gchar***, gchar***, GError**);
gboolean maki_dbus_channel_topic (makiDBus*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_channels (makiDBus*, const gchar*, gchar***, GError**);
gboolean maki_dbus_config_get (makiDBus*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_config_set (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_connect (makiDBus*, const gchar*, GError**);
gboolean maki_dbus_ctcp (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_dcc_send (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_dcc_sends (makiDBus*, GArray**, gchar***, gchar***, gchar***, GArray**, GArray**, GArray**, GArray**, GError**);
gboolean maki_dbus_dcc_send_accept (makiDBus*, guint64, GError**);
gboolean maki_dbus_dcc_send_get (makiDBus*, guint64, const gchar*, gchar**, GError**);
gboolean maki_dbus_dcc_send_remove (makiDBus*, guint64, GError**);
gboolean maki_dbus_dcc_send_resume (makiDBus*, guint64, GError**);
gboolean maki_dbus_dcc_send_set (makiDBus*, guint64, const gchar*, const gchar*, GError**);
gboolean maki_dbus_ignore (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_ignores (makiDBus*, const gchar*, gchar***, GError**);
gboolean maki_dbus_invite (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_join (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_kick (makiDBus*, const gchar*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_list (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_log (makiDBus*, const gchar*, const gchar*, guint64, gchar***, GError**);
gboolean maki_dbus_message (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_mode (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_names (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_nick (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_nickserv (makiDBus*, const gchar*, GError**);
gboolean maki_dbus_notice (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_oper (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_part (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_quit (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_raw (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_get (makiDBus*, const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_server_get_list (makiDBus*, const gchar*, const gchar*, const gchar*, gchar***, GError**);
gboolean maki_dbus_server_list (makiDBus*, const gchar*, const gchar*, gchar***, GError**);
gboolean maki_dbus_server_remove (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_rename (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_set (makiDBus*, const gchar*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_server_set_list (makiDBus*, const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_servers (makiDBus*, gchar***, GError**);
gboolean maki_dbus_shutdown (makiDBus*, const gchar*, GError**);
gboolean maki_dbus_support_chantypes (makiDBus*, const gchar*, gchar**, GError**);
gboolean maki_dbus_support_prefix (makiDBus*, const gchar*, gchar***, GError**);
gboolean maki_dbus_topic (makiDBus*, const gchar*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_unignore (makiDBus*, const gchar*, const gchar*, GError**);
gboolean maki_dbus_user_away (makiDBus*, const gchar*, const gchar*, gboolean*, GError**);
gboolean maki_dbus_user_channel_mode (makiDBus*, const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_user_channel_prefix (makiDBus*, const gchar*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_user_from (makiDBus*, const gchar*, const gchar*, gchar**, GError**);
gboolean maki_dbus_version (makiDBus*, GArray**, GError**);
gboolean maki_dbus_who (makiDBus*, const gchar*, const gchar*, gboolean, GError**);
gboolean maki_dbus_whois (makiDBus*, const gchar*, const gchar*, GError**);

#endif
