include ../Makefile.common

CFLAGS := $(subst -pedantic,,$(CFLAGS))

CFLAGS  += $(shell pkg-config --cflags dbus-glib-1) $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gthread-2.0)
LDFLAGS += $(shell pkg-config --libs dbus-glib-1) $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs gthread-2.0)

COMPONENTS = cache channel config connection dbus in log marshal misc out servers
OBJECTS = maki.o sashimi.o $(COMPONENTS:%=maki_%.o)

all: maki

install: all
	$(INSTALL) -d -m 755 '$(PREFIX)/bin'
	$(INSTALL) -m 755 maki '$(PREFIX)/bin'

clean:
	$(RM) maki
	$(RM) $(OBJECTS)
	$(RM) maki_dbus_glue.h maki_marshal.c maki_marshal.h
	$(RM) libsashimi.so

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

libsashimi.so: sashimi.c
	$(QUIET_CC) $(CC) $(CFLAGS) -fPIC $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
