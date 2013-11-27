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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include "quasselc.h"

#include <common.h>
#include <network.h>

void Login(GIOChannel* h, char *user, char *pass) {
	//HeartBeat
	char msg[2048];
	int size;
	int elements;

	size=0;
	elements=0;
	bzero(msg, sizeof(msg));

	size+=add_string_in_map(msg+size, "MsgType", "ClientLogin");
	elements++;

	size+=add_string_in_map(msg+size, "User", user);
	elements++;

	size+=add_string_in_map(msg+size, "Password", pass);
	elements++;

	//The message will be of that length
	uint32_t v=htonl(size+8);
	net_transmit(h, (char*)&v, 4);
	//This is a QMap
	v=htonl(8);
	net_transmit(h, (char*)&v, 4);
	
	//QMap is valid
	v=0;
	net_transmit(h, (char*)&v, 1);
	//The QMap has <elements> elements
	v=htonl(elements);
	net_transmit(h, (char*)&v, 4);
	net_transmit(h, msg, size);
}

void HeartbeatReply(GIOChannel* h) {
	//HeartBeat
	char msg[2048];
	int size;

	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 2 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 2);

	//QVariant<Int> = HeartbeatReply
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 6);

	//QTime
	size+=add_qvariant(msg+size, 15);
	//I don't have any clue what time it is...
	size+=add_int(msg+size, 0);

	//The message will be of that length
	uint32_t v=htonl(size+4);
	net_transmit(h, (char*)&v, 4);
	net_transmit(h, msg, size);
}

void send_message(GIOChannel*h, struct bufferinfo b, char *message) {
	//HeartBeat
	char msg[2048];
	int size;

	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 4 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 4);

	//QVariant<Int> = RPC
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 2);

	//QVariant<Bytearray> = "2sendInput(BufferInfo,QString)"
	size+=add_qvariant(msg+size, 12);
	size+=add_bytearray(msg+size, "2sendInput(BufferInfo,QString)");

	//QVariant<BufferInfo>
	size+=add_qvariant(msg+size, 127);
	size+=add_bytearray(msg+size, "BufferInfo");
	size+=add_bufferinfo(msg+size, b);

	//String
	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, message);

	//The message will be of that length
	uint32_t v=htonl(size);
	net_transmit(h, (char*)&v, 4);
	net_transmit(h, msg, size);
}

void initRequest(GIOChannel* h, char *val, char *arg) {
	char msg[2048];
	int size;
	
	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 3 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 3);

	//QVariant<Int> = InitRequest
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 3);

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, val);

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, arg);

	//The message will be of that length
	uint32_t v=htonl(size+4);
	net_transmit(h, (char*)&v, 4);
	net_transmit(h, msg, size);
}

void requestBacklog(GIOChannel *h, int buffer, int first, int last, int limit, int additional) {
	char msg[2048];
	int size;
	
	size=0;
	bzero(msg, sizeof(msg));

	//QVariant<QList<QVariant>> of 9 elements
	size+=add_qvariant(msg+size, 9);
	size+=add_int(msg+size, 9);

	//QVariant<Int> = Sync
	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, 1);

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, "BacklogManager");

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, "");

	size+=add_qvariant(msg+size, 10);
	size+=add_string(msg+size, "requestBacklog");

	size+=add_qvariant(msg+size, 127);
	size+=add_bytearray(msg+size, "BufferId");
	size+=add_int(msg+size, buffer);

	size+=add_qvariant(msg+size, 127);
	size+=add_bytearray(msg+size, "MsgId");
	size+=add_int(msg+size, first);

	size+=add_qvariant(msg+size, 127);
	size+=add_bytearray(msg+size, "MsgId");
	size+=add_int(msg+size, last);

	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, limit);

	size+=add_qvariant(msg+size, 2);
	size+=add_int(msg+size, additional);


	//The message will be of that length
	uint32_t v=htonl(size+4);
	net_transmit(h, (char*)&v, 4);
	net_transmit(h, msg, size);
}
