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

static iconv_t ico2;
static void __init() __attribute__((constructor));
static void __init() {
	ico2 = iconv_open("UTF-8", "UTF-16BE");
}

char *convert_back(char *str, int orig_size, int* size) {
	size_t s1,s2;
	static char buf[512];
	char *pos=buf;
	bzero(buf, sizeof(buf));
	s1=orig_size;
	s2=sizeof(buf);
	iconv(ico2, &str, &s1, &pos, &s2);
	if(s1!=0) {
		fprintf(stderr, "iconv failed ! \n");
		abort();
	}
	*size=sizeof(buf)-s2;
	return buf;
}

struct bufferinfo get_bufferinfo(char **buf) {
	struct bufferinfo b;
	b.id=get_int(buf);
	b.network=get_int(buf);
	b.type=get_short(buf);
	b.group=get_int(buf);
	b.name=get_bytearray(buf);
	return b;
}

struct message get_message(char **buf) {
	struct message m;
	m.id=get_int(buf);
	m.timestamp=get_int(buf);
	m.type=get_int(buf);
	m.flags=get_byte(buf);
	m.buffer=get_bufferinfo(buf);
	m.sender=get_bytearray(buf);
	m.content=get_bytearray(buf);

	return m;
}

void get_date(char **buf) {
	(*buf)+=9;
}

char* get_bytearray(char **buf) {
	uint32_t size = *((uint32_t*)*buf);
	size=ltob(size);
	(*buf)+=4;
	if(size==0xFFFFFFFF)
		return strdup("");
	char *ret=malloc(size+1);
	memcpy(ret, *buf, size);
	ret[size]=0;
	(*buf)+=size;
	return ret;
}

char* get_string(char **buf) {
	uint32_t size = *((uint32_t*)*buf);
	size=ltob(size);
	(*buf)+=4;

	if(size==0xffffffff) {
		return strdup("");
	}
	int res=0;
	char *str=convert_back(*buf, size, &res);
	(*buf)+=size;
	return strdup(str);
}

char get_byte(char **buf) {
	char v = **buf;
	(*buf)++;
	return v;
}

uint32_t get_int(char **buf) {
	uint32_t v = *((uint32_t*)*buf);
	v=ltob(v);
	(*buf)+=4;
	return v;
}

uint16_t get_short(char **buf) {
	uint16_t v = *((uint16_t*)*buf);
	v=stob(v);
	(*buf)+=2;
	return v;
}

int get_qvariant(char **buf) {
	uint32_t type = *((uint32_t*)*buf);
	type=ltob(type);
	(*buf)+=4;
	char null=**buf;
	if(null) {
		//NOP
	}
	(*buf)++;
	return type;
}

void get_variant_t(char **buf, int type) {
	switch(type) {
		case 1:
			get_byte(buf);
			break;
		case 2:
		case 3:
			get_int(buf);
			break;
		case 8:
			get_map(buf);
			break;
		case 9:
			get_list(buf);
			break;
		case 10:
			get_string(buf);
			break;
		case 11:
			get_stringlist(buf);
			break;
		case 16:
			get_date(buf);
			break;
		case 127:
			{
				char *usertype=get_bytearray(buf);
				if(strcmp(usertype, "NetworkId")==0 || strcmp(usertype, "IdentityId")==0) {
					get_int(buf);
				} else if(strcmp(usertype, "Identity")==0) {
					get_map(buf);
				} else if(strcmp(usertype, "BufferInfo")==0) {
					get_bufferinfo(buf);
				} else {
					printf("Unsupported usertype = %s (%d)\n", usertype, __LINE__);
					abort();
				}
			}
			break;
		default:
			abort();
	}
}

void get_variant(char **buf) {
	int type=get_qvariant(buf);
	get_variant_t(buf, type);
}

void get_map(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_string(buf);
		get_variant(buf);
	}
}

void get_stringlist(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_string(buf);
	}
}

void get_list(char **buf) {
	int size=get_int(buf);
	int i;
	for(i=0;i<size;++i) {
		get_variant(buf);
	}
}
