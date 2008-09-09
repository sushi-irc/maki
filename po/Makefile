include ../../Makefile.common

LANGUAGES = de
PO        = $(LANGUAGES:%=%.po)
C         = $(wildcard ../*.c)
H         = $(wildcard ../*.h)

all:

install: $(PO:%.po=%.mo)
	for file in $+; do \
		../../tools/install-mo.sh maki "$${file}" '$(DESTDIR)$(localedir)'; \
	done

clean:
	$(RM) maki.pot
	$(RM) *.mo

maki.pot: $(C) $(H)
	$(QUIET_GEN) xgettext --from-code=UTF-8 --keyword=_ --keyword=N_ --output=$@ $+

%.mo: %.po
	$(QUIET_GEN) msgfmt --output-file=$@ $+
