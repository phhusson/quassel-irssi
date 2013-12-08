#ifndef QUASSEL_EXPORT_H
#define QUASSEL_EXPORT_H

/* quassel plugin */
extern void quassel_irssi_init_ack(void *server);
extern void quassel_irssi_set_last_seen_msg(void *arg, int buffer_id, int msgid);
extern void irssi_quassel_backlog(void* server, int msg_id, int timestamp, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content);
extern void quassel_irssi_join(void* arg, char* network, char *chan, char* nick, char* mode);
extern void quassel_irssi_joined(void* arg, char* network, char *chan);
extern void irssi_quassel_handle(void* r, int msg_id, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content);
extern void quassel_irssi_topic(void* server, char* network, char *chan, char *topic);
extern void irssi_handle_connected(void*);

/* quassel lib */
extern void irssi_send_message(GIOChannel* h, int buffer, const char *message);
extern void quassel_mark_as_read(GIOChannel* h, int buffer_id);
extern void quassel_set_last_seen_msg(GIOChannel* h, int buffer_id, int msg_id);
extern void quassel_login(GIOChannel* h, char *user, char *pass);
extern void quassel_init_packet(GIOChannel* h, int ssl);
extern int quassel_parse_message(GIOChannel* h, char *buf, void *arg);
#endif
