include ../../Makefile.common

PO = $(wildcard *.po)
C  = $(wildcard ../*.c)
H  = $(wildcard ../*.h)

all: $(PO:%.po=%.mo)

install: all
	for file in $(PO:%.po=%.mo); do \
		../../tools/install-mo.sh maki $${file} $(sharedir)/locale; \
	done

clean:
	$(RM) *.pot
	$(RM) *.mo
	$(RM) -r locale

update: $(wildcard *.po)

maki.pot: $(C) $(H)
	xgettext --keyword=_ --keyword=N_ --output=$@ $(C) $(H)

%.po: maki.pot
	if test -f $@; then \
		msgmerge --update $@ $<; \
	else \
		msginit --input=$< --output=$@; \
	fi

%.mo: %.po
	msgfmt --output-file=$@ $+
	../../tools/install-mo.sh maki $@ locale
