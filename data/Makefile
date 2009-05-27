include ../Makefile.common

dbusdir = $(shell $(PKG_CONFIG) --variable session_bus_services_dir dbus-1)

all:

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(dbusdir)'
	$(INSTALL) -d -m 755 '$(DESTDIR)$(mandir)/man1'
	-$(SED) 's#@bindir@#$(bindir)#' 'de.ikkoku.sushi.service' > '$(DESTDIR)$(dbusdir)/de.ikkoku.sushi.service'
	$(SED) 's#@SUSHI_VERSION@#$(SUSHI_VERSION)#' 'maki.1.in' | $(GZIP) > '$(DESTDIR)$(mandir)/man1/maki.1.gz'

clean:
