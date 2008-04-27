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

#include "maki.h"

struct maki_config* maki_config_new (const gchar* path)
{
	struct maki_config* m_config;

	m_config = g_new(struct maki_config, 1);

	m_config->logging.enabled = TRUE;
	m_config->logging.time_format = g_strdup("%Y-%m-%d %H:%M:%S");
	m_config->reconnect.retries = 3;
	m_config->reconnect.timeout = 10;

	maki_config_reload(m_config, path);

	return m_config;
}

#define maki_config_value(key, value) if (error) { g_error_free(error); error = NULL; } else { m_config->key = value; }
#define maki_config_string(key, value) if (error) { g_error_free(error); error = NULL; } else { g_free(m_config->key); m_config->key = value; }

void maki_config_reload (struct maki_config* m_config, const gchar* path)
{
	GKeyFile* key_file;

	key_file = g_key_file_new();

	if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
	{
		gboolean logging_enabled;
		gchar* logging_time_format;
		gint reconnect_retries;
		gint reconnect_timeout;
		GError* error = NULL;

		logging_enabled = g_key_file_get_boolean(key_file, "logging", "enabled", &error);
		maki_config_value(logging.enabled, logging_enabled);

		logging_time_format = g_key_file_get_string(key_file, "logging", "time_format", &error);
		maki_config_string(logging.time_format, logging_time_format);

		reconnect_retries = g_key_file_get_integer(key_file, "reconnect", "retries", &error);
		maki_config_value(reconnect.retries, reconnect_retries);

		reconnect_timeout = g_key_file_get_integer(key_file, "reconnect", "timeout", &error);
		maki_config_value(reconnect.timeout, reconnect_timeout);
	}

	g_key_file_free(key_file);
}

void maki_config_free (struct maki_config* m_config)
{
	g_free(m_config->logging.time_format);

	g_free(m_config);
}
