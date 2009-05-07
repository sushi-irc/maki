include ../../Makefile.common

# To quiet warnings from generated code.
SUSHI_CFLAGS := $(subst -pedantic,,$(SUSHI_CFLAGS))

SUSHI_CFLAGS  += $(shell $(PKG_CONFIG) --cflags dbus-glib-1) $(shell $(PKG_CONFIG) --cflags glib-2.0) $(shell $(PKG_CONFIG) --cflags gobject-2.0) $(shell $(PKG_CONFIG) --cflags gthread-2.0)
SUSHI_LDFLAGS += $(shell $(PKG_CONFIG) --libs   dbus-glib-1) $(shell $(PKG_CONFIG) --libs   glib-2.0) $(shell $(PKG_CONFIG) --libs   gobject-2.0) $(shell $(PKG_CONFIG) --libs   gthread-2.0)

ifeq ($(shell PKG_CONFIG='$(PKG_CONFIG)' ../../tools/pkg-config.sh nice), yes)
	SUSHI_CFLAGS  += $(shell $(PKG_CONFIG) --cflags nice) -DSUSHI_HAVE_NICE
	SUSHI_LDFLAGS += $(shell $(PKG_CONFIG) --libs   nice)
endif

SUSHI_CFLAGS += -DLOCALEDIR='"$(localedir)"'
SUSHI_CFLAGS += -DSUSHI_VERSION='"$(SUSHI_VERSION)"'

COMPONENTS = ilib channel channel_user dcc dbus in instance log marshal misc out server user
HEADERS    = maki.h sashimi.h $(COMPONENTS:%=%.h) dbus_glue.h
OBJECTS    = maki.o sashimi.o $(COMPONENTS:%=%.o)

all: maki

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(bindir)'
	$(INSTALL) -m 755 maki '$(DESTDIR)$(bindir)'

clean:
	$(RM) -f maki
	$(RM) -f $(OBJECTS)
	$(RM) -f dbus_glue.h marshal.list marshal.c marshal.h
	$(RM) -f libsashimi.so

dbus_glue.h: dbus.xml
	$(QUIET_GEN) dbus-binding-tool --mode=glib-server --prefix=maki_dbus $+ > $@

marshal.list: dbus.c
	$(QUIET_GEN) $(GREP) -E -h -o 'maki_marshal_[0-9A-Z]+__([0-9A-Z]+_)*[0-9A-Z]+' $+ \
		| $(SED) -e 's/^maki_marshal_//' -e 's/__/:/' -e 's/_/,/g' \
		| $(SORT) -u \
		> $@

marshal.c: marshal.list
	$(QUIET_GEN) glib-genmarshal --header --body --prefix=maki_marshal $+ > $@

marshal.h: marshal.list
	$(QUIET_GEN) glib-genmarshal --header --prefix=maki_marshal $+ > $@

$(OBJECTS): %.o: %.c $(HEADERS)
	$(QUIET_CC) $(CC) $(SUSHI_CFLAGS) -c -o $@ $<

maki: $(OBJECTS)
	$(QUIET_LD) $(CC) $(SUSHI_LDFLAGS) -o $@ $+

libsashimi.so: sashimi.c
	$(QUIET_CC) $(CC) $(SUSHI_CFLAGS) -fPIC $(SUSHI_LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
