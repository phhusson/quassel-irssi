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

static CHATNET_REC *create_chatnet(void) {
    return g_new0(CHATNET_REC, 1);
}

char *channame(int net, char *buf) {
	char *ret = NULL;
	asprintf(&ret, "%d-%s", net, buf);
	return ret;
}

static SERVER_SETUP_REC *create_server_setup(void) {
    return g_new0(SERVER_SETUP_REC, 1);
}

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
} Quassel_CHANNEL_REC;

#define STRUCT_SERVER_REC struct Quassel_SERVER_REC_s 
#define STRUCT_QUERY_REC struct Quassel_QUERY_REC_s 
typedef struct Quassel_QUERY_REC_s {
#include <irssi/src/core/query-rec.h>
	int buffer_id;
} Quassel_QUERY_REC;

#define Quassel_PROTOCOL (chat_protocol_lookup("Quassel"))

static SERVER_CONNECT_REC * create_server_connect(void) {
    return (SERVER_CONNECT_REC *) g_new0(Quassel_SERVER_CONNECT_REC, 1);
}

static CHANNEL_SETUP_REC *create_channel_setup(void) {
    return g_new0(CHANNEL_SETUP_REC, 1);
}

static void destroy_server_connect(SERVER_CONNECT_REC *conn) {
	(void) conn;
}

CHANNEL_REC *quassel_channel_create(SERVER_REC *server, const char *name,
				    const char *visible_name, int automatic)
{
	Quassel_CHANNEL_REC *rec;

	rec = g_new0(Quassel_CHANNEL_REC, 1);

	channel_init((CHANNEL_REC *) rec, (SERVER_REC *) server,
		     name, visible_name, automatic);
	rec->buffer_id = -1;

	return (CHANNEL_REC*)rec;
}

static void quassel_channels_join(SERVER_REC *server, const char *data,
			      int automatic) {
	quassel_channel_create(server, data, data, automatic);
}


void quassel_server_connect(SERVER_REC *server) {
	if (!server_start_connect(server)) {
		server_connect_unref(server->connrec);
		g_free(server);
	}
}

QUERY_REC* quassel_query_create(const char *server_tag, const char* nick, int automatic) {
	Quassel_QUERY_REC *rec = g_new0(Quassel_QUERY_REC, 1);
	rec->chat_type = Quassel_PROTOCOL;
	rec->name = g_strdup(nick);
	rec->server_tag = g_strdup(server_tag);
	query_init((QUERY_REC*)rec, automatic);

	return (QUERY_REC*)rec;
}

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

extern void quassel_init_packet(GIOChannel*);
extern void quassel_parse_message(GIOChannel*, char*, void*);

static void quassel_parse_incoming(Quassel_SERVER_REC* r) {
	GIOChannel *chan = net_sendbuffer_handle(r->handle);

	server_ref((SERVER_REC*)r);
	if(!r->size) {
		uint32_t size;
		net_receive(chan, (char*)&size, 4);
		size = ntohl(size);
		r->size = size;
		r->msg = malloc(size);
		r->got = 0;
	}

	r->got += net_receive(chan, r->msg+r->got, r->size - r->got);
	if(r->got == r->size) {
		quassel_parse_message(chan, r->msg, r);
		free(r->msg);
		r->size = 0;
		r->got = 0;
		r->msg = NULL;
	}
	server_unref((SERVER_REC*)r);
}

void irssi_quassel_handle(Quassel_SERVER_REC* r, int bufferid, int network, char* buffer_id, char* sender, int type, int flags, char* content) {
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
	if(type == 1) {
		char *recoded;
		Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(r), chan);
		if(!chan_rec)
			chan_rec = (Quassel_CHANNEL_REC*) quassel_channel_create(SERVER(r), chan, chan, 0);
		chan_rec->buffer_id = bufferid;
		recoded = recode_in(SERVER(r), content, chan);
		if(strcmp(sender, SERVER(r)->nick) == 0) {
			signal_emit("message own_public", 5,
				r, recoded, sender, "coin", chan);
		} else {
			signal_emit("message public", 5,
				r, recoded, nick, "coin", chan);
		}
		g_free(recoded);
	} else if(type == 0x08) {
		//Nick
		Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(r), chan);
		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chan_rec, nick);

		//nick already renamed
		if(!nick_rec)
			goto end;
		nicklist_rename(SERVER(r), nick, content);
		signal_emit("message nick", 4, SERVER(r), content, nick, address);
	} else if(type == 0x20) {
		//Join
		quassel_irssi_join2(r, chan, nick, "");

		Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(r), chan);
		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chan_rec, nick);
		signal_emit("nicklist new", 2, chan_rec, nick_rec);
		signal_emit("message join", 4, SERVER(r), chan, nick, address);
	} else if(type == 0x40) {
		//part
		signal_emit("message part", 5, SERVER(r), chan, nick, address, content);

		Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(SERVER(r), chan);
		NICK_REC* nick_rec = nicklist_find((CHANNEL_REC*)chan_rec, nick);
		signal_emit("nicklist remove", 2, chan_rec, nick_rec);
	}
