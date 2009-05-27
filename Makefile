include Makefile.common

all:
	$(MAKE) -C source $@

install: all
	$(MAKE) -C data $@
	$(MAKE) -C po $@
	$(MAKE) -C source $@

clean:
	$(MAKE) -C po $@
	$(MAKE) -C source $@
