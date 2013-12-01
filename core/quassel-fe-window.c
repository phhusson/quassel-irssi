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

//fe-windows.c
extern WINDOW_REC *active_win;
extern void quassel_mark_as_read(GIOChannel*, int);
extern void quassel_set_last_seen_msg(GIOChannel*, int, int);

static void quassel_chan_read(Quassel_CHANNEL_REC* chanrec) {
	GIOChannel *giochan = net_sendbuffer_handle(chanrec->server->handle);
	quassel_set_last_seen_msg(giochan, chanrec->buffer_id, chanrec->last_msg_id);
	quassel_mark_as_read(giochan, chanrec->buffer_id);
}

void quassel_irssi_check_read(Quassel_CHANNEL_REC* chanrec) {
	if(!active_win)
		return;
	WI_ITEM_REC *wi = active_win->active;
	if(!wi)
		return;
	Quassel_SERVER_REC *active_server = (Quassel_SERVER_REC*)wi->server;
	if(!PROTO_CHECK_CAST(SERVER(active_server), Quassel_SERVER_REC, chat_type, "Quassel"))
		return;
	Quassel_CHANNEL_REC *active_chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(active_server), wi->visible_name);
	if(active_chanrec != chanrec)
		return;

	quassel_chan_read(chanrec);
}

static void window_read(WINDOW_REC* win) {
	if(!win)
		return;
	WI_ITEM_REC *wi = win->active;
	if(!wi)
		return;
	Quassel_SERVER_REC *server = (Quassel_SERVER_REC*)wi->server;
	if(!PROTO_CHECK_CAST(SERVER(server), Quassel_SERVER_REC, chat_type, "Quassel"))
		return;
	Quassel_CHANNEL_REC *chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(server), wi->visible_name);

	quassel_chan_read(chanrec);
}

static void sig_window_changed(WINDOW_REC *active, WINDOW_REC *old) {
	window_read(active);
	window_read(old);
}

void quassel_irssi_set_last_seen_msg(void *arg, int buffer_id, int msgid) {
	(void) msgid;

	Quassel_SERVER_REC *server = (Quassel_SERVER_REC*)arg;
	if(!PROTO_CHECK_CAST(SERVER(server), Quassel_SERVER_REC, chat_type, "Quassel"))
		return;

	Quassel_CHANNEL_REC* chanrec = NULL;
	//First find channel
	GSList *chans = server->channels;
	while(chans) {
		chanrec = (Quassel_CHANNEL_REC*) chans->data;
		if(chanrec->buffer_id == buffer_id)
			break;
		chanrec = NULL;
		chans = g_slist_next(chans);
	}
	if(!chanrec)
		return;

	//Now find windows
	GSList *win = windows;
	while(win) {
		WINDOW_REC* winrec = (WINDOW_REC*) win->data;
		if(winrec->active_server != SERVER(server) &&
			winrec->connect_server != SERVER(server))
			goto next;

		if(!winrec->active)
			goto next;
		if(strcmp(winrec->active->visible_name, chanrec->name)==0) {
				signal_emit("window dehilight", 1, winrec);
		}
#if 0
		GSList *bound = winrec->bound_items;
		while(bound) {
			WINDOW_BIND_REC* bindrec = (WINDOW_BIND_REC*) bound->data;
			fprintf(stderr, "%s VS %s\n", bindrec->name, chanrec->name);
			if(strcmp(bindrec->name, chanrec->name) == 0) {
	fprintf(stderr, "%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
				signal_emit("window dehilight", 1, winrec);
			}
			bound = g_slist_next(bound);
		}
#endif

next:
		win = g_slist_next(win);
	}
}

void quassel_fewindow_init(void) {
	signal_add("window changed", (SIGNAL_FUNC) sig_window_changed);
}

void quassel_fewindow_deinit(void) {
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
}
