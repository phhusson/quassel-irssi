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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include "quasselc.h"
#include "export.h"
#include "connector.h"

#if 0
#define dprintf(x...) printf(x)
#else
static void useless_printf(const char *str, ...) {
	(void)str;
}
#define dprintf(x...) useless_printf(x)
#endif

static int highlight=0;

struct buffer {
	struct bufferinfo i;
	int lastseen;
	int marker;
	int displayed;
};
static struct buffer *buffers;
static int n_buffers;

static void new_buffer(int bufid, int netid, short type, int group, char *name) {
	if(bufid>=n_buffers) {
		buffers=realloc(buffers, sizeof(struct buffer)*(bufid+1));
		int i;
		for(i=n_buffers;i<=bufid;++i)
			buffers[i].i.id=-1;
		n_buffers=bufid+1;
	}
	if(buffers[bufid].i.id == (uint32_t)bufid)
		return;
	buffers[bufid].i.network=netid;
	buffers[bufid].i.id=bufid;
	buffers[bufid].i.type=type;
	buffers[bufid].i.group=group;
	buffers[bufid].i.name=name;
	buffers[bufid].marker=0;
	buffers[bufid].lastseen=0;
	buffers[bufid].displayed=1;
}

int quassel_buffer_displayed(uint32_t bufid);
int quassel_buffer_displayed(uint32_t bufid) {
	if(buffers[bufid].i.id == bufid)
		return buffers[bufid].displayed;
	return 1;
}

static void may_new_buffer(int bufid, int netid, short type, int group, char* name) {
	if(bufid>=n_buffers || (bufid>=0 && buffers[bufid].i.id != (uint32_t)-1))
		new_buffer(bufid, netid, type, group, name);
}

int quassel_find_buffer_id(const char *name, uint32_t network) {
	int i;
	for(i=0;i<n_buffers;++i) {
		if(buffers[i].i.id==(uint32_t)-1)
			continue;
		if(strcasecmp(buffers[i].i.name, name)==0 && (network == (uint32_t)-1 || buffers[i].i.network == network))
			return i;
	}
	return -1;
}

void quassel_send_message(GIOChannel* h, int buffer, const char *message) {
	if(buffer==-1) {
		fprintf(stderr, "Sending a message to unknown buffer... Case not handled\n");
		exit(1);
	}
	send_message(h, buffers[buffer].i, message);
}

void handle_message(struct message m, void *arg) {
	may_new_buffer(m.buffer.id, m.buffer.network, m.buffer.type, m.buffer.group, strdup(m.buffer.name));
	quassel_irssi_handle(arg, m.id, m.buffer.id, m.buffer.network, m.buffer.name, m.sender, m.type, m.flags, m.content);
}

void handle_backlog(struct message m, void *arg) {
	quassel_irssi_backlog(arg, m.id, m.timestamp, m.buffer.id, m.buffer.network, m.buffer.name, m.sender, m.type, m.flags, m.content);
}

void quassel_irssi_request_backlogs(void* h, int all, int additional) {
	for(int i=0;i<n_buffers;++i) {
		if(buffers[i].i.id==(uint32_t)-1)
			continue;
		if(!all && !buffers[i].displayed)
			continue;
		quassel_request_backlog(h, buffers[i].i.id,
				buffers[i].lastseen, -1, 150, additional);
	}
}

