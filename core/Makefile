#IRSSI_INCLUDE:=/home/phh/irssi-0.8.16-rc1.phh/
IRSSI_INCLUDE:=/usr/include/irssi/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/core/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/fe-common/core/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/fe-text/
IRSSI_CFLAGS+=$(shell pkg-config glib-2.0 --cflags)
IRSSI_CFLAGS+=-DUOFF_T_LONG -fPIC -DHAVE_OPENSSL

CFLAGS=-std=gnu11 -Wall -Wextra -Werror -g -O2 $(IRSSI_CFLAGS) -Wmissing-prototypes -Wmissing-declarations
all: libquassel_core.so

LIBQUASSEL:=lib/bot.o lib/cmds.o lib/display.o lib/endian.o
LIBQUASSEL+=lib/getters.o lib/main.o lib/setters.o

irssi/network-openssl.o: CFLAGS:=$(IRSSI_CFLAGS)

libquassel_core.so: quassel-core.o quassel-net.o quassel-fe-window.o quassel-msgs.o quassel-cmds.o irssi/network-openssl.o $(LIBQUASSEL)
	gcc -shared $^ -o $@

clean:
	rm -f irssi/*.o lib/*.o *.o libquassel_core.so
