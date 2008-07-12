include ../Makefile.common

CFLAGS := $(subst -pedantic,,$(CFLAGS))

CFLAGS  += $(shell pkg-config --cflags dbus-glib-1) $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gobject-2.0) $(shell pkg-config --cflags gthread-2.0)
LDFLAGS += $(shell pkg-config --libs   dbus-glib-1) $(shell pkg-config --libs   glib-2.0) $(shell pkg-config --libs   gobject-2.0) $(shell pkg-config --libs   gthread-2.0)

dbusdir = $(shell pkg-config --variable session_bus_services_dir dbus-1)

COMPONENTS = cache channel channel_user config connection dbus in log marshal misc out servers user
HEADERS    = maki.h sashimi.h $(COMPONENTS:%=maki_%.h)
OBJECTS    = maki.o sashimi.o $(COMPONENTS:%=maki_%.o)

all: maki

install: all
	$(INSTALL) -d -m 755 '$(bindir)'
	$(INSTALL) -m 755 maki '$(bindir)'
	$(SED) 's#@bindir@#$(bindir)#' 'de.ikkoku.sushi.service' > '$(dbusdir)/de.ikkoku.sushi.service'

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

$(OBJECTS): %.o: %.c $(HEADERS)
	$(QUIET_CC) $(CC) $(CFLAGS) -c -o $@ $<

maki: $(OBJECTS)
	$(QUIET_LD) $(CC) $(LDFLAGS) -o $@ $+

libsashimi.so: sashimi.c
	$(QUIET_CC) $(CC) $(CFLAGS) -fPIC $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
