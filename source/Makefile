include ../Makefile.common

# To quiet warnings from generated code.
SUSHI_CFLAGS := $(subst -pedantic,,$(SUSHI_CFLAGS))

SUSHI_CFLAGS  += $(shell $(PKG_CONFIG) --cflags dbus-1 dbus-glib-1 glib-2.0 gobject-2.0 gthread-2.0 gmodule-2.0)
SUSHI_LDFLAGS += $(shell $(PKG_CONFIG) --libs   dbus-1 dbus-glib-1 glib-2.0 gobject-2.0 gthread-2.0 gmodule-2.0)

ifeq ($(shell PKG_CONFIG='$(PKG_CONFIG)' ../tools/pkg-config.sh nice), yes)
	SUSHI_CFLAGS  += $(shell $(PKG_CONFIG) --cflags nice) -DSUSHI_HAVE_NICE
	SUSHI_LDFLAGS += $(shell $(PKG_CONFIG) --libs   nice)
endif

SUSHI_CFLAGS += -DMAKI_PLUGIN_DIRECTORY='"$(libdir)/sushi/maki/plugins"'
SUSHI_CFLAGS += -DMAKI_SHARE_DIRECTORY='"$(sharedir)/sushi/maki"'
SUSHI_CFLAGS += -DLOCALEDIR='"$(localedir)"'
SUSHI_CFLAGS += -DSUSHI_VERSION='"$(SUSHI_VERSION)"'

SUSHI_REMOTE_CFLAGS  += $(shell $(PKG_CONFIG) --cflags glib-2.0)
SUSHI_REMOTE_LDFLAGS += $(shell $(PKG_CONFIG) --libs   glib-2.0)

COMPONENTS = ilib sashimi channel channel_user dcc_send dbus dbus_server in instance log maki marshal misc out plugin server user
HEADERS    = $(COMPONENTS:%=%.h) dbus_glue.h
OBJECTS    = $(COMPONENTS:%=%.o)

all: maki sushi-remote
	$(MAKE) -C plugins $@

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(bindir)'
	$(INSTALL) -m 755 maki '$(DESTDIR)$(bindir)'
	$(INSTALL) -m 755 sushi-remote '$(DESTDIR)$(bindir)'

	$(MAKE) -C plugins $@

clean:
	$(MAKE) -C plugins $@

	$(RM) -f maki
	$(RM) -f $(OBJECTS)
	$(RM) -f dbus_glue.h marshal.list marshal.c marshal.h
	$(RM) -f sushi-remote.o
	$(RM) -f libsashimi.so

dbus_glue.h: ../data/dbus.xml
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

sushi-remote.o: sushi-remote.c
	$(QUIET_CC) $(CC) $(SUSHI_REMOTE_CFLAGS) -c -o $@ $+

sushi-remote: sushi-remote.o
	$(QUIET_LD) $(CC) $(SUSHI_REMOTE_LDFLAGS) -o $@ $+

libsashimi.so: sashimi.c
	$(QUIET_CC) $(CC) $(SUSHI_CFLAGS) -fPIC $(SUSHI_LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
