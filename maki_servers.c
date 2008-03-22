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

void maki_servers (struct maki* maki)
{
	const gchar* file;
	gchar* path;
	GDir* servers;
	GKeyFile* key_file;

	g_mkdir_with_parents(maki->directories.servers, 0755);

	servers = g_dir_open(maki->directories.servers, 0, NULL);

	while ((file = g_dir_read_name(servers)) != NULL)
	{
		gchar** group;
		gchar** groups;

		path = g_strconcat(maki->directories.servers, G_DIR_SEPARATOR_S, file, NULL);

		key_file = g_key_file_new();

		if (g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL))
		{
			groups = g_key_file_get_groups(key_file, NULL);

			for (group = groups; *group != NULL; ++group)
			{
				if (g_ascii_strncasecmp(*group, "server", 6) == 0)
				{
					gchar* address;
					gchar* nick;
					gchar* name;
					gint port;
					struct maki_connection* m_conn;

					address = g_key_file_get_string(key_file, *group, "address", NULL);
					port = g_key_file_get_integer(key_file, *group, "port", NULL);
					nick = g_key_file_get_string(key_file, *group, "nick", NULL);
					name = g_key_file_get_string(key_file, *group, "name", NULL);

					m_conn = g_new(struct maki_connection, 1);

					m_conn->maki = maki;
					m_conn->server = g_strdup(file);
					m_conn->connection = sashimi_new(address, port, nick, name, maki_callback, m_conn);
					m_conn->channels = g_queue_new();

					sashimi_connect(m_conn->connection);

					g_hash_table_insert(maki->connections, m_conn->server, m_conn);

					g_free(address);
					g_free(nick);
					g_free(name);
				}
				else
				{
					gchar* buffer;
					gboolean autojoin;
					struct maki_connection* m_conn;

					autojoin = g_key_file_get_boolean(key_file, *group, "autojoin", NULL);

					m_conn = g_hash_table_lookup(maki->connections, file);

					if (autojoin)
					{
						g_queue_push_tail(m_conn->channels, g_strdup(*group));
						buffer = g_strdup_printf("JOIN %s", *group);
						sashimi_send(m_conn->connection, buffer);
						g_free(buffer);
					}
				}
			}

			g_strfreev(groups);
		}

		g_key_file_free(key_file);
		g_free(path);
	}

	g_dir_close(servers);
}
