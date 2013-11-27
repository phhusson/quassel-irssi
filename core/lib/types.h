#ifndef TYPES_H
#define TYPES_H

struct bufferinfo {
	uint32_t id;
	uint32_t network;
	uint16_t type;
	uint32_t group;
	char *name;
};

struct message {
	uint32_t id;
	uint32_t timestamp;
	uint32_t type;
	char flags;
	struct bufferinfo buffer;
	char *sender;
	char *content;
};

#endif