end:
	free(chan);
	free(nick);
}

void irssi_handle_connected(Quassel_SERVER_REC* r) {
	r->connected = TRUE;
}

void irssi_send_message(GIOChannel* h, int buffer, const char *message);
static void send_message(SERVER_REC *server, const char *target,
			 const char *msg, int target_type) {
	Quassel_CHANNEL_REC* chan_rec = (Quassel_CHANNEL_REC*) channel_find(server, target);
	(void) target_type;
	if(!chan_rec)
		return;
	
	irssi_send_message(net_sendbuffer_handle(server->handle), chan_rec->buffer_id, msg);
}

static void sig_connected(Quassel_SERVER_REC* r) {
	r->readtag =
		g_input_add(net_sendbuffer_handle(r->handle),
			    G_INPUT_READ,
			    (GInputFunction) quassel_parse_incoming, r);
	quassel_init_packet(net_sendbuffer_handle(r->handle));
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
	char *s = NULL;
	asprintf(&s, "%s-%s", network, chan);
	channel_change_topic(server, s, topic, "", time(NULL));
	Quassel_CHANNEL_REC* chanrec = (Quassel_CHANNEL_REC*)channel_find(SERVER(server), s);
	free(s);
	if(!chanrec)
		return;
	if(chanrec->buffer_id == -1)
		chanrec->buffer_id = quassel_find_buffer_id(chan, atoi(network));
}

static const char *get_nick_flags(SERVER_REC *server) {
	(void)server;
	return "";
}

extern void quassel_user_pass(char *user, char *pass);
SERVER_REC* quassel_server_init_connect(SERVER_CONNECT_REC* conn) {
	Quassel_SERVER_CONNECT_REC *r = (Quassel_SERVER_CONNECT_REC*) conn;
	(void) r;

	Quassel_SERVER_REC *ret = (Quassel_SERVER_REC*) g_new0(Quassel_SERVER_REC, 1);
	ret->chat_type = Quassel_PROTOCOL;
	ret->connrec = r;
	ret->msg = NULL;
	ret->size = 0;
	ret->got = 0;
	server_connect_ref(SERVER_CONNECT(conn));

	ret->connrec->use_ssl = 0;
	ret->connrec->port = 443;

	ret->channels_join = quassel_channels_join;
	ret->send_message = send_message;
	ret->get_nick_flags = get_nick_flags;
	auto int ischannel(SERVER_REC* server, const char* chan) {
		(void) server;
		(void) chan;
		return 0;
	}
	ret->ischannel = ischannel;
	quassel_user_pass(conn->nick, conn->password);

	server_connect_init((SERVER_REC*)ret);

	return (SERVER_REC*)ret;
}

void quassel_core_init(void) {
	CHAT_PROTOCOL_REC *rec;
	rec = g_new0(CHAT_PROTOCOL_REC, 1);
	rec->name = "Quassel";
	rec->fullname = "Quassel an IRC bouncer with its own protocol";
	rec->chatnet = "quassel";

	rec->case_insensitive = TRUE;
	rec->create_chatnet = create_chatnet;
	rec->create_server_setup = create_server_setup;
	rec->create_server_connect = create_server_connect;
    rec->create_channel_setup = create_channel_setup;
    rec->destroy_server_connect = destroy_server_connect;

    rec->server_init_connect = quassel_server_init_connect;
    rec->server_connect = quassel_server_connect;
    rec->query_create = quassel_query_create;

	rec->channel_create = quassel_channel_create;

	command_set_options("connect", "+quassel");
	command_set_options("server add", "-quassel");

    chat_protocol_register(rec);
    g_free(rec);

    module_register("quassel", "core");

	signal_add_first("server connected", (SIGNAL_FUNC) sig_connected);
}

void quassel_core_deinit(void) {
	signal_emit("chat protocol deinit", 1, chat_protocol_find("Quassel"));
	chat_protocol_unregister("Quassel");
}
