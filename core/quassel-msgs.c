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
#include <signals.h>

//fe-common/core
#include <fe-windows.h>

#include "quassel-irssi.h"

void quassel_irssi_join2(void* arg, char* _chan, char* nick,
		char* mode) {
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(arg), _chan);
	if(!chan_rec)
		return;

	NICK_REC* rec = g_new0(NICK_REC, 1);
	rec->nick = g_strdup(nick);
	for(int i=0; mode[i]; ++i) {
		if(mode[i] == 'o')
			rec->op = 1;
		if(mode[i] == 'v')
			rec->voice = 1;

		if(rec->op)
			rec->prefixes[0] = '@';
		else if(rec->voice)
			rec->prefixes[0] = '+';
	}
	nicklist_insert(CHANNEL(chan_rec), rec);
}

void quassel_irssi_join(void* arg, char* network,
		char *chan, char* nick,
		char* mode) {
	char *_chan = channame(atoi(network), chan);
	quassel_irssi_join2(arg, _chan, nick, mode);
	free(_chan);
}

void quassel_irssi_joined(void* arg, char* network, char *chan) {
	char *_chan = channame(atoi(network), chan);
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(arg), _chan);
	if(!chan_rec) goto end;
	signal_emit("channel joined", 1, chan_rec);
end:
	free(_chan);
}

void irssi_quassel_handle(Quassel_SERVER_REC* r, int msg_id, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content) {
	(void)flags;
	char *chan = channame(network, buffer_id);
	char *nick = strdup(sender);
	char *address;
	if( (address=index(nick, '!')) != NULL)
		*address = 0;
	address++;
	//Text message
	/*
	   Plain(0x00001),                                                            
	   Notice(0x00002),                                                           
	   Action(0x00004),                                                           
	   Nick(0x00008),                                                             
	   Mode(0x00010),                                                             
	   Join(0x00020),                                                             
	   Part(0x00040),                                                             
	   Quit(0x00080),                                                             
	   Kick(0x00100),                                                             
	   Kill(0x00200),                                                             
	   Server(0x00400),                                                           
	   Info(0x00800),                                                             
	   Error(0x01000),                                                            
	   DayChange(0x02000),                                                        
	   Topic(0x04000),                                                            
	   NetsplitJoin(0x08000),                                                     
	   NetsplitQuit(0x10000),                                                     
	   Invite(0x20000);
	*/
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*) channel_find(SERVER(r), chan);
	if(!chanrec)
		chanrec = (Quassel_CHANNEL_REC*) quassel_channel_create(SERVER(r), chan, chan, 0);
	chanrec->last_msg_id = msg_id;
	if(type == 1) {
		char *recoded;
		chanrec->buffer_id = bufferid;
		recoded = recode_in(SERVER(r), content, chan);
		if(strcmp(sender, SERVER(r)->nick) == 0) {
			signal_emit("message own_public", 4, r, recoded, chan, NULL);
		} else {
			signal_emit("message public", 5,
				r, recoded, nick, "coin", chan);
		}
		g_free(recoded);
	} else if(type == 0x08) {
		//Nick
		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chanrec, nick);

		//nick already renamed
		if(!nick_rec)
			goto end;
		nicklist_rename(SERVER(r), nick, content);
		signal_emit("message nick", 4, SERVER(r), content, nick, address);
	} else if(type == 0x10) {
		//Change mode
	} else if(type == 0x20) {
		//Join
		quassel_irssi_join2(r, chan, nick, "");

		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chanrec, nick);
		signal_emit("nicklist new", 2, chanrec, nick_rec);
		signal_emit("message join", 4, SERVER(r), chan, nick, address);
	} else if(type == 0x40) {
		//part
		signal_emit("message part", 5, SERVER(r), chan, nick, address, content);

		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chanrec, nick);
		nicklist_remove((CHANNEL_REC*)chanrec, nick_rec);
	} else if(type == 0x80) {
		//Quit
		signal_emit("message quit", 4, SERVER(r), nick, address, content);

		GSList *nicks = nicklist_get_same(SERVER(r), nick);
		for (GSList *tmp = nicks; tmp != NULL; tmp = tmp->next->next) {
			CHANNEL_REC* channel = tmp->data;
			NICK_REC* nickrec = tmp->next->data;
			nicklist_remove(CHANNEL(channel), nickrec);
		}
		g_slist_free(nicks);
	} else /*if(type == 0x400) */{
		char *str = NULL;
		asprintf(&str, "%d:%s:%s\n", type, sender, content);
		chanrec->buffer_id = bufferid;
		signal_emit("message public", 5,
				r, str, "server", "coin", chan);
	}

	quassel_irssi_check_read(chanrec);

end:
	free(chan);
	free(nick);
}

void irssi_send_message(GIOChannel* h, int buffer, const char *message);
void quassel_irssi_send_message(SERVER_REC *server, const char *target,
			 const char *msg, int target_type) {
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(server, target);
	(void) target_type;
	if(!chan_rec)
		return;
	
	irssi_send_message(net_sendbuffer_handle(server->handle), chan_rec->buffer_id, msg);
}

static void sig_own_public(SERVER_REC *server, const char *msg, const char *channel,
	const char *orig_target) {
	(void) msg;
	(void) channel;
	if(!PROTO_CHECK_CAST(SERVER(server), Quassel_SERVER_REC, chat_type, "Quassel"))
		return;
	if(orig_target != NULL) {
		//own_public message sent from /msg command
		signal_stop();
		return;
	}
}

static void channel_change_topic(SERVER_REC *server, const char *channel,
				 const char *topic, const char *setby,
				 time_t settime)
{
	CHANNEL_REC *chanrec;
	char *recoded = NULL;
	
	chanrec = channel_find(SERVER(server), channel);
	if (chanrec == NULL) {
		chanrec = quassel_channel_create(server, channel, channel, 0);
	}
	/* the topic may be send out encoded, so we need to 
	   recode it back or /topic <tab> will not work properly */
	recoded = recode_in(SERVER(server), topic, channel);
	if (topic != NULL) {
		g_free_not_null(chanrec->topic);
		chanrec->topic = recoded == NULL ? NULL : g_strdup(recoded);
	}
	g_free(recoded);

	g_free_not_null(chanrec->topic_by);
	g_free_not_null(chanrec->topic_by);
	chanrec->topic_by = g_strdup(setby);
	
	chanrec->topic_time = settime;

	signal_emit("channel topic changed", 1, chanrec);
}

extern int quassel_find_buffer_id(char *name, int network);
void quassel_irssi_topic(SERVER_REC* server, char* network, char *chan, char *topic) {
	char *s = channame(atoi(network), chan);
	channel_change_topic(server, s, topic, "", time(NULL));
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*)channel_find(SERVER(server), s);
	free(s);
	if(!chanrec)
		return;
	if(chanrec->buffer_id == -1)
		chanrec->buffer_id = quassel_find_buffer_id(chan, atoi(network));
}

void quassel_msgs_init(void) {
	signal_add_first("message own_public", (SIGNAL_FUNC) sig_own_public);
}
