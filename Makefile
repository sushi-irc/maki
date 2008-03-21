include ../Makefile.common

CFLAGS  += -I../sashimi $(shell pkg-config --cflags dbus-1 glib-2.0) -fgnu89-inline
LDFLAGS += -L../sashimi $(shell pkg-config --libs dbus-1 glib-2.0) -lsashimi

all: maki

clean:
	$(RM) maki

maki: maki.c
	$(QUIET_CC) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+