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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#ifdef HAVE_GUPNP
#include <libgupnp/gupnp-control-point.h>
#endif

#ifdef HAVE_GUPNP_IGD
#include <libgupnp-igd/gupnp-simple-igd.h>
#endif

#include <instance.h>

gboolean init (void);
void deinit (void);

gchar* get_external_ip (void);
gboolean add_port (const gchar*, guint, const gchar*);
gboolean remove_port (guint);

#ifdef HAVE_GUPNP
GUPnPContext* upnp_context;
GUPnPControlPoint* upnp_control_point;
GUPnPServiceProxy* upnp_service_proxy;
#endif

#ifdef HAVE_GUPNP_IGD
GUPnPSimpleIgd* upnp_igd;
#endif

#ifdef HAVE_GUPNP
static
void
on_service_proxy_available (GUPnPControlPoint* cp, GUPnPServiceProxy* proxy, gpointer user_data)
{
	upnp_service_proxy = g_object_ref(proxy);
}
#endif

G_MODULE_EXPORT
gchar*
get_external_ip (void)
{
#ifdef HAVE_GUPNP
	if (upnp_service_proxy != NULL)
	{
		GError* error = NULL;
		gchar* ip;

		gupnp_service_proxy_send_action(upnp_service_proxy,
			"GetExternalIPAddress",
			&error,
			NULL,
			"NewExternalIPAddress", G_TYPE_STRING, &ip,
			NULL
		);

		if (error == NULL)
		{
			return ip;
		}

		g_error_free(error);
	}
#endif

	return NULL;
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
#endif

	return TRUE;
}

G_MODULE_EXPORT
gboolean
remove_port (guint port)
{
	g_return_val_if_fail(port != 0, FALSE);

#ifdef HAVE_GUPNP_IGD
	gupnp_simple_igd_remove_port(upnp_igd, "TCP", port);
#endif

	return TRUE;
}

G_MODULE_EXPORT
gboolean
init (void)
{
	makiInstance* inst = maki_instance_get_default();

#ifdef HAVE_GUPNP
	upnp_context = gupnp_context_new(maki_instance_main_context(inst), NULL, 0, NULL);
	g_assert(upnp_context != NULL);
	upnp_control_point = gupnp_control_point_new(upnp_context, "urn:schemas-upnp-org:service:WANIPConnection:1");
	g_assert(upnp_control_point != NULL);
	upnp_service_proxy = NULL;

	g_signal_connect(upnp_control_point, "service-proxy-available", G_CALLBACK(on_service_proxy_available), NULL);

	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(upnp_control_point), TRUE);
#endif

#ifdef HAVE_GUPNP_IGD
	upnp_igd = gupnp_simple_igd_new(maki_instance_main_context(inst));
	g_assert(upnp_igd != NULL);
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
}
