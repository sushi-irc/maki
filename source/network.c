/*
 * Copyright (c) 2009-2010 Michael Kuhn
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

#include "maki.h"

#include "network.h"

#include "ilib.h"

#ifdef HAVE_NICE
#include <interfaces.h>
#include <stun/usages/bind.h>
#endif

struct maki_network
{
	makiInstance* instance;

	struct
	{
		gchar* ip;
	}
	local;

	struct
	{
		struct sockaddr_storage addr;
		socklen_t addrlen;
	}
	remote;

	GMutex* lock;
};

static gboolean maki_network_update_local (gpointer data)
{
	makiNetwork* net = data;

	g_mutex_lock(net->lock);
	g_free(net->local.ip);
	net->local.ip = NULL;
	g_mutex_unlock(net->lock);

#ifdef HAVE_NICE
	{
		GList* ips;

		if ((ips = nice_interfaces_get_local_ips(FALSE)) != NULL)
		{
			GList* l;

			g_mutex_lock(net->lock);
			net->local.ip = g_strdup(ips->data);
			g_mutex_unlock(net->lock);

			for (l = ips; l != NULL; l = g_list_next(l))
			{
				g_free(l->data);
			}

			g_list_free(ips);
		}
	}
#endif

	return FALSE;
}

static gboolean maki_network_update_remote (gpointer data)
{
	makiNetwork* net = data;

	g_mutex_lock(net->lock);
	memset(&net->remote.addr, 0, sizeof(net->remote.addr));
	net->remote.addrlen = 0;
	g_mutex_unlock(net->lock);

#ifdef HAVE_NICE
	{
		gchar* stun;

		struct addrinfo* ai;
		struct addrinfo* p;
		struct addrinfo hints;

		stun = maki_instance_config_get_string(net->instance, "network", "stun");

		if (stun == NULL)
		{
			goto end;
		}

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = 0;
		hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

		if (getaddrinfo(stun, "3478", NULL, &ai) != 0)
		{
			g_free(stun);
			goto end;
		}

		g_free(stun);

		for (p = ai; p != NULL; p = p->ai_next)
		{
			makiNetworkAddress me;
			socklen_t me_len = sizeof(me.ss);

			if (stun_usage_bind_run(p->ai_addr, p->ai_addrlen, &(me.sa), &me_len) != STUN_USAGE_BIND_RETURN_SUCCESS)
			{
				continue;
			}

			g_mutex_lock(net->lock);
			net->remote.addr = me.ss;
			net->remote.addrlen = me_len;
			g_mutex_unlock(net->lock);

			break;
		}

		freeaddrinfo(ai);
	}
#endif

end:
	return FALSE;
}

makiNetwork* maki_network_new (makiInstance* inst)
{
	makiNetwork* net;

	net = g_new(makiNetwork, 1);

	net->instance = inst;

	net->local.ip = NULL;

	memset(&net->remote.addr, 0, sizeof(net->remote.addr));
	net->remote.addrlen = 0;

	net->lock = g_mutex_new();

	return net;
}

void maki_network_free (makiNetwork* net)
{
	g_mutex_free(net->lock);

	g_free(net->local.ip);

	g_free(net);
}

void maki_network_update (makiNetwork* net)
{
	g_return_if_fail(net != NULL);

	i_idle_add(maki_network_update_local, net, maki_instance_main_context(net->instance));
	i_idle_add(maki_network_update_remote, net, maki_instance_main_context(net->instance));
}

gboolean maki_network_remote_addr (makiNetwork* net, struct sockaddr* addr, socklen_t* addrlen)
{
	g_return_val_if_fail(net != NULL, FALSE);
	g_return_val_if_fail(addr != NULL, FALSE);
	g_return_val_if_fail(addrlen != NULL, FALSE);

	g_mutex_lock(net->lock);

	if (*addrlen < net->remote.addrlen || net->remote.addrlen == 0)
	{
		goto error;
	}

	memcpy(addr, &net->remote.addr, net->remote.addrlen);
	*addrlen = net->remote.addrlen;

	g_mutex_unlock(net->lock);

	return TRUE;

error:
	g_mutex_unlock(net->lock);

	return FALSE;
}

gboolean maki_network_upnp_add_port (makiNetwork* net, guint port, const gchar* description)
{
	gboolean (*add_port) (const gchar*, guint, const gchar*);

	if (maki_instance_plugin_method(net->instance, "upnp", "add_port", &add_port))
	{
		return (*add_port)(net->local.ip, port, description);
	}

	return FALSE;
}

gboolean maki_network_upnp_remove_port (makiNetwork* net, guint port)
{
	gboolean (*remove_port) (guint);

	if (maki_instance_plugin_method(net->instance, "upnp", "remove_port", &remove_port))
	{
		return (*remove_port)(port);
	}

	return FALSE;
}
