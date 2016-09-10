#ifndef QUASSEL_IRSSI_H
#define QUASSEL_IRSSI_H

#include "export.h"

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
	int init_last_seen_msg_id;
	int last_seen_msg_id;

	int got_backlog;
} Quassel_CHANNEL_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
#define STRUCT_QUERY_REC struct Quassel_QUERY_REC_s 
typedef struct Quassel_QUERY_REC_s {
#include <query-rec.h>
	int buffer_id;
} Quassel_QUERY_REC;

typedef struct Quassel_CHATNET_REC_s {
#include <chatnet-rec.h>
	int legacy;
	int load_backlog;
	int backlog_additional;
} Quassel_CHATNET_REC;

#define Quassel_PROTOCOL (chat_protocol_lookup("Quassel"))


//quassel-net.c
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);
extern void quassel_irssi_channels_join(SERVER_REC *server, const char *data, int automatic);
extern void quassel_net_init(CHAT_PROTOCOL_REC* rec);

//quassel-fe-window.c
extern void quassel_irssi_check_read(Quassel_CHANNEL_REC* chanrec);
void quassel_fewindow_init(void);
void quassel_fewindow_deinit(void);

static inline char *channame(int net, const char *buf) {
	char *ret = NULL;
	int len = asprintf(&ret, "%d-%s", net, buf);
	(void)len;
	return ret;
}

//quassel-msgs.c
extern void quassel_irssi_send_message(SERVER_REC *arg, const char *target, const char *msg, int target_type);
extern void quassel_msgs_init(void);
extern void quassel_msgs_deinit(void);

//quassel-core.c
CHANNEL_REC *quassel_channel_create(SERVER_REC *server, const char *name, const char *visible_name, int automatic);
#ifdef IRSSI_ABI_VERSION
void quassel_core_abicheck(int *version);
#endif

//quassel-cmds.c
extern void quassel_cmds_init(void);
extern void quassel_cmds_deinit(void);

//quassel-fe-level.c
void quassel_felevel_init(void);
void quassel_felevel_deinit(void);

//quassel-cfg.c
void quassel_cfg_init(void);
void quassel_cfg_deinit(void);

//Used by irssi itself
extern void quassel_core_init(void);
extern void quassel_core_deinit(void);

#endif /* QUASSEL_IRSSI_H */
