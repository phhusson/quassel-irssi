#ifndef DISPLAY_H
#define DISPLAY_H

int display_string(char *buf);
int display_bufferinfo(char *buf);
int display_message(char *buf);
int display_bytearray(char *buf);
int display_usertype(char *buf);
int display_int(char *buf, int type);
int display_short(char *buf);
int display_date(char *buf);
int display_time(char *buf);
int display_bool(char *buf);
int display_map(char *buf);
int display_list(char *buf);
int display_stringlist(char *buf);
int display_qvariant(char *buf);
#endif
