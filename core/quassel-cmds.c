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

extern void irssi_send_message(GIOChannel* h, int buffer, const char *message);
extern int quassel_find_buffer_id(const char *name, int network);
extern void quassel_irssi_send_message(SERVER_REC *server, const char *target,
			 const char *msg, int target_type);

static void cmd_quote(const char *arg, Quassel_SERVER_REC* server, WI_ITEM_REC* wi) {
	char *cmd = NULL;
	int len = asprintf(&cmd, "/%s", arg);
	(void)len;
	quassel_irssi_send_message(SERVER(server), wi->visible_name, cmd, 0);
	free(cmd);
}

void quassel_request_backlog(GIOChannel *h, int buffer, int first, int last, int limit, int additional);
void cmd_qbacklog(const char *arg, Quassel_SERVER_REC *server, WI_ITEM_REC* wi) {
	(void)arg;
	int n = atoi(arg);
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(server), wi->visible_name);
	if(!chanrec)
		return;

	int first = -1;
	int additional = 0;
	int last = chanrec->first_msg_id;
	if(chanrec->last_seen_msg_id != -1) {
		first = chanrec->last_seen_msg_id;
		if(n) {
			additional = 0;
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
void quassel_cmds_init(void) {
	command_bind_proto("quote", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_quote);
	command_bind_proto("qbacklog", Quassel_PROTOCOL, NULL, (SIGNAL_FUNC) cmd_qbacklog);
}

void quassel_cmds_deinit(void) {
	command_unbind("quote", (SIGNAL_FUNC) cmd_quote);
	command_unbind("qbacklog", (SIGNAL_FUNC) cmd_qbacklog);
}
