#ifndef QUASSEL_IRSSI_H
#define QUASSEL_IRSSI_H

#define STRUCT_SERVER_CONNECT_REC struct Quassel_SERVER_CONNECT_REC_s 
typedef struct Quassel_SERVER_CONNECT_REC_s {
#include <server-connect-rec.h>
} Quassel_SERVER_CONNECT_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
typedef struct Quassel_SERVER_REC_s {
#include <server-rec.h>
	char *msg;
	int size;
	int got;

	int ssl;
} Quassel_SERVER_REC;

#define STRUCT_CHANNEL_REC struct Quassel_CHANNEL_REC_s 
typedef struct Quassel_CHANNEL_REC_s {
#include <channel-rec.h>
	int buffer_id;
	int first_msg_id;
	int last_msg_id;
	int last_seen_msg_id;

	int got_backlog;
} Quassel_CHANNEL_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
#define STRUCT_QUERY_REC struct Quassel_QUERY_REC_s 
typedef struct Quassel_QUERY_REC_s {
#include <query-rec.h>
	int buffer_id;
} Quassel_QUERY_REC;

#define Quassel_PROTOCOL (chat_protocol_lookup("Quassel"))


//quassel-net.c
extern void irssi_handle_connected(Quassel_SERVER_REC* r);
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);
extern void quassel_irssi_channels_join(SERVER_REC *server, const char *data, int automatic);
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);

//quassel-fe-window.c
extern void quassel_irssi_check_read(Quassel_CHANNEL_REC* chanrec);
void quassel_fewindow_init(void);
void quassel_fewindow_deinit(void);

static inline char *channame(int net, char *buf) {
	char *ret = NULL;
	int len = asprintf(&ret, "%d-%s", net, buf);
	(void)len;
	return ret;
}

//quassel-msgs.c
extern void quassel_irssi_send_message(SERVER_REC *server, const char *target, const char *msg, int target_type);
extern void quassel_msgs_init(void);

//quassel-core.c
CHANNEL_REC *quassel_channel_create(SERVER_REC *server, const char *name, const char *visible_name, int automatic);

// lib
void quassel_request_backlog(GIOChannel *h, int buffer, int first, int last, int limit, int additional);
#endif /* QUASSEL_IRSSI_H */
