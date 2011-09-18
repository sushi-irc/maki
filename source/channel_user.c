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

#include "config.h"

#include <glib.h>

#include "channel_user.h"

#include "user.h"

struct maki_channel_user
{
	makiUser* user;
	guint prefix;

	guint ref_count;
};

makiChannelUser*
maki_channel_user_new (makiUser* user)
{
	makiChannelUser* cuser;

	cuser = g_new(makiChannelUser, 1);
	cuser->user = maki_user_ref(user);
	cuser->prefix = 0;

	cuser->ref_count = 1;

	return cuser;
}

makiChannelUser*
maki_channel_user_ref (makiChannelUser* cuser)
{
	cuser->ref_count++;

	return cuser;
}

void
maki_channel_user_unref (gpointer data)
{
	makiChannelUser* cuser = data;

	cuser->ref_count--;

	if (cuser->ref_count == 0)
	{
		maki_user_unref(cuser->user);

		g_free(cuser);
	}
}

makiUser*
maki_channel_user_user (makiChannelUser* cuser)
{
	return cuser->user;
}

gboolean
maki_channel_user_prefix_get (makiChannelUser* cuser, guint pos)
{
	return (cuser->prefix & (1 << pos));
}

void
maki_channel_user_prefix_set (makiChannelUser* cuser, guint pos, gboolean set)
{
	if (set)
	{
		cuser->prefix |= (1 << pos);
	}
	else
	{
		cuser->prefix &= ~(1 << pos);
	}
}

void
maki_channel_user_set_prefix (makiChannelUser* cuser, guint prefix)
{
	cuser->prefix = prefix;
}
