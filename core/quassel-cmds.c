/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <module.h>
#include <channels-setup.h>
#include <channels.h>
#include <chat-protocols.h>
#include <chatnets.h>
#include <commands.h>
#include <net-sendbuffer.h>
#include <network.h>
#include <queries.h>
#include <recode.h>
#include <servers-setup.h>
#include <nicklist.h>
#include <servers.h>
#include <settings.h>
#include <signals.h>

//fe-common/core
#include <fe-windows.h>

#include "quassel-irssi.h"

static void cmd_quote(const char *arg, Quassel_SERVER_REC* server, WI_ITEM_REC* wi) {
	char *cmd = NULL;
	int len = asprintf(&cmd, "/%s", arg);
	(void)len;
	quassel_irssi_send_message(SERVER(server), wi->visible_name, cmd, 0);
	free(cmd);
}

static void cmd_qbacklog(const char *arg, Quassel_SERVER_REC *server, WI_ITEM_REC* wi) {
	(void)arg;
	int n = atoi(arg);
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(server), wi->visible_name);
	if(!chanrec)
		return;

	int first = -1;
	int additional = 0;
	int last = chanrec->first_msg_id;
	if(chanrec->init_last_seen_msg_id != -1) {
		first = chanrec->init_last_seen_msg_id;
		if(!n) {
			additional = 0;
			n = 150;
		} else {
			additional = n;
			n = 150;
		}
	} else {
		n = n ? n : 10;
	}
	if(chanrec->buffer_id != -1) {
		quassel_request_backlog(chanrec->server->handle->handle, chanrec->buffer_id, first, last, n, additional);
	}

	signal_stop();
}

static void cmd_self(const char *arg, Quassel_SERVER_REC* server, WI_ITEM_REC* wi) {
	if(SERVER(server)->chat_type != Quassel_PROTOCOL)
		return;

	char *cmd = NULL;
	int len = asprintf(&cmd, "/%s %s", current_command, arg);
	(void)len;
	quassel_irssi_send_message(SERVER(server), wi ? wi->visible_name: "", cmd, 0);
	free(cmd);

	signal_stop();
}

void quassel_cmds_init(void) {
	command_bind_proto("quote", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_quote);
	command_bind_proto("qbacklog", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_qbacklog);

	//Commands that are sent as is to quassel core
	command_bind_proto_first("away", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("ban", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("unban", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("ctcp", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("delkey", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("deop", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("dehalfop", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("devoice", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("invite", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("join", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("keyx", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("kick", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("kill", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("list", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("me", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("mode", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("nick", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("notice", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("oper", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("op", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("halfop", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("part", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("ping", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("query", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("setkey", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("showkey", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("topic", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("voice", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("wait", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("who", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
	command_bind_proto_first("whois", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_self);
}

void quassel_cmds_deinit(void) {
	command_unbind("quote", (SIGNAL_FUNC) cmd_quote);
	command_unbind("qbacklog", (SIGNAL_FUNC) cmd_qbacklog);

	//Commands that are sent as is to quassel core
	//Created from init command with
	// s/\vcommand_bind_proto_first(.*), Quassel_PROTOCOL, NULL(.*)$/command_unbind\1\2/g
	command_unbind("away", (SIGNAL_FUNC) cmd_self);
	command_unbind("ban", (SIGNAL_FUNC) cmd_self);
	command_unbind("unban", (SIGNAL_FUNC) cmd_self);
	command_unbind("ctcp", (SIGNAL_FUNC) cmd_self);
	command_unbind("delkey", (SIGNAL_FUNC) cmd_self);
	command_unbind("deop", (SIGNAL_FUNC) cmd_self);
	command_unbind("dehalfop", (SIGNAL_FUNC) cmd_self);
	command_unbind("devoice", (SIGNAL_FUNC) cmd_self);
	command_unbind("invite", (SIGNAL_FUNC) cmd_self);
	command_unbind("join", (SIGNAL_FUNC) cmd_self);
	command_unbind("keyx", (SIGNAL_FUNC) cmd_self);
	command_unbind("kick", (SIGNAL_FUNC) cmd_self);
	command_unbind("kill", (SIGNAL_FUNC) cmd_self);
	command_unbind("list", (SIGNAL_FUNC) cmd_self);
	command_unbind("me", (SIGNAL_FUNC) cmd_self);
	command_unbind("mode", (SIGNAL_FUNC) cmd_self);
	command_unbind("nick", (SIGNAL_FUNC) cmd_self);
	command_unbind("notice", (SIGNAL_FUNC) cmd_self);
	command_unbind("oper", (SIGNAL_FUNC) cmd_self);
	command_unbind("op", (SIGNAL_FUNC) cmd_self);
	command_unbind("halfop", (SIGNAL_FUNC) cmd_self);
	command_unbind("part", (SIGNAL_FUNC) cmd_self);
	command_unbind("ping", (SIGNAL_FUNC) cmd_self);
	command_unbind("query", (SIGNAL_FUNC) cmd_self);
	command_unbind("setkey", (SIGNAL_FUNC) cmd_self);
	command_unbind("showkey", (SIGNAL_FUNC) cmd_self);
	command_unbind("topic", (SIGNAL_FUNC) cmd_self);
	command_unbind("voice", (SIGNAL_FUNC) cmd_self);
	command_unbind("wait", (SIGNAL_FUNC) cmd_self);
	command_unbind("who", (SIGNAL_FUNC) cmd_self);
	command_unbind("whois", (SIGNAL_FUNC) cmd_self);
}
