include ../Makefile.common

CFLAGS := $(subst -pedantic,,$(CFLAGS))

CFLAGS  += -I../sashimi $(shell pkg-config --cflags dbus-glib-1) $(shell pkg-config --cflags glib-2.0)
LDFLAGS += -L../sashimi $(shell pkg-config --libs dbus-glib-1) $(shell pkg-config --libs glib-2.0) -lsashimi

COMPONENTS = cache connection dbus irc log marshal misc out servers
OBJECTS = maki.o $(COMPONENTS:%=maki_%.o)

all: maki

clean:
	$(RM) maki
	$(RM) $(OBJECTS)
	$(RM) maki_dbus_glue.h maki_marshal.c maki_marshal.h

maki_dbus.c: maki_dbus_glue.h maki_marshal.h

maki_dbus_glue.h: maki_dbus.xml
	$(QUIET_GEN) dbus-binding-tool --mode=glib-server --prefix=maki_dbus $+ > $@

maki_marshal.c: maki_marshal.list
	$(QUIET_GEN) glib-genmarshal --body $+ > $@

maki_marshal.h: maki_marshal.list
	$(QUIET_GEN) glib-genmarshal --header $+ > $@

$(OBJECTS): %.o: %.c
	$(QUIET_CC) $(CC) $(CFLAGS) -c -o $@ $+

maki: $(OBJECTS)
	$(QUIET_LD) $(CC) $(LDFLAGS) -o $@ $+