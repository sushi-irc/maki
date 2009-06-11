include ../Makefile.common

all:

install: all
	$(INSTALL) -d -m 755 '$(DESTDIR)$(docdir)/sushi/maki'
	$(INSTALL) -d -m 755 '$(DESTDIR)$(docdir)/sushi/maki/irc'
	$(INSTALL) -d -m 755 '$(DESTDIR)$(docdir)/sushi/maki/os'
	$(INSTALL) -m 644 *.txt '$(DESTDIR)$(docdir)/sushi/maki'
	$(INSTALL) -m 644 irc/*.txt '$(DESTDIR)$(docdir)/sushi/maki/irc'
	$(INSTALL) -m 644 os/*.txt '$(DESTDIR)$(docdir)/sushi/maki/os'

clean:
