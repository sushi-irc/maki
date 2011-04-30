/*
 * Copyright (c) 2009-2011 Michael Kuhn
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

#define _XOPEN_SOURCE

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include <string.h>

#ifdef HAVE_NICE
#include <interfaces.h>
#include <stun/usages/bind.h>
#endif

#include <ilib.h>

#include "network.h"

struct maki_network
{
	makiInstance* instance;

	struct
	{
		GInetAddress* inet_address;
	}
	internal;

	struct
	{
		GInetAddress* inet_address;
	}
	external;

	GMutex* lock;
};

static
gboolean
maki_network_update_internal (gpointer data)
{
	makiNetwork* net = data;

	g_mutex_lock(net->lock);

	if (net->internal.inet_address != NULL)
	{
		g_object_unref(net->internal.inet_address);
	}

	net->internal.inet_address = NULL;

#ifdef HAVE_NICE
	{
		GList* ips;

		if ((ips = nice_interfaces_get_local_ips(FALSE)) != NULL)
		{
			GList* l;

			net->internal.inet_address = g_inet_address_new_from_string(ips->data);

			for (l = ips; l != NULL; l = g_list_next(l))
			{
				g_free(l->data);
			}

			g_list_free(ips);
		}
	}
#endif

	g_mutex_unlock(net->lock);

	return FALSE;
}

static
gboolean
maki_network_update_external (gpointer data)
{
	makiNetwork* net = data;

	g_mutex_lock(net->lock);

	if (net->external.inet_address != NULL)
	{
		g_object_unref(net->external.inet_address);
	}

	net->external.inet_address = NULL;

	{
		gchar* (*get_external_ip) (void);

		if (maki_instance_plugin_method(net->instance, "upnp", "get_external_ip", (gpointer*)&get_external_ip))
		{
			gchar* ip;

			if ((ip = (*get_external_ip)()) != NULL)
			{
				net->external.inet_address = g_inet_address_new_from_string(ip);

				g_free(ip);

				goto end;
			}
		}
	}

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

		if (getaddrinfo(stun, "3478", &hints, &ai) != 0)
		{
			g_free(stun);
			goto end;
		}

		g_free(stun);

		for (p = ai; p != NULL; p = p->ai_next)
		{
			makiNetworkAddress me;
			GInetAddress* inet_address = NULL;
			socklen_t me_len = sizeof(me);

			if (stun_usage_bind_run(p->ai_addr, p->ai_addrlen, &(me.sa), &me_len) != STUN_USAGE_BIND_RETURN_SUCCESS)
			{
				continue;
			}

			if (me.sa.sa_family == AF_INET)
			{
				inet_address = g_inet_address_new_from_bytes((gconstpointer)&(me.sin.sin_addr.s_addr), G_SOCKET_FAMILY_IPV4);
			}
			else if (me.sa.sa_family == AF_INET6)
			{
				inet_address = g_inet_address_new_from_bytes((gconstpointer)&(me.sin6.sin6_addr.s6_addr), G_SOCKET_FAMILY_IPV6);
			}

			net->external.inet_address = inet_address;

			break;
		}

		freeaddrinfo(ai);
	}
#endif

end:
	g_mutex_unlock(net->lock);

	return FALSE;
}

makiNetwork*
maki_network_new (makiInstance* inst)
{
	makiNetwork* net;

	net = g_new(makiNetwork, 1);

	net->instance = inst;

	net->internal.inet_address = NULL;
	net->external.inet_address = NULL;

	net->lock = g_mutex_new();

	return net;
}

void
maki_network_free (makiNetwork* net)
{
	g_mutex_free(net->lock);

	if (net->internal.inet_address != NULL)
	{
		g_object_unref(net->internal.inet_address);
	}

	if (net->external.inet_address != NULL)
	{
		g_object_unref(net->external.inet_address);
	}

	g_free(net);
}

void
maki_network_update (makiNetwork* net)
{
	g_return_if_fail(net != NULL);

	g_mutex_lock(net->lock);
	/* FIXME sources */
	i_idle_add(maki_network_update_internal, net, maki_instance_main_context(net->instance));
	i_idle_add(maki_network_update_external, net, maki_instance_main_context(net->instance));
	g_mutex_unlock(net->lock);
}

GInetAddress*
maki_network_internal_address (makiNetwork* net)
{
	GInetAddress* inet_address;

	g_return_val_if_fail(net != NULL, NULL);

	g_mutex_lock(net->lock);
	inet_address = g_object_ref(net->internal.inet_address);
	g_mutex_unlock(net->lock);

	return inet_address;
}

GInetAddress*
maki_network_external_address (makiNetwork* net)
{
	GInetAddress* inet_address;

	g_return_val_if_fail(net != NULL, NULL);

	g_mutex_lock(net->lock);
	inet_address = g_object_ref(net->external.inet_address);
	g_mutex_unlock(net->lock);

	return inet_address;
}

gboolean
maki_network_upnp_add_port (makiNetwork* net, guint port, gchar const* description)
{
	gboolean (*add_port) (gchar const*, guint, gchar const*);
	gboolean ret = FALSE;

	g_mutex_lock(net->lock);

	if (maki_instance_plugin_method(net->instance, "upnp", "add_port", (gpointer*)&add_port))
	{
		gchar* ip;

		ip = g_inet_address_to_string(net->internal.inet_address);
		ret = (*add_port)(ip, port, description);
		g_free(ip);
	}

	g_mutex_unlock(net->lock);

	return ret;
}

gboolean
maki_network_upnp_remove_port (makiNetwork* net, guint port)
{
	gboolean (*remove_port) (guint);
	gboolean ret = FALSE;

	g_mutex_lock(net->lock);

	if (maki_instance_plugin_method(net->instance, "upnp", "remove_port", (gpointer*)&remove_port))
	{
		ret = (*remove_port)(port);
	}

	g_mutex_unlock(net->lock);

	return ret;
}
