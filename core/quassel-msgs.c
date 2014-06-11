/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <module.h>
#include <channels-setup.h>
#include <channels.h>
#include <chat-protocols.h>
#include <chatnets.h>
#include <commands.h>
#include <levels.h>
#include <net-sendbuffer.h>
#include <network.h>
#include <nicklist.h>
#include <printtext.h>
#include <queries.h>
#include <recode.h>
#include <servers-setup.h>
#include <servers.h>
#include <settings.h>
#include <signals.h>

//fe-common/core
#include <fe-windows.h>
//fe-common/
#include <irc/module-formats.h>

#include "quassel-irssi.h"

static void quassel_irssi_join2(void* arg, char* _chan, char* nick, char* host,
		char* mode) {
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(arg), _chan);
	if(!chan_rec)
		return;

	NICK_REC* rec = g_new0(NICK_REC, 1);
	rec->nick = g_strdup(nick);
	rec->host = g_strdup(host);
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

void quassel_irssi_join(void* arg, int network,
		char *chan, char* nick,
		char* mode) {
	char *_chan = channame(network, chan);
	quassel_irssi_join2(arg, _chan, nick, NULL, mode);
	free(_chan);
}

void quassel_irssi_joined(void* arg, int network, char *chan) {
	char *_chan = channame(network, chan);
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(arg), _chan);
	if(!chan_rec) goto end;
	chan_rec->joined = 1;
	signal_emit("message join", 4, SERVER(arg), _chan, SERVER(arg)->nick, "quassel@irssi");
	signal_emit("channel joined", 1, chan_rec);
end:
	free(_chan);
}

static void print_ctcpaction(Quassel_SERVER_REC *server, const char* content, const char* nick, const char* address, const char* target) {
	/* channel action */
	(void) content;
	(void) address;
	if(strcmp(active_win->active->visible_name, target) == 0) {
		/* message to active channel in window */
		printformat(server, target, MSGLEVEL_ACTIONS|MSGLEVEL_PUBLIC,
				IRCTXT_ACTION_PUBLIC, nick, content);
	} else {
		/* message to not existing/active channel, or to @/+ */
		printformat(server, target, MSGLEVEL_ACTIONS|MSGLEVEL_PUBLIC,
				IRCTXT_ACTION_PUBLIC_CHANNEL,
				nick, target, content);
	}
}

