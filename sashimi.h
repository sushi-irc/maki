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

#ifndef _SASHIMI_H_
#define _SASHIMI_H_

struct sashimi_connection;

struct sashimi_message
{
	gchar* message;
	gpointer data;
};


struct sashimi_message* sashimi_message_new (gchar*, gpointer);
void sashimi_message_free (gpointer);

struct sashimi_connection* sashimi_new (const gchar*, gushort, GAsyncQueue*, gpointer);
void sashimi_reconnect (struct sashimi_connection*, void (*) (gpointer), gpointer);
void sashimi_timeout (struct sashimi_connection*, guint);
int sashimi_connect (struct sashimi_connection*);
int sashimi_send (struct sashimi_connection*, const gchar*);
int sashimi_queue (struct sashimi_connection*, const gchar*);
int sashimi_disconnect (struct sashimi_connection*);
int sashimi_free (struct sashimi_connection*);

#endif
