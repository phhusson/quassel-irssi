#ifndef CMDS_H
#define CMDS_H
#include "types.h"

void Login(GIOChannel *h, char *user, char *pass);
void HeartbeatReply(GIOChannel*);
void send_message(GIOChannel*, struct bufferinfo b, char *message);
void initRequest(GIOChannel*, char *val, char *arg);
void requestBacklog(GIOChannel*, int buffer, int first, int last, int limit, int additional);
#endif
