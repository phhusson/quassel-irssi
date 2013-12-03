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

int display_string(char *buf) {
	uint32_t size = *((uint32_t*)buf);
	size = ntohl(size);
	if(size==0xffffffff) {
		printf(" Str()");
		return 4;
	}
	buf+=4;
	int res=0;
	char *str=convert_back(buf, size, &res);
	printf(" Str('%s')", str);
	return size+4;
}

int display_bufferinfo(char *buf) {
	uint32_t id = *((uint32_t*)buf);
	id=ntohl(id);
	buf+=4;

	uint32_t networkID=*((uint32_t*)buf);
	networkID=ntohl(networkID);
	buf+=4;

	uint16_t type=*((uint16_t*)buf);
	type=ntohs(type);
	buf+=2;

	uint32_t groupID=*((uint32_t*)buf);
	groupID=ntohl(groupID);
	buf+=4;

	uint32_t size = *((uint32_t*)buf);
	size = ntohl(size);
	buf+=4;

	if(size==0xffffffff) {
		printf("Size was <0 !\n");
		size=0;
	}
	printf("BufferInfo(%d, %d, %d, %d, '%s')", id, networkID, type, groupID, buf);
	return 18+size;
}

int display_message(char *buf) {
	char *orig_buf=buf;
	uint32_t messageID=*((uint32_t*)buf);
	messageID=ntohl(messageID);
	buf+=4;

	uint32_t timestamp=*((uint32_t*)buf);
	timestamp=ntohl(timestamp);
	buf+=4;

	uint32_t type=*((uint32_t*)buf);
	type=ntohl(type);
	buf+=4;

	char flags=*buf;
	buf++;

	printf("Message(%d, %d, %d, %d, ", messageID, timestamp, type, flags);
	buf+=display_bufferinfo(buf);

	printf(", ");
	//Sender
	buf+=display_bytearray(buf);
	printf(", ");
	//Content
	buf+=display_bytearray(buf);

	printf(")\n");

	return buf-orig_buf;
}

int display_bytearray(char *buf) {
	uint32_t size = *((uint32_t*)buf);
	size = ntohl(size);
	if(size==0xffffffff) {
		printf("Bytearray()");
		return 4;
	} else {
		char *str=malloc(size+1);
		memcpy(str, buf+4, size);
		str[size]=0;
		printf("Bytearray('%s')", str);
		free(str);
	}
	return 4+size;
}

int display_usertype(char *buf) {
	char *orig_buf=buf;
	uint32_t size = *((uint32_t*)buf);
	size = ntohl(size);
	buf+=4;
	if(strcmp(buf, "NetworkId")==0 || strcmp(buf, "IdentityId")==0 || strcmp(buf, "BufferId")==0 || strcmp(buf, "MsgId")==0) {
		printf("%s(", buf);
		buf+=strlen(buf)+1;
		buf+=display_int(buf, 0);
		printf(")");
	} else if(strcmp(buf, "Identity")==0) {
		buf+=strlen(buf)+1;
		buf+=display_map(buf);
	} else if(strcmp(buf, "BufferInfo")==0) {
		buf+=strlen(buf)+1;
		buf+=display_bufferinfo(buf);
	} else if(strcmp(buf, "Message")==0) {
		buf+=strlen(buf)+1;
		buf+=display_message(buf);
	} else if(strcmp(buf, "Network::Server")==0) {
		buf+=strlen(buf)+1;
		buf+=display_map(buf);
	} else {
		printf(" Usertype('%s') \n", buf);
		printf("Unsupported.\n");
		exit(0);
	}
	return buf-orig_buf;
}

int display_int(char *buf, int type) {
	uint32_t size = *((uint32_t*)buf);
	size = ntohl(size);
	printf(" Int(%08x, %d) ", size, type);
	return 4;
}

int display_short(char *buf) {
	uint16_t size = *((uint16_t*)buf);
	size = ntohs(size);
	printf(" Short(%04x) ", size);
	return 2;
}

int display_date(char *buf) {
	uint32_t julianDay = *((uint32_t*)buf);
	julianDay=ntohl(julianDay);
	buf+=4;
	uint32_t secondSinceMidnight = *((uint32_t*)buf);
	secondSinceMidnight=ntohl(secondSinceMidnight);
	buf+=4;
	uint32_t isUTC = *buf;
	buf++;
	printf(" Date(%d, %d, %d) \n", julianDay, secondSinceMidnight, isUTC);
	return 9;
}

int display_time(char *buf) {
	uint32_t secondsSinceMidnight = *((uint32_t*)buf);
	secondsSinceMidnight=ntohl(secondsSinceMidnight);
	printf(" Time(%d) \n", secondsSinceMidnight);
	return 4;
}

int display_bool(char *buf) {
	printf(" Bool(%s) \n", *buf ? "TRUE" : "FALSE");
	return 1;
}

int display_map(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ntohl(elements);
	printf("Got %d elements\n", elements);
	printf("Map(\n");
	buf+=4;
	uint32_t i;
	for(i=0;i<elements;++i) {
		//key
		printf("key= ");
		buf+=display_string(buf);
		printf(", ");
		//Value
		printf("value= ");
		buf+=display_qvariant(buf);
		printf("\n");
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_list(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ntohl(elements);
	printf("Got %d elements\n", elements);
	printf("List(\n");
	buf+=4;
	uint32_t i;
	for(i=0;i<elements;++i) {
		//Value
		printf("value= ");
		buf+=display_qvariant(buf);
		printf("\n");
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_stringlist(char *buf) {
	char *orig_buf=buf;
	uint32_t elements = *((uint32_t*)buf);
	elements = ntohl(elements);
	printf("Got %d elements\n", elements);
	printf("StringList(\n");
	buf+=4;
	uint32_t i;
	for(i=0;i<elements;++i) {
		//Value
		printf("value= ");
		buf+=display_string(buf);
	}
	printf(");\n");
	return buf-orig_buf;
}

int display_qvariant(char *buf) {
	char *orig_buf=buf;
	uint32_t type = *((uint32_t*)buf);
	type=ntohl(type);
	buf+=4;
	char null=*buf;
	buf++;
	if(null) {
		//Nothing to do
	}
	switch(type) {
		case 1:
			buf+=display_bool(buf);
			break;
		case 2:
		case 3:
			buf+=display_int(buf, type);
			break;
		case 8:
			buf+=display_map(buf);
			break;
		case 9:
			buf+=display_list(buf);
			break;
		case 10:
			buf+=display_string(buf);
			break;
		case 11:
			buf+=display_stringlist(buf);
			break;
		case 12:
			buf+=display_bytearray(buf);
			break;
		case 15:
			buf+=display_time(buf);
			break;
		case 16:
			buf+=display_date(buf);
			break;
		case 127:
			//User type !
			buf+=display_usertype(buf);
			break;
		case 133:
			buf+=display_short(buf);
			break;
		default:
			printf("Unknown QVariant type: %d\n", type);
			exit(-1);
			break;
	};
	return buf-orig_buf;
}

