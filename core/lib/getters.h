#ifndef GETTERS_H
#define GETTERS_H

char *convert_back(char *str, int orig_size, int *size);

struct bufferinfo get_bufferinfo(char **buf);
struct message get_message(char **buf);
char* get_bytearray(char **buf);
char* get_string(char **buf);
char get_byte(char **buf);
uint32_t get_int(char **buf);
uint16_t get_short(char **buf);
int get_qvariant(char **buf);

//Those getters doesn't return anything, just sets buf to proepr address
void get_variant_t(char **buf, int type);
void get_variant(char **buf);
void get_map(char **buf);
void get_stringlist(char **buf);
void get_list(char **buf);
void get_date(char **buf);

#endif
