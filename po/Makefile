include ../../Makefile.common

PO = $(wildcard *.po)
C  = $(wildcard ../*.c)
H  = $(wildcard ../*.h)

all:

install: $(PO:%.po=%.mo)
	for file in $+; do \
		../../tools/install-mo.sh maki "$${file}" '$(DESTDIR)$(localedir)'; \
	done

clean:
	$(RM) maki.pot
	$(RM) *.mo

maki.pot: $(C) $(H)
	$(QUIET_GEN) xgettext --keyword=_ --keyword=N_ --output=$@ $+

%.po: maki.pot
	$(QUIET_GEN) if test -f $@; then \
		touch $@; \
		msgmerge --update $@ $+; \
	else \
		msginit --input=$+ --output=$@; \
	fi

%.mo:
	$(QUIET_GEN) msgfmt --output-file=$@ $(subst .mo,.po,$@)
