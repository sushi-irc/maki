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

#ifndef H_MAKI
#define H_MAKI

#define G_DISABLE_DEPRECATED
#define _XOPEN_SOURCE

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "ilib.h"

#define SUSHI_DBUS_SERVICE "de.ikkoku.sushi"
#define SUSHI_DBUS_PATH "/de/ikkoku/sushi"
#define SUSHI_DBUS_INTERFACE "de.ikkoku.sushi"

#include "channel.h"
#include "channel_user.h"
#include "dcc_send.h"
#include "dbus.h"
#include "dbus_server.h"
#include "in.h"
#include "instance.h"
#include "log.h"
#include "maki.h"
#include "misc.h"
#include "out.h"
#include "plugin.h"
#include "server.h"
#include "user.h"

extern gboolean opt_verbose;

extern makiDBus* dbus;
extern makiDBusServer* dbus_server;
extern GMainLoop* main_loop;

#endif
