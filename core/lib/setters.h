#ifndef SETTERS_H
#define SETTERS_

//char *convert_string(char *str, int *size);

int add_string(char *msg, const char *str);
int add_bytearray(char *msg, const char *str);
int add_int(char *msg, uint32_t v);
int add_short(char *msg, uint16_t v);
int add_qvariant(char *msg, int type);
int add_bufferinfo(char *buf, struct bufferinfo b);
int add_string_in_map(char *msg, char *key, char* value);
int add_bool_in_map(char *msg, char *key, int value);
int add_int_in_map(char *msg, char *key, int value);
#endif
