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

#define MAKI_NAME "maki"

struct maki;

#include "maki_cache.h"
#include "maki_channel.h"
#include "maki_channel_user.h"
#include "maki_config.h"
#include "maki_connection.h"
#include "maki_dbus.h"
#include "maki_in.h"
#include "maki_log.h"
#include "maki_misc.h"
#include "maki_out.h"
#include "maki_servers.h"
#include "maki_user.h"

struct maki
{
	makiDBus* bus;

	struct maki_config* config;

	GHashTable* connections;

	struct
	{
		gchar* config;
		gchar* logs;
		gchar* servers;
		gchar* sushi;
	}
	directories;

	GMainLoop* loop;

	GAsyncQueue* message_queue;

	struct
	{
		gboolean debug;
	}
	opt;

	struct
	{
		GThread* messages;
	}
	threads;
};

struct maki* maki (void);
struct maki* maki_new (void);
void maki_free (struct maki*);
