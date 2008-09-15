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

#define G_DISABLE_DEPRECATED

#include <glib.h>
#include <glib/gi18n.h>

#include "sashimi.h"

#define SUSHI_NAME "sushi"
#define SUSHI_VERSION "1.0-alpha1"
#define SUSHI_URL "http://sushi.ikkoku.de/"
#define SUSHI_QUIT_MESSAGE SUSHI_NAME " – " SUSHI_URL

#define MAKI_NAME "maki"

#include "cache.h"

/* FIXME */
typedef struct maki_instance makiInstance;

#include "server.h"

#include "user.h"
#include "channel_user.h"

#include "channel.h"
#include "config.h"

#include "dbus.h"

#include "instance.h"

#include "in.h"
#include "log.h"
#include "misc.h"
#include "out.h"

gboolean opt_debug;

GMainLoop* main_loop;
