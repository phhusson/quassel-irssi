INSTALL = install
DESTDIR ?=
LIBDIR ?= /usr/lib

#IRSSI_INCLUDE:=/home/phh/irssi-0.8.16-rc1.phh/
IRSSI_INCLUDE:=/usr/include/irssi/
IRSSI_LIB?=$(DESTDIR)/$(LIBDIR)/irssi
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/core/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/fe-common/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/fe-common/core/
IRSSI_CFLAGS+=-I$(IRSSI_INCLUDE)/src/fe-text/
IRSSI_CFLAGS+=$(shell pkg-config glib-2.0 --cflags)
IRSSI_CFLAGS+=-DUOFF_T_LONG -fPIC -DHAVE_OPENSSL
OBJECTS:=quasselc-connector.o quassel-core.o
OBJECTS+=quassel-net.o quassel-msgs.o quassel-cmds.o
OBJECTS+=irssi/network-openssl.o
OBJECTS+=quassel-fe-window.o quassel-fe-level.o quassel-cfg.o

LDFLAGS ?=

ifndef SYSTEM_QUASSELC
    QUASSELC_FLAGS:=-Ilib
else
    QUASSELC_FLAGS:=$(shell pkg-config --cflags quasselc)
    LDFLAGS += -lquasselc
endif

CFLAGS=-std=gnu11 -Wall -Wextra -Werror -g -O2 $(IRSSI_CFLAGS) $(QUASSELC_FLAGS) -Wmissing-prototypes -Wmissing-declarations

TARGET=libquassel_core.so

all: libquassel_core.so

ifndef SYSTEM_QUASSELC
    LIBQUASSEL:=lib/cmds.o lib/display.o lib/io.o
    LIBQUASSEL+=lib/getters.o lib/main.o lib/setters.o
    LIBQUASSEL+=lib/negotiation.o
    OBJECTS += $(LIBQUASSEL)
endif

irssi/network-openssl.o: CFLAGS:=$(IRSSI_CFLAGS)

quasselc-connector.o: CFLAGS:=$(CFLAGS)

$(TARGET): $(OBJECTS)
	gcc -shared $^ -o $@ -lz $(LDFLAGS)

install: $(TARGET)
	$(INSTALL) -d $(IRSSI_LIB)/modules
	$(INSTALL) -m0644 libquassel_core.so $(IRSSI_LIB)/modules

user_install: $(TARGET)
	$(INSTALL) -d $(HOME)/.irssi/modules/
	$(INSTALL) -m0644 libquassel_core.so $(HOME)/.irssi/modules/

clean:
	rm -f irssi/*.o lib/*.o *.o libquassel_core.so
