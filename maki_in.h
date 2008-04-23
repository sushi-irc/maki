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

#define IRC_RPL_ISUPPORT "005"
#define IRC_RPL_UMODEIS "221"
#define IRC_RPL_AWAY "301"
#define IRC_RPL_UNAWAY "305"
#define IRC_RPL_NOWAWAY "306"
#define IRC_RPL_CHANNELMODEIS "324"
#define IRC_RPL_TOPIC "332"
#define IRC_RPL_NAMREPLY "353"
#define IRC_RPL_ENDOFNAMES "366"
#define IRC_RPL_MOTD "372"
#define IRC_RPL_ENDOFMOTD "376"
#define IRC_ERR_NOMOTD "422"
#define IRC_ERR_NICKNAMEINUSE "433"

gchar* maki_remove_colon (gchar*);
void maki_nickserv (struct maki_connection*);
gboolean maki_mode_has_parameter (struct maki_connection*, gchar, gchar);
gboolean maki_join (gpointer);
gboolean maki_is_channel(struct maki_connection*, const gchar*);
gpointer maki_in_runner (gpointer);
