#ifndef BOT_H
#define BOT_H
#include "types.h"
#include <stdarg.h>

void handle_message(struct message m, void*);
void handle_backlog(struct message m);
void handle_fd(int fd);

typedef enum {
	BufferSyncer,
	IrcChannel,
	IrcUser,
	Network,
} object_t;

typedef enum {
	/* BufferSyncer */
	Create,			//(int bufferid, int network, char *name);
	TempRemoved,		//(int bufferid);
	Removed,		//(int bufferid);
	Displayed,		//(int bufferid);
	MarkBufferAsRead,	//(int bufferid);
	SetLastSeenMsg,		//(int bufferid, int messageid);
	SetMarkerLine,		//(int bufferid, int messageid);
	/* IrcChannel */
	JoinIrcUsers,		//(char *network_number, char *channel, int size, char **users, char **modes);
	AddUserMode,		//(char *network_number, char *channel, char *user, char *mode);
	RemoveUserMode,		//(char *network_number, char *channel, char *user, char *mode);
	/* IrcUser */
	Quit,			//(char *network_number, char *nick);
	SetServer,		//(char *network_number, char *nick, char *server);
	SetRealName,		//(char *network_number, char *nick, char *realname);
	SetAway,		//(char *network_number, char *nick, char away); //=1 if away, 0 else
	SetNick2,		//(char *network_number, char *nick); //Only new nick available, not old one
	PartChannel,		//(char *network_number, char *nick, char *channel);
	SetNick,		//(char *network_number, char *old_nick, char *new_nick);
	/* Network */
	AddIrcUser,		//(char *network_number, char *fullname);
	SetLatency,		//(char *network_number, int latency); //unit ?
} function_t;
void handle_sync(object_t o, function_t f, ...);
#endif
