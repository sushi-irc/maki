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

#include <libgupnp-igd/gupnp-simple-igd.h>

gboolean init (void);
void deinit (void);

gboolean add_port (const gchar*, guint, const gchar*);
gboolean remove_port (guint);

GUPnPSimpleIgd* upnp_igd;

G_MODULE_EXPORT
gboolean add_port (const gchar* ip, guint port, const gchar* description)
{
	g_return_val_if_fail(ip != NULL, FALSE);
	g_return_val_if_fail(port != 0, FALSE);
	g_return_val_if_fail(description != NULL, FALSE);

	gupnp_simple_igd_add_port(upnp_igd, "TCP", port, ip, port, 600, description);

	return TRUE;
}

G_MODULE_EXPORT
gboolean remove_port (guint port)
{
	g_return_val_if_fail(port != 0, FALSE);

	gupnp_simple_igd_remove_port(upnp_igd, "TCP", port);

	return TRUE;
}

G_MODULE_EXPORT
gboolean init (void)
{
	upnp_igd = gupnp_simple_igd_new(NULL);

	return TRUE;
}

G_MODULE_EXPORT
void deinit (void)
{
	g_object_unref(upnp_igd);
}
