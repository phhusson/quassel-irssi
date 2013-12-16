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
#include <levels.h>
#include <signals.h>

//fe-common/core
#include <fe-windows.h>
#include <printtext.h>

#include "quassel-irssi.h"
#include "irssi/irssi-gui.h"

static void quassel_chan_read(Quassel_CHANNEL_REC* chanrec) {
	GIOChannel *giochan = net_sendbuffer_handle(chanrec->server->handle);
	quassel_set_last_seen_msg(giochan, chanrec->buffer_id, chanrec->last_msg_id);
	quassel_set_marker(giochan, chanrec->buffer_id, chanrec->last_msg_id);
	quassel_mark_as_read(giochan, chanrec->buffer_id);
}

static Quassel_CHANNEL_REC* window2chanrec(WINDOW_REC *window) {
	if(!window)
		return NULL;
	WI_ITEM_REC *wi = window->active;
	if(!wi)
		return NULL;
	Quassel_SERVER_REC *server = (Quassel_SERVER_REC*)wi->server;
	if(!PROTO_CHECK_CAST(SERVER(server), Quassel_SERVER_REC, chat_type, "Quassel"))
		return NULL;
	Quassel_CHANNEL_REC *chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(server), wi->visible_name);
	return chanrec;
}

void quassel_irssi_check_read(Quassel_CHANNEL_REC* chanrec) {
	if(!chanrec)
		return;
	Quassel_CHANNEL_REC *active_chanrec = window2chanrec(active_win);
	if(active_chanrec != chanrec)
		return;

	quassel_chan_read(chanrec);
}

static void window_read(WINDOW_REC* win) {
	if(!win)
		return;
	Quassel_CHANNEL_REC *chanrec = window2chanrec(win);
	if(!chanrec)
		return;

	quassel_chan_read(chanrec);
}

static void sig_window_changed(WINDOW_REC *active, WINDOW_REC *old) {
	window_read(active);
	window_read(old);

	textbuffer_view_set_bookmark_bottom(WINDOW_GUI(active)->view, "useless_end");
	LINE_REC *linerec = textbuffer_view_get_bookmark(WINDOW_GUI(active)->view, "trackbar");
	LINE_REC *last_line = textbuffer_view_get_bookmark(WINDOW_GUI(active)->view, "useless_end");
	if(linerec == last_line && linerec)
		textbuffer_view_remove_line(WINDOW_GUI(active)->view, linerec);
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
	chanrec->last_seen_msg_id = msgid;
	if(chanrec->init_last_seen_msg_id == -1)
		chanrec->init_last_seen_msg_id = msgid;

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

			if(winrec != active_win) {
				LINE_REC *linerec = textbuffer_view_get_bookmark(WINDOW_GUI(winrec)->view, "trackbar");
				if(linerec)
					textbuffer_view_remove_line(WINDOW_GUI(winrec)->view, linerec);

				char *str = malloc(winrec->width+3);
				str[0] = '%';
				str[1] = 'K';
				for(int i=0; i<winrec->width; ++i)
					str[i+2]='-';
				str[winrec->width+2]=0;
				printtext_string_window(winrec, MSGLEVEL_NEVER, str);
				textbuffer_view_set_bookmark_bottom(WINDOW_GUI(winrec)->view, "trackbar");
			}
		}

next:
		win = g_slist_next(win);
	}
}


extern void mainwindows_redraw(void);
void quassel_irssi_backlog(void* arg, int msg_id, int timestamp, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content) {
	(void) msg_id;
	(void) bufferid;
	(void) type;
	(void) flags;
	Quassel_SERVER_REC *server = (Quassel_SERVER_REC*)arg;

	char *chan = channame(network, buffer_id);
	char *nick = strdup(sender);
	char *address;
	if( (address=index(nick, '!')) != NULL)
		*address = 0;
	address++;

	GSList *win = windows;
	while(win) {
		WINDOW_REC* winrec = (WINDOW_REC*) win->data;
		if(winrec->active_server != SERVER(server) &&
			winrec->connect_server != SERVER(server))
			goto next;

		if(!winrec->active)
			goto next;

		if(strcmp(winrec->active->visible_name, chan)!=0)
			goto next;

		if(!WINDOW_GUI(winrec) ||
				!WINDOW_GUI(winrec)->view ||
				!WINDOW_GUI(winrec)->view->buffer)
			goto next;

		LINE_INFO_REC lineinforec;
		lineinforec.time=timestamp;
		lineinforec.level = 0;

		LINE_REC* pos = WINDOW_GUI(winrec)->view->buffer->first_line;
		LINE_REC* before = pos;
		for(; pos!=NULL; pos = pos->next) {
			if(pos->info.time >= timestamp)
				break;
			before = pos;
		}

		unsigned char *data = NULL;
		int len = asprintf((char**)&data, "%d:%s:%sxx", timestamp, nick, content);
		data[len-2]=0;
		data[len-1]=0x80;
		LINE_REC *linerec = textbuffer_insert(WINDOW_GUI(winrec)->view->buffer, before, data, len, &lineinforec);
		free(data);
		textbuffer_view_insert_line(WINDOW_GUI(winrec)->view, linerec);
		if(WINDOW_GUI(winrec)->insert_after)
			WINDOW_GUI(winrec)->insert_after = linerec;

		WINDOW_GUI(winrec)->view->dirty = 1;
		winrec->last_line = time(NULL);

		mainwindows_redraw();
next:
		win = g_slist_next(win);
	}
	free(nick);
}

void quassel_fewindow_init(void) {
	signal_add("window changed", (SIGNAL_FUNC) sig_window_changed);
}

void quassel_fewindow_deinit(void) {
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
}
