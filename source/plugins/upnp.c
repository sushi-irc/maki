/*
 * Copyright (c) 2009-2012 Michael Kuhn
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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#ifdef HAVE_GUPNP
#include <libgupnp/gupnp-control-point.h>
#endif

#ifdef HAVE_GUPNP_IGD
#include <libgupnp-igd/gupnp-simple-igd.h>
#endif

#ifdef HAVE_MINIUPNPC
#include <miniupnpc.h>
#include <upnpcommands.h>
#endif

#include <ilib.h>

#include <instance.h>

#include "plugin.h"
#include "upnp.h"

struct UPnPGetExternalIPData
{
	makiUPnPGetExternalIPCallback callback;
	gpointer data;
	gchar* ip;
};

typedef struct UPnPGetExternalIPData UPnPGetExternalIPData;

gboolean get_external_ip (makiUPnPGetExternalIPCallback, gpointer);
gboolean add_port (const gchar*, guint, const gchar*);
gboolean remove_port (guint);

#ifdef HAVE_GUPNP
static GUPnPContext* upnp_context = NULL;
static GUPnPControlPoint* upnp_control_point = NULL;
static GUPnPServiceProxy* upnp_service_proxy = NULL;
#endif

#ifdef HAVE_GUPNP_IGD
static GUPnPSimpleIgd* upnp_igd = NULL;
#endif

#ifdef HAVE_MINIUPNPC
static struct UPNPDev* miniupnpc_dev = NULL;
static struct UPNPUrls miniupnpc_urls = { 0 };
static struct IGDdatas miniupnpc_datas;
#endif

#ifdef HAVE_GUPNP
static
void
on_service_proxy_available (GUPnPControlPoint* cp, GUPnPServiceProxy* proxy, gpointer user_data)
{
	upnp_service_proxy = g_object_ref(proxy);
}
#endif

#ifdef HAVE_GUPNP
static
void
on_get_external_ip (GUPnPServiceProxy* proxy, GUPnPServiceProxyAction* action, gpointer user_data)
{
	UPnPGetExternalIPData* p = user_data;
	GError* error = NULL;
	gchar* ip = NULL;

	gupnp_service_proxy_end_action(proxy,
		action,
		&error,
		"NewExternalIPAddress", G_TYPE_STRING, &ip,
		NULL
	);

	if (error == NULL)
	{
		p->callback(ip, p->data);
	}
	else
	{
		g_error_free(error);
	}

	g_free(ip);
	g_free(p);
}
#endif

#ifdef HAVE_MINIUPNPC
static
gboolean
get_external_ip_idle (gpointer user_data)
{
	UPnPGetExternalIPData* p = user_data;

	p->callback(p->ip, p->data);

	g_free(p->ip);
	g_free(p);

	return FALSE;
}
#endif

G_MODULE_EXPORT
gboolean
get_external_ip (makiUPnPGetExternalIPCallback callback, gpointer data)
{
#ifdef HAVE_GUPNP
	if (upnp_service_proxy != NULL)
	{
		UPnPGetExternalIPData* p;

		p = g_new(UPnPGetExternalIPData, 1);
		p->callback = callback;
		p->data = data;
		p->ip = NULL;

		gupnp_service_proxy_begin_action(upnp_service_proxy,
			"GetExternalIPAddress",
			on_get_external_ip,
			p,
			NULL
		);

		return TRUE;
	}
#endif

#ifdef HAVE_MINIUPNPC
	{
		UPnPGetExternalIPData* p;
		gchar external_ip[64] = { '\0' };

		UPNP_GetExternalIPAddress(miniupnpc_urls.controlURL, miniupnpc_datas.first.servicetype, external_ip);

		p = g_new(UPnPGetExternalIPData, 1);
		p->callback = callback;
		p->data = data;
		p->ip = g_strdup(external_ip);

		i_idle_add(get_external_ip_idle, p, g_main_context_get_thread_default());

		return TRUE;
	}
#endif

	return FALSE;
}

G_MODULE_EXPORT
gboolean
add_port (const gchar* ip, guint port, const gchar* description)
{
	g_return_val_if_fail(ip != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);
	g_return_val_if_fail(description != NULL, FALSE);

#ifdef HAVE_GUPNP_IGD
	gupnp_simple_igd_add_port(upnp_igd, "TCP", port, ip, port, 600, description);

	return TRUE;
#endif

#ifdef HAVE_MINIUPNPC
	{
		gchar* port_str;

		port_str = g_strdup_printf("%d", port);
#ifdef HAVE_MINIUPNPC_16
		UPNP_AddPortMapping(miniupnpc_urls.controlURL, miniupnpc_datas.first.servicetype, port_str, port_str, ip, description, "TCP", NULL, "600");
#else
		UPNP_AddPortMapping(miniupnpc_urls.controlURL, miniupnpc_datas.first.servicetype, port_str, port_str, ip, description, "TCP", NULL);
#endif
		g_free(port_str);

		return TRUE;
	}
#endif

	return FALSE;
}

G_MODULE_EXPORT
gboolean
remove_port (guint port)
{
	g_return_val_if_fail(port != 0, FALSE);

#ifdef HAVE_GUPNP_IGD
	gupnp_simple_igd_remove_port(upnp_igd, "TCP", port);

	return TRUE;
#endif

#ifdef HAVE_MINIUPNPC
	{
		gchar* port_str;

		port_str = g_strdup_printf("%d", port);
		UPNP_DeletePortMapping(miniupnpc_urls.controlURL, miniupnpc_datas.first.servicetype, port_str, "TCP", NULL);
		g_free(port_str);

		return TRUE;
	}
#endif

	return FALSE;
}

G_MODULE_EXPORT
gboolean
init (void)
{
#ifdef HAVE_GUPNP
	upnp_context = gupnp_context_new(NULL, NULL, 0, NULL);
	g_assert(upnp_context != NULL);

	upnp_control_point = gupnp_control_point_new(upnp_context, "urn:schemas-upnp-org:service:WANIPConnection:1");
	g_assert(upnp_control_point != NULL);

	upnp_service_proxy = NULL;

	g_signal_connect(upnp_control_point, "service-proxy-available", G_CALLBACK(on_service_proxy_available), NULL);

	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(upnp_control_point), TRUE);
#endif

#ifdef HAVE_GUPNP_IGD
	upnp_igd = gupnp_simple_igd_new();
	g_assert(upnp_igd != NULL);
#endif

#ifdef HAVE_MINIUPNPC
	{
		gchar addr[64] = { '\0' };
#ifdef HAVE_MINIUPNPC_16
		gint error;

		miniupnpc_dev = upnpDiscover(1000, NULL, NULL, 0, 0, &error);
#else
		miniupnpc_dev = upnpDiscover(1000, NULL, NULL, 0);
#endif
		g_assert(miniupnpc_dev != NULL);
		//g_assert(error == UPNPDISCOVER_SUCCESS);
		UPNP_GetValidIGD(miniupnpc_dev, &miniupnpc_urls, &miniupnpc_datas, addr, sizeof(addr));
	}
#endif

	return TRUE;
}

G_MODULE_EXPORT
void
deinit (void)
{
#ifdef HAVE_GUPNP
	if (upnp_service_proxy != NULL)
	{
		g_object_unref(upnp_service_proxy);
	}

	g_object_unref(upnp_context);
	g_object_unref(upnp_control_point);
#endif

#ifdef HAVE_GUPNP_IGD
	g_object_unref(upnp_igd);
#endif

#ifdef HAVE_MINIUPNPC
	FreeUPNPUrls(&miniupnpc_urls);
	freeUPNPDevlist(miniupnpc_dev);
#endif
}