static int initBufferStatus = 0;
void handle_sync(void* irssi_arg, object_t o, function_t f, ...) {
	(void) o;
	va_list ap;
	char *fnc=NULL;
	char *net,*chan,*nick,*name;
	int netid,bufid,msgid;
	va_start(ap, f);
	switch(f) {
		/* BufferSyncer */
		case Create:
			bufid=va_arg(ap, int);
			netid=va_arg(ap, int);
			int type=va_arg(ap, int);
			int group=va_arg(ap, int);
			name=va_arg(ap, char*);
			dprintf("CreateBuffer(%d, %d, %d, %d, %s)\n", netid, bufid, type, group, name);
			new_buffer(bufid, netid, type, group, name);
			break;
		case MarkBufferAsRead:
			highlight=0;
			if(!fnc)
				fnc="MarkBufferAsRead";
		/* Falls through */
		case Displayed:
			if(!fnc)
				fnc="BufferDisplayed";
			bufid=va_arg(ap, int);
			dprintf("%s(%d)\n", fnc, bufid);
			//buffers[bufid].displayed=1;
			break;
		case Removed:
			if(!fnc)
				fnc="BufferRemoved";
		/* Falls through */
		case TempRemoved:
			if(!fnc)
				fnc="BufferTempRemoved";
			bufid=va_arg(ap, int);
			if(bufid>=n_buffers)
				break;
			buffers[bufid].displayed=0;
			if(buffers[bufid].i.id == (uint32_t)-1)
				break;
			quassel_irssi_hide(irssi_arg, buffers[bufid].i.network, buffers[bufid].i.name);
			dprintf("%s(%d)\n", fnc, bufid);
			break;
		case SetLastSeenMsg:
			if(!fnc)
				fnc="SetLastSeenMsg";
			bufid=va_arg(ap, int);
			msgid=va_arg(ap, int);
			buffers[bufid].lastseen=msgid;
			quassel_irssi_set_last_seen_msg(irssi_arg, bufid, msgid);
			dprintf("%s(%d, %d)\n", fnc, bufid, msgid);
			break;
		case SetMarkerLine:
			if(!fnc)
				fnc="SetMarkerLine";
			bufid=va_arg(ap, int);
			msgid=va_arg(ap, int);
			buffers[bufid].marker=msgid;
			dprintf("%s(%d, %d)\n", fnc, bufid, msgid);
			break;
		case DoneBuffersInit: {
			int info = va_arg(ap, int);
			int o = initBufferStatus;
			initBufferStatus |= info;
			//Got both BufferViewConfig and BufferSyncer
			if(initBufferStatus == 3 && initBufferStatus != o)
				quassel_irssi_ready(irssi_arg);
			} break;
		/* IrcChannel */
		case JoinIrcUsers:
			net=va_arg(ap, char*);
			chan=va_arg(ap, char*);
			int size=va_arg(ap, int);
			char **users=va_arg(ap, char**);
			char **modes=va_arg(ap, char**);
			if(size==0)
				break;
			if(size>1) {
				dprintf("Too many users joined\n");
				break;
			}
			dprintf("JoinIrcUser(%s, %s, %s, %s)\n", net, chan, users[0], modes[0]);
			break;
		case AddUserMode:
			if(!fnc)
				fnc="AddUserMode";
		/* Falls through */
		case RemoveUserMode:
			if(!fnc)
				fnc="RemoveUserMode";
			net=va_arg(ap, char*);
			chan=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			char *mode=va_arg(ap, char*);
			dprintf("%s(%s, %s, %s, %s)\n", fnc, net, chan, nick, mode);
			break;
		/* IrcUser */
		case SetNick2:
			if(!fnc)
				fnc="SetNick";
		/* Falls through */
		case Quit:
			if(!fnc)
				fnc="Quit";
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			dprintf("%s(%s, %s)\n", fnc, net, nick);
			break;
		case SetNick:
			if(!fnc)
				fnc="SetNick";
		/* Falls through */
		case SetServer:
			if(!fnc)
				fnc="SetServer";
		/* Falls through */
		case SetRealName:
			if(!fnc)
				fnc="SetRealName";
		/* Falls through */
		case PartChannel:
			if(!fnc)
				fnc="PartChannel";
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			char *str=va_arg(ap, char*);
			dprintf("%s(%s, %s, %s)\n", fnc, net, nick, str);
			break;
		case SetAway:
			net=va_arg(ap, char*);
			nick=va_arg(ap, char*);
			int away=va_arg(ap, int);
			dprintf("setAway(%s, %s, %s)\n", net, nick, away ? "away" : "present");
			break;
		/* Network */
		case AddIrcUser:
			net=va_arg(ap, char*);
			char *name=va_arg(ap, char*);
			dprintf("AddIrcUser(%s, %s)\n", net, name);
			break;
		case SetLatency:
			net=va_arg(ap, char*);
			int latency=va_arg(ap, int);
			dprintf("SetLatency(%s, %d)\n", net, latency);
			break;
		case MyNick:
			net = va_arg(ap, char*);
			nick = va_arg(ap, char*);
			dprintf("MyNick(%s, %d)\n", net, nick);
			break;
	}
	va_end(ap);
}

void handle_event(void* arg, GIOChannel *h, event_t t, ...) {
	va_list ap;
	va_start(ap, t);

	int net;
	char *chan;
	char *topic;
	char *nick;
	char *mode;
	switch(t) {
		case ClientInitAck:
			quassel_irssi_init_ack(arg);
			break;

		case SessionInit:
			initBufferStatus = 0;
			//Get buffers' display status
			initRequest(h, "BufferViewConfig", "0");
			//Retrieve marker lines and last seen msgs
			initRequest(h, "BufferSyncer", "");
			quassel_irssi_handle_connected(arg);
			break;

		case TopicChange:
			net = va_arg(ap, int);
			chan = va_arg(ap, char*);
			topic = va_arg(ap, char*);
			quassel_irssi_topic(arg, net, chan, topic);
			break;

		case ChanPreAddUser:
			net = va_arg(ap, int);
			chan = va_arg(ap, char*);
			nick = va_arg(ap, char*);
			mode = va_arg(ap, char*);
			quassel_irssi_join(arg, net, chan, nick, mode);
			break;

		case ChanReady:
			net = va_arg(ap, int);
			chan = va_arg(ap, char*);
			quassel_irssi_joined(arg, net, chan);
			break;
		case ClientLoginReject:
			quassel_irssi_init_nack(arg);
			break;
	}
	va_end(ap);
}
