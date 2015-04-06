#ifndef _CONNECTOR_H
#define _CONNECTOR_H

/* quassel plugin */
extern void quassel_irssi_init_ack(void *server);
extern void quassel_irssi_set_last_seen_msg(void *arg, int buffer_id, int msgid);
extern void quassel_irssi_backlog(void* server, int msg_id, int timestamp, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content);
extern void quassel_irssi_join(void* arg, int network, char *chan, char* nick, char* mode);
extern void quassel_irssi_joined(void* arg, int network, char *chan);
extern void quassel_irssi_handle(void* r, int msg_id, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content);
extern void quassel_irssi_topic(void* server, int network, char *chan, char *topic);
extern void quassel_irssi_handle_connected(void*);
extern void quassel_irssi_hide(void* arg, int net, const char* chan);
extern void quassel_irssi_ready(void *arg);
extern void quassel_irssi_init_ack(void* arg);
extern void quassel_irssi_init_nack(void* arg);
extern void quassel_irssi_request_backlogs(void* h, int all, int additional);
#endif /* _CONNECTOR_H */
