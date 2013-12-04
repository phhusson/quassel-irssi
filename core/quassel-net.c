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

#include "quassel-irssi.h"

static void quassel_server_connect(SERVER_REC *server) {
	if (!server_start_connect(server)) {
		server_connect_unref(server->connrec);
		g_free(server);
	}
}

extern void quassel_init_packet(GIOChannel*,int);
extern void quassel_parse_message(GIOChannel*, char*, void*);

static void quassel_parse_incoming(Quassel_SERVER_REC* r) {
	GIOChannel *chan = net_sendbuffer_handle(r->handle);

	server_ref((SERVER_REC*)r);
	if(!r->size) {
		uint32_t size;
		net_receive(chan, (char*)&size, 4);
		size = ntohl(size);
		if(!size)
			return;
		r->msg = malloc(size);
		if(!r->msg)
			return;
		r->size = size;
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

void irssi_handle_connected(Quassel_SERVER_REC* r) {
	r->connected = TRUE;
}

static void sig_connected(Quassel_SERVER_REC* r) {
	if(!PROTO_CHECK_CAST(SERVER(r), Quassel_SERVER_REC, chat_type, "Quassel"))
		return;
	r->readtag =
		g_input_add(net_sendbuffer_handle(r->handle),
			    G_INPUT_READ,
			    (GInputFunction) quassel_parse_incoming, r);
	quassel_init_packet(net_sendbuffer_handle(r->handle), r->ssl);
}

static const char *get_nick_flags(SERVER_REC *server) {
	(void)server;
	return "";
}

static SERVER_REC* quassel_server_init_connect(SERVER_CONNECT_REC* conn) {
	Quassel_SERVER_CONNECT_REC *r = (Quassel_SERVER_CONNECT_REC*) conn;

	Quassel_SERVER_REC *ret = (Quassel_SERVER_REC*) g_new0(Quassel_SERVER_REC, 1);
	ret->chat_type = Quassel_PROTOCOL;
	ret->connrec = r;
	ret->msg = NULL;
	ret->size = 0;
	ret->got = 0;
	server_connect_ref(SERVER_CONNECT(conn));

	if(conn->use_ssl) {
		ret->ssl = 1;
	}
	ret->connrec->use_ssl = 0;

	ret->channels_join = quassel_irssi_channels_join;
	ret->send_message = quassel_irssi_send_message;
	ret->get_nick_flags = get_nick_flags;
	auto int ischannel(SERVER_REC* server, const char* chan) {
		(void) server;
		(void) chan;
		return 0;
	}
	ret->ischannel = ischannel;

	server_connect_init((SERVER_REC*)ret);

	return (SERVER_REC*)ret;
}

void quassel_net_init(CHAT_PROTOCOL_REC* rec) {
	rec->server_init_connect = quassel_server_init_connect;
	rec->server_connect = quassel_server_connect;
	signal_add_first("server connected", (SIGNAL_FUNC) sig_connected);
}

void quassel_login(GIOChannel* h, char *user, char *pass);
GIOChannel *irssi_ssl_get_iochannel(GIOChannel *handle, int port, SERVER_REC *server);
void quassel_irssi_init_ack(Quassel_SERVER_REC *server) {
	if(!server->ssl)
		goto login;
	GIOChannel* ssl_handle = irssi_ssl_get_iochannel(server->handle->handle, 1337, SERVER(server));
	int error;
	//That's polling, and that's really bad...
	while( (error=irssi_ssl_handshake(ssl_handle)) & 1) {
		if(error==-1) {
			fprintf(stderr, "SSL handshake failed\n");
			exit(1);
		}
	}
	server->handle->handle = ssl_handle;

login:
	quassel_login(server->handle->handle, server->connrec->nick, server->connrec->password);
}
