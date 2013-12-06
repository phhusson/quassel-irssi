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
typedef struct {
	int level;
	time_t time;
} LINE_INFO_REC;
typedef struct _LINE_REC {
	/* Text in the line. \0 means that the next char will be a
	   color or command.

	   If the 7th bit is set, the lowest 7 bits are the command
	   (see LINE_CMD_xxxx). Otherwise they specify a color change:

	   Bit:
            5 - Setting a background color
            4 - Use "default terminal color"
            0-3 - Color

	   DO NOT ADD BLACK WITH \0\0 - this will break things. Use
	   LINE_CMD_COLOR0 instead. */
	struct _LINE_REC *prev, *next;

	unsigned char *text;
        LINE_INFO_REC info;
} LINE_REC;
typedef struct _TEXT_CHUNK_REC TEXT_CHUNK_REC;
typedef struct {
	GSList *text_chunks;
        LINE_REC *first_line;
        int lines_count;

	LINE_REC *cur_line;
	TEXT_CHUNK_REC *cur_text;

	unsigned int last_eol:1;
	int last_fg;
	int last_bg;
	int last_flags;
} TEXT_BUFFER_REC;
typedef struct _LINE_REC LINE_REC;
LINE_REC *textbuffer_insert(TEXT_BUFFER_REC *buffer, LINE_REC *insert_after,
			    const unsigned char *data, int len,
			    LINE_INFO_REC *info);
LINE_REC *textbuffer_append(TEXT_BUFFER_REC *buffer,
			    const unsigned char *data, int len,
			    LINE_INFO_REC *info);

//fe-text/textbuffer-view.h
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
void textbuffer_view_insert_line(TEXT_BUFFER_VIEW_REC *view, LINE_REC *line);
void textbuffer_view_remove_line(TEXT_BUFFER_VIEW_REC*, LINE_REC*);
struct _TEXT_BUFFER_VIEW_REC {
	TEXT_BUFFER_REC *buffer;
	GSList *siblings; /* other views that use the same buffer */

        struct _TERM_WINDOW *window;
	int width, height;

	int default_indent;
        void* default_indent_func;
	unsigned int longword_noindent:1;
	unsigned int scroll:1; /* scroll down automatically when at bottom */
	unsigned int utf8:1; /* use UTF8 in this view */

	struct _TEXT_BUFFER_CACHE_REC *cache;
	int ypos; /* cursor position - visible area is 0..height-1 */

	LINE_REC *startline; /* line at the top of the screen */
	int subline; /* number of "real lines" to skip from `startline' */

        /* marks the bottom of the text buffer */
	LINE_REC *bottom_startline;
	int bottom_subline;

	/* how many empty lines are in screen. a screenful when started
	   or used /CLEAR */
	int empty_linecount;
        /* window is at the bottom of the text buffer */
	unsigned int bottom:1;
        /* if !bottom - new text has been printed since we were at bottom */
	unsigned int more_text:1;
        /* Window needs a redraw */
	unsigned int dirty:1;

	/* Bookmarks to the lines in the buffer - removed automatically
	   when the line gets removed from buffer */
        GHashTable *bookmarks;
};

#include "quassel-irssi.h"

extern void quassel_mark_as_read(GIOChannel*, int);
extern void quassel_set_last_seen_msg(GIOChannel*, int, int);

static void quassel_chan_read(Quassel_CHANNEL_REC* chanrec) {
	GIOChannel *giochan = net_sendbuffer_handle(chanrec->server->handle);
	quassel_set_last_seen_msg(giochan, chanrec->buffer_id, chanrec->last_msg_id);
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

int quassel_find_buffer_id(char *name, uint32_t network);
void quassel_request_backlog(GIOChannel *h, int buffer, int first, int last, int limit, int additional);
void cmd_qbacklog(const char *arg) {
	(void)arg;
	int n = atoi(arg);
	Quassel_CHANNEL_REC* chanrec = window2chanrec(active_win);
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

extern void mainwindows_redraw(void);
void irssi_quassel_backlog(Quassel_SERVER_REC* server, int msg_id, int timestamp, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content) {
	(void) msg_id;
	(void) bufferid;
	(void) type;
	(void) flags;

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
	command_bind("qbacklog", NULL, (SIGNAL_FUNC) cmd_qbacklog);
}

void quassel_fewindow_deinit(void) {
	signal_remove("window changed", (SIGNAL_FUNC) sig_window_changed);
	command_unbind("qbacklog", (SIGNAL_FUNC) cmd_qbacklog);
}


