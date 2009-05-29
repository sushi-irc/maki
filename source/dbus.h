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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

enum
{
	s_action,
	s_away,
	s_away_message,
	s_back,
	s_banlist,
	s_cannot_join,
	s_connect,
	s_connected,
	s_ctcp,
	s_dcc_send,
	s_invite,
	s_join,
	s_kick,
	s_list,
	s_message,
	s_mode,
	s_motd,
	s_names,
	s_nick,
	s_no_such,
	s_notice,
	s_oper,
	s_part,
	s_quit,
	s_shutdown,
	s_topic,
	s_whois,
	s_last
};

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

void maki_dbus_emit (guint, ...);

#endif
