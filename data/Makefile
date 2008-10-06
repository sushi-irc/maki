include ../../Makefile.common

dbusdir = $(shell pkg-config --variable session_bus_services_dir dbus-1)

all:

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(dbusdir)'
	-$(SED) 's#@bindir@#$(bindir)#' 'de.ikkoku.sushi.service' > '$(DESTDIR)$(dbusdir)/de.ikkoku.sushi.service'

clean:
