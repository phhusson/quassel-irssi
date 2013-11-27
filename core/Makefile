IRSSI_INCLUDE:=/usr/include/irssi/
CFLAGS=-std=gnu11 -Wall -Wextra -Werror -g
CFLAGS+=-I$(IRSSI_INCLUDE)/src/
CFLAGS+=-I$(IRSSI_INCLUDE)/src/core/
CFLAGS+=$(shell pkg-config glib-2.0 --cflags)
CFLAGS+=-DUOFF_T_LONG -fPIC
all: libquassel_core.so

LIBQUASSEL:=lib/bot.o lib/cmds.o lib/display.o lib/endian.o
LIBQUASSEL+=lib/getters.o lib/main.o lib/setters.o

libquassel_core.so: quassel-core.o $(LIBQUASSEL)
	gcc -shared $^ -o $@

clean:
	rm -f lib/*.o *.o libquassel_core.so
