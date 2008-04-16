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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

typedef struct
{
	GObject parent;
	DBusGConnection* bus;
	struct maki* maki;
}
makiDBus;

typedef struct
{
	GObjectClass parent;
}
makiDBusClass;

GType maki_dbus_get_type (void);

#define MAKI_DBUS_TYPE            (maki_dbus_get_type())
#define MAKI_DBUS(object)         (G_TYPE_CHECK_INSTANCE_CAST((object), MAKI_DBUS_TYPE, makiDBus))
#define MAKI_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MAKI_DBUS_TYPE, makiDBusClass))
#define IS_MAKI_DBUS(object)      (G_TYPE_CHECK_INSTANCE_TYPE((object), MAKI_DBUS_TYPE))
#define IS_MAKI_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MAKI_DBUS_TYPE))
#define MAKI_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MAKI_DBUS_TYPE, makiDBus))

void maki_dbus_emit_action (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_away (makiDBus*, gint64, const gchar*);
void maki_dbus_emit_away_message (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_back (makiDBus*, gint64, const gchar*);
void maki_dbus_emit_connect (makiDBus*, gint64, const gchar*);
void maki_dbus_emit_connected (makiDBus*, gint64, const gchar*, const gchar*);
void maki_dbus_emit_ctcp (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_join (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_kick (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_message (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_mode (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_motd (makiDBus*, gint64, const gchar*, const gchar*);
void maki_dbus_emit_nick (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_notice (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_own_query (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_part (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_query (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_quit (makiDBus*, gint64, const gchar*, const gchar*, const gchar*);
void maki_dbus_emit_reconnect (makiDBus*, gint64, const gchar*);
void maki_dbus_emit_shutdown (makiDBus*, gint64);
void maki_dbus_emit_topic (makiDBus*, gint64, const gchar*, const gchar*, const gchar*, const gchar*);
