include ../../Makefile.common

SUSHI_CFLAGS  += $(shell $(PKG_CONFIG) --cflags glib-2.0 gmodule-2.0) -I..
SUSHI_LDFLAGS += $(shell $(PKG_CONFIG) --libs   glib-2.0 gmodule-2.0)

PLUGINS =

ifeq ($(shell if $(PKG_CONFIG) --exists libnm_glib && $(PKG_CONFIG) --atleast-version=0.7.0 libnm_glib; then echo yes; fi), yes)
	PLUGINS += nm
	NM_CFLAGS  = $(shell $(PKG_CONFIG) --cflags gobject-2.0 libnm_glib)
	NM_LDFLAGS = $(shell $(PKG_CONFIG) --libs   gobject-2.0 libnm_glib)
endif

LIBRARIES  = $(PLUGINS:%=lib%.so)

all: $(LIBRARIES)

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(libdir)/sushi/maki/plugins'
	test -z '$(LIBRARIES)' || $(INSTALL) -m 644 $(LIBRARIES) '$(DESTDIR)$(libdir)/sushi/maki/plugins'

clean:
	$(RM) -f $(LIBRARIES)

libnm.so: nm.c
	$(QUIET_LD) $(CC) $(SUSHI_CFLAGS) $(NM_CFLAGS) -fPIC $(SUSHI_LDFLAGS) $(NM_LDFLAGS) -shared -Wl,-soname,$@ -o $@ $+
