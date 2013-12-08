#ifndef IRSSI_GUI_H
#define IRSS_GUI_H
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

#endif /* IRSSI_GUI_H */
