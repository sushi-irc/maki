include ../../Makefile.common

PO = $(wildcard *.po)
C  = $(wildcard ../*.c)
H  = $(wildcard ../*.h)

all: $(PO:%.po=%.mo)

install: all
	for file in $(PO:%.po=%.mo); do \
		../../tools/install-mo.sh maki $${file} $(localedir); \
	done

clean:
	$(RM) maki.pot
	$(RM) *.mo

update: $(wildcard *.po)

maki.pot: $(C) $(H)
	$(QUIET_GEN) xgettext --keyword=_ --keyword=N_ --output=$@ $(C) $(H)

%.po: maki.pot
	$(QUIET_GEN) if test -f $@; then \
		touch $@; \
		msgmerge --update $@ $<; \
	else \
		msginit --input=$< --output=$@; \
	fi

%.mo: %.po
	$(QUIET_GEN) msgfmt --output-file=$@ $+
