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

#ifdef HAVE_NICE
#include <interfaces.h>
#include <stun/usages/bind.h>
#endif

#ifdef HAVE_GUPNP_IGD_1_0
#include <libgupnp-igd/gupnp-simple-igd.h>
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

#ifdef HAVE_GUPNP_IGD_1_0
	GUPnPSimpleIgd* upnp_igd;
#endif
};

static void maki_network_update_local (makiNetwork* net)
{
	g_free(net->local.ip);
	net->local.ip = NULL;

#ifdef HAVE_NICE
	{
		GList* ips;

		if ((ips = nice_interfaces_get_local_ips(FALSE)) != NULL)
		{
			GList* l;

			net->local.ip = g_strdup(ips->data);

			for (l = ips; l != NULL; l = g_list_next(l))
			{
				g_free(l->data);
			}

			g_list_free(ips);
		}
	}
#endif
}

static void maki_network_update_remote (makiNetwork* net)
{
	memset(&net->remote.addr, 0, sizeof(net->remote.addr));
	net->remote.addrlen = 0;

#ifdef HAVE_NICE
	{
		gchar* stun;

		struct addrinfo* ai;
		struct addrinfo* p;
		struct addrinfo hints;

		stun = maki_instance_config_get_string(net->instance, "network", "stun");

		if (stun == NULL)
		{
			return;
		}

		net->remote.addrlen = 0;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = 0;
		hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

		if (getaddrinfo(stun, "3478", NULL, &ai) != 0)
		{
			g_free(stun);
			return;
		}

		g_free(stun);

		for (p = ai; p != NULL; p = p->ai_next)
		{
			struct sockaddr_storage me;
			socklen_t me_len = sizeof(me);

			if (stun_usage_bind_run(p->ai_addr, p->ai_addrlen, (struct sockaddr*)&me, &me_len) != STUN_USAGE_BIND_RETURN_SUCCESS)
			{
				continue;
			}

			net->remote.addr = me;
			net->remote.addrlen = me_len;

			break;
		}

		freeaddrinfo(ai);
	}
#endif
}

makiNetwork* maki_network_new (makiInstance* inst)
{
	makiNetwork* net;

	net = g_new(makiNetwork, 1);

	net->instance = inst;

	net->local.ip = NULL;

	memset(&net->remote.addr, 0, sizeof(net->remote.addr));
	net->remote.addrlen = 0;

#ifdef HAVE_GUPNP_IGD_1_0
	net->upnp_igd = gupnp_simple_igd_new(NULL);
#endif

	maki_network_update(net);

	return net;
}

void maki_network_free (makiNetwork* net)
{
#ifdef HAVE_GUPNP_IGD_1_0
	g_object_unref(net->upnp_igd);
#endif

	g_free(net->local.ip);

	g_free(net);
}

void maki_network_update (makiNetwork* net)
{
	g_return_if_fail(net != NULL);

	maki_network_update_local(net);
	maki_network_update_remote(net);
}

gboolean maki_network_remote_addr (makiNetwork* net, struct sockaddr* addr, socklen_t* addrlen)
{
	g_return_val_if_fail(net != NULL, FALSE);
	g_return_val_if_fail(addr != NULL, FALSE);
	g_return_val_if_fail(addrlen != NULL, FALSE);

	if (*addrlen < net->remote.addrlen || net->remote.addrlen == 0)
	{
		return FALSE;
	}

	memcpy(addr, &net->remote.addr, net->remote.addrlen);
	*addrlen = net->remote.addrlen;

	return TRUE;
}

gboolean maki_network_upnp_add_port (makiNetwork* net, guint port, const gchar* description)
{
	g_return_val_if_fail(net != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);
	g_return_val_if_fail(description != NULL, FALSE);

#ifdef HAVE_GUPNP_IGD_1_0
	if (net->local.ip != NULL)
	{
		gupnp_simple_igd_add_port(net->upnp_igd, "TCP", port, net->local.ip, port, 600, description);
	}

	return TRUE;
#endif

	return FALSE;
}

gboolean maki_network_upnp_remove_port (makiNetwork* net, guint port)
{
	g_return_val_if_fail(net != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);

#ifdef HAVE_GUPNP_IGD_1_0
	gupnp_simple_igd_remove_port(net->upnp_igd, "TCP", port);

	return TRUE;
#endif

	return FALSE;
}
