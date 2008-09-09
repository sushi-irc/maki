include ../Makefile.common

CFLAGS := $(subst -pedantic,,$(CFLAGS))

CFLAGS  += $(shell pkg-config --cflags dbus-glib-1) $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags gobject-2.0) $(shell pkg-config --cflags gthread-2.0)
LDFLAGS += $(shell pkg-config --libs   dbus-glib-1) $(shell pkg-config --libs   glib-2.0) $(shell pkg-config --libs   gobject-2.0) $(shell pkg-config --libs   gthread-2.0)

CFLAGS += -DLOCALEDIR='"$(localedir)"'

dbusdir = $(shell pkg-config --variable session_bus_services_dir dbus-1)

COMPONENTS = cache channel channel_user config connection dbus in log marshal misc out servers user
HEADERS    = maki.h sashimi.h $(COMPONENTS:%=%.h) dbus_glue.h
OBJECTS    = maki.o sashimi.o $(COMPONENTS:%=%.o)

all: maki

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(bindir)'
	$(INSTALL) -d -m 755 '$(DESTDIR)$(dbusdir)'
	$(INSTALL) -m 755 maki '$(DESTDIR)$(bindir)'
	-$(SED) 's#@bindir@#$(bindir)#' 'de.ikkoku.sushi.service' > '$(DESTDIR)$(dbusdir)/de.ikkoku.sushi.service'

	$(MAKE) -C po $@

clean:
	$(RM) maki
	$(RM) $(OBJECTS)
	$(RM) dbus_glue.h marshal.list marshal.c marshal.h
	$(RM) libsashimi.so

	$(MAKE) -C po $@

dbus_glue.h: dbus.xml
	$(QUIET_GEN) dbus-binding-tool --mode=glib-server --prefix=maki_dbus $+ > $@

marshal.list: dbus.c
	$(QUIET_GEN) grep -E -h -o 'maki_marshal_[0-9A-Z]+__([0-9A-Z]+_?)+' $+ \
		| sed -e 's/^maki_marshal_//' -e 's/__/:/' -e 's/_/,/g' \
		| sort -u \
		> $@

marshal.c: marshal.list
	$(QUIET_GEN) glib-genmarshal --body --prefix=maki_marshal $+ > $@

marshal.h: marshal.list
	$(QUIET_GEN) glib-genmarshal --header --prefix=maki_marshal $+ > $@

$(OBJECTS): %.o: %.c $(HEADERS)
	$(QUIET_CC) $(CC) $(CFLAGS) -c -o $@ $<

maki: $(OBJECTS)
	$(QUIET_LD) $(CC) $(LDFLAGS) -o $@ $+

libsashimi.so: sashimi.c
	$(QUIET_CC) $(CC) $(CFLAGS) -fPIC $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
