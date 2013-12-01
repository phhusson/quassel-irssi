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
//fe-text/gui-window.h
#define WINDOW_GUI(a) ((GUI_WINDOW_REC *) ((a)->gui_data))                         

typedef struct {
	struct _MAIN_WINDOW_REC *parent;
	struct _TEXT_BUFFER_VIEW_REC *view;

	unsigned int scroll:1;
	unsigned int use_scroll:1;

	unsigned int sticky:1;
	unsigned int use_insert_after:1;
	struct _LINE_REC *insert_after;
} GUI_WINDOW_REC;
//fe-text/textbuffer.h
typedef struct _LINE_REC LINE_REC;
//fe-text/textbuffer-view.h (not included in irssi-dev ?!?)
/* Set a bookmark in view */
typedef struct _TEXT_BUFFER_VIEW_REC TEXT_BUFFER_VIEW_REC;
void textbuffer_view_set_bookmark(TEXT_BUFFER_VIEW_REC *view,                      
		const char *name, LINE_REC *line);                               
/* Set a bookmark in view to the bottom line */                                    
void textbuffer_view_set_bookmark_bottom(TEXT_BUFFER_VIEW_REC *view,               
		const char *name);                                            
/* Return the line for bookmark */                                                 
LINE_REC *textbuffer_view_get_bookmark(TEXT_BUFFER_VIEW_REC *view,                 
		const char *name);
void textbuffer_view_remove_line(TEXT_BUFFER_VIEW_REC*, LINE_REC*);

#include "quassel-irssi.h"

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

void quassel_fewindow_init(void) {
	signal_add("window changed", (SIGNAL_FUNC) sig_window_changed);
}

void quassel_fewindow_deinit(void) {
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
}


