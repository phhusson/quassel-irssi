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

static iconv_t ico;
static void __init() __attribute__((constructor));
static void __init() {
	ico = iconv_open("UTF-16BE//TRANSLIT", "UTF-8");
}

char *convert_string(char *str, int* size) {
	size_t s1,s2;
	static char buf[2048];
	char *pos=buf;
	bzero(buf, sizeof(buf));
	s1=strlen(str);
	s2=sizeof(buf);
	iconv(ico, &str, &s1, &pos, &s2);
	if(s1!=0) {
		fprintf(stderr, "iconv failed ! \n");
		abort();
	}
	*size=sizeof(buf)-s2;
	return buf;
}

int add_string(char *msg, char *str) {
	int size=0;
	char *tmp=convert_string(str, &size);
	*(uint32_t*)(msg)=htonl(size);
	memcpy(msg+4, tmp, size);
	return size+4;
}

int add_bytearray(char *msg, char *str) {
	*(uint32_t*)(msg)=htonl(strlen(str));
	memcpy(msg+4, str, strlen(str));
	return strlen(str)+4;
}

int add_bytearray2(char *msg, char *str, int size) {
	*(uint32_t*)(msg)=htonl(size);
	memcpy(msg+4, str, size);
	return size+4;
}

int add_int(char *msg, uint32_t v) {
	*(uint32_t*)(msg)=htonl(v);
	return 4;
}

int add_short(char *msg, uint16_t v) {
	*(uint16_t*)(msg)=htons(v);
	return 2;
}

int add_qvariant(char *msg, int type) {
	*(uint32_t*)(msg)=htonl(type);
	msg[4]=0;
	return 5;
}

int add_bufferinfo(char *buf, struct bufferinfo b) {
	int pos=0;
	pos+=add_int(buf+pos, b.id);
	pos+=add_int(buf+pos, b.network);
	pos+=add_short(buf+pos, b.type);
	pos+=add_int(buf+pos, b.group);
	pos+=add_bytearray(buf+pos, b.name);
	return pos;
}

int add_string_in_map(char *msg, char *key, char* value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 10);
	buf+=add_string(buf, value);

	return buf-msg;
}

int add_bool_in_map(char *msg, char *key, int value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 1);
	*buf=value;
	buf++;

	return buf-msg;
}

int add_int_in_map(char *msg, char *key, int value) {
	char *buf=msg;

	//Key
	buf+=add_string(msg, key);

	//Value
	buf+=add_qvariant(buf, 3);
	buf+=add_int(buf, value);

	return buf-msg;
}

