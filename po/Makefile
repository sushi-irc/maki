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
	$(RM) -r locale

update: $(wildcard *.po)

maki.pot: $(C) $(H)
	xgettext --keyword=_ --keyword=N_ --output=$@ $(C) $(H)

%.po: maki.pot
	if test -f $@; then \
		touch $@; \
		msgmerge --update $@ $<; \
	else \
		msginit --input=$< --output=$@; \
	fi

%.mo: %.po
	msgfmt --output-file=$@ $+
	../../tools/install-mo.sh maki $@ locale