void quassel_irssi_handle(void* arg, int msg_id, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content) {
	(void)flags;
	Quassel_SERVER_REC *r = (Quassel_SERVER_REC*) arg;
	char *chan = channame(network, buffer_id);
	char *nick = strdup(sender);
	char *address;
	if( (address=index(nick, '!')) != NULL) {
		*address = 0;
		address++;
	} else
		address=strdup("");
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
		chanrec = (Quassel_CHANNEL_REC*) quassel_channel_create(SERVER(r), chan, chan, 1);
	if(chanrec->first_msg_id == -1)
		chanrec->first_msg_id = msg_id;
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
	} else if(type == 0x02) {
		if(strcmp(nick, buffer_id) == 0 || strcmp(buffer_id, "")==0 ) {
			printformat(SERVER(r), nick, MSGLEVEL_NOTICES,
					IRCTXT_NOTICE_PRIVATE, nick, address, content);
			signal_emit("message priv_notice", 5,
					r, content, nick, "", nick);
		} else {
			printformat(SERVER(r), chan, MSGLEVEL_NOTICES,
					IRCTXT_NOTICE_PUBLIC, nick, chan, content);
			signal_emit("message notice", 5,
					r, content, nick, "", chan);
		}
	} else if(type == 0x04) {
		print_ctcpaction(r, content, nick, address, chan);
		signal_emit("message action", 5,
				r, content, nick, "", chan);
	} else if(type == 0x08) {
		//Nick
		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chanrec, nick);

		//nick already renamed
		if(!nick_rec)
			goto end;
		nicklist_rename(SERVER(r), nick, content);
		signal_emit("message nick", 4, SERVER(r), content, nick, address);
	} else if(type == 0x20) {
		//Join
		quassel_irssi_join2(r, chan, nick, address, "");

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
	} else if(type == 0x100) {
		//Kick
		char *kicked = content;
		char *msg = index(content, ' ');
		if(!msg) {
			msg = "";
		} else {
			msg[0] = 0;
			msg++;
		}
		signal_emit("message kick", 6, SERVER(r), chan, kicked, nick, address, msg);
	} else if(type == 0x4000) {
		//Topic
		//Formatted string... better use IrcChannel::setTopic
	} else /*if(type == 0x400) */{
		//TODO:
		/*
		   Mode(0x00010),
		   Kill(0x00200),
		   Server(0x00400),
		   Info(0x00800),
		   Error(0x01000),
		   DayChange(0x02000),
		   NetsplitJoin(0x08000),
		   NetsplitQuit(0x10000),
		   Invite(0x20000);
		 */
		char *str = NULL;
		char *type_str = NULL;
		type_str = type == 0x10 ? "Mode" :
			type == 0x200 ? "Kill" :
			type == 0x400 ? "Server" :
			type == 0x800 ? "Info" :
			type == 0x1000 ? "Error" :
			type == 0x2000 ? "DayChange" :
			type == 0x8000 ? "NetsplitJoin" :
			type == 0x10000 ? "NetsplitQuit" :
			type == 0x20000 ? "Invite" : "Unknown type";
		int len = asprintf(&str, "%s:%s", type_str, content);
		(void)len;
		chanrec->buffer_id = bufferid;
		//Show it as a notice...
		printformat(SERVER(r), chan, MSGLEVEL_NOTICES,
				IRCTXT_NOTICE_PUBLIC, sender, chan, str);
		free(str);
	}

	quassel_irssi_check_read(chanrec);

end:
	free(chan);
	free(nick);
}

void quassel_irssi_send_message(SERVER_REC *server, const char *target,
			 const char *msg, int target_type) {
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(server, target);
	(void) target_type;
	
	if(chan_rec && chan_rec->buffer_id != -1) {
		quassel_send_message(net_sendbuffer_handle(server->handle), chan_rec->buffer_id, msg);
	} else {
		char chan[256];
		int network = 0;
		if(sscanf(target, "%d-%255s", &network, chan) != 2)
			quassel_send_message(net_sendbuffer_handle(server->handle), quassel_find_buffer_id(target, -1), msg);
		else
			quassel_send_message(net_sendbuffer_handle(server->handle), quassel_find_buffer_id(chan, network), msg);
	}
}

static void sig_own_msg(SERVER_REC *server, const char *msg, const char *channel,
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
		chanrec = quassel_channel_create(server, channel, channel, 1);
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
	chanrec->topic_by = g_strdup(setby);
	
	chanrec->topic_time = settime;

	signal_emit("channel topic changed", 1, chanrec);
}

void quassel_irssi_topic(void* arg, int network, char *chan, char *topic) {
	Quassel_SERVER_REC *server = (Quassel_SERVER_REC*)arg;
	char *s = channame(network, chan);
	channel_change_topic(SERVER(server), s, topic, "", time(NULL));
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*)channel_find(SERVER(server), s);
	if(!chanrec)
		return;
	if(chanrec->buffer_id == -1)
		chanrec->buffer_id = quassel_find_buffer_id(chan, network);
	if(chanrec->joined)
		signal_emit("message topic", 5, server, s, topic, "someone", "");
	free(s);
}

void quassel_msgs_init(void) {
	signal_add_first("message own_public", (SIGNAL_FUNC) sig_own_msg);
	signal_add_first("message own_private", (SIGNAL_FUNC) sig_own_msg);
	theme_register(fecommon_irc_formats);
}

void quassel_msgs_deinit(void) {
	signal_remove("message own_public", (SIGNAL_FUNC) sig_own_msg);
	signal_remove("message own_private", (SIGNAL_FUNC) sig_own_msg);
}
