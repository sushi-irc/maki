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

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include "plugin.h"

GModule*
maki_plugin_load (const gchar* name)
{
	gchar* path;
	GModule* module = NULL;

	if ((path = g_module_build_path(MAKI_PLUGIN_DIRECTORY, name)) != NULL)
	{
		if ((module = g_module_open(path, G_MODULE_BIND_LOCAL)) != NULL)
		{
			makiPluginInitFunc init;

			if (g_module_symbol(module, "init", (gpointer*)&init))
			{
				(*init)();
			}
		}

		g_free(path);
	}

	return module;
}

void
maki_plugin_unload (gpointer data)
{
	GModule* module = data;
	makiPluginDeinitFunc deinit;

	if (g_module_symbol(module, "deinit", (gpointer*)&deinit))
	{
		(*deinit)();
	}

	g_module_close(module);
}
