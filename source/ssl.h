/*
 * Copyright (C) 2000-2003, Ximian, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef H_SSL
#define H_SSL

#define SOUP_SSL_ERROR soup_ssl_error_quark()

typedef enum {
	SOUP_SSL_ERROR_HANDSHAKE_NEEDS_READ,
	SOUP_SSL_ERROR_HANDSHAKE_NEEDS_WRITE,
	SOUP_SSL_ERROR_CERTIFICATE
} SoupSSLError;

typedef struct SoupSSLCredentials SoupSSLCredentials;

SoupSSLCredentials *soup_ssl_get_client_credentials  (const char         *ca_file);
void                soup_ssl_free_client_credentials (SoupSSLCredentials *creds);

GQuark              soup_ssl_error_quark             (void);

GIOChannel         *soup_ssl_wrap_iochannel          (GIOChannel         *sock,
						      gboolean            non_blocking,
						      const char         *remote_host,
						      SoupSSLCredentials *creds);

#endif
