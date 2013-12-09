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

static CHATNET_REC *create_chatnet(void) {
    return g_new0(CHATNET_REC, 1);
}

static SERVER_SETUP_REC *create_server_setup(void) {
    return g_new0(SERVER_SETUP_REC, 1);
}

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
	rec->last_msg_id = -1;
	rec->last_seen_msg_id = -1;
	rec->init_last_seen_msg_id = -1;
	rec->first_msg_id = -1;
	rec->got_backlog = 0;

	return (CHANNEL_REC*)rec;
}

void quassel_irssi_channels_join(SERVER_REC *server, const char *data,
			      int automatic) {
	quassel_channel_create(server, data, data, automatic);
}

static QUERY_REC* quassel_query_create(const char *server_tag, const char* nick, int automatic) {
	Quassel_QUERY_REC *rec = g_new0(Quassel_QUERY_REC, 1);
	rec->chat_type = Quassel_PROTOCOL;
	rec->name = g_strdup(nick);
	rec->server_tag = g_strdup(server_tag);
	query_init((QUERY_REC*)rec, automatic);

	return (QUERY_REC*)rec;
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

	rec->query_create = quassel_query_create;

	rec->channel_create = quassel_channel_create;

	command_set_options("connect", "+quassel");
	command_set_options("server add", "-quassel");

	quassel_net_init(rec);
	chat_protocol_register(rec);
	g_free(rec);

	quassel_fewindow_init();
	quassel_msgs_init();
	quassel_cmds_init();

	module_register("quassel", "core");
}

void quassel_core_deinit(void) {
	signal_emit("chat protocol deinit", 1, chat_protocol_find("Quassel"));
	chat_protocol_unregister("Quassel");
	quassel_fewindow_deinit();
	quassel_msgs_deinit();
	quassel_cmds_deinit();
}
