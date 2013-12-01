#ifndef QUASSEL_IRSSI_H
#define QUASSEL_IRSSI_H

#define STRUCT_SERVER_CONNECT_REC struct Quassel_SERVER_CONNECT_REC_s 
typedef struct Quassel_SERVER_CONNECT_REC_s {
#include <irssi/src/core/server-connect-rec.h>
} Quassel_SERVER_CONNECT_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
typedef struct Quassel_SERVER_REC_s {
#include <irssi/src/core/server-rec.h>
	char *msg;
	int size;
	int got;
} Quassel_SERVER_REC;

#define STRUCT_CHANNEL_REC struct Quassel_CHANNEL_REC_s 
typedef struct Quassel_CHANNEL_REC_s {
#include <irssi/src/core/channel-rec.h>
	int buffer_id;
	int last_msg_id;
} Quassel_CHANNEL_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
#define STRUCT_QUERY_REC struct Quassel_QUERY_REC_s 
typedef struct Quassel_QUERY_REC_s {
#include <irssi/src/core/query-rec.h>
	int buffer_id;
} Quassel_QUERY_REC;

#define Quassel_PROTOCOL (chat_protocol_lookup("Quassel"))


//quassel-net.c
extern void irssi_handle_connected(Quassel_SERVER_REC* r);
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);
extern void quassel_irssi_channels_join(SERVER_REC *server, const char *data, int automatic);
extern void quassel_irssi_send_message(SERVER_REC *server, const char *target, const char *msg, int target_type);
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);

//quassel-fe-window.c
extern void quassel_irssi_check_read(Quassel_CHANNEL_REC* chanrec);
void quassel_fewindow_init(void);
void quassel_fewindow_deinit(void);
#endif /* QUASSEL_IRSSI_H */
