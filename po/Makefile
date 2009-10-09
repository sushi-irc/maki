include ../Makefile.common

LANGUAGES = ar de es
PO        = $(LANGUAGES:%=%.po)
C         = $(wildcard ../source/*.c)
H         = $(wildcard ../source/*.h)

all: maki.pot

install: $(PO:%.po=%.mo)
	for file in $+; do \
		INSTALL='$(INSTALL)' ../tools/install-mo.sh maki "$${file}" '$(DESTDIR)$(localedir)'; \
	done

clean:
	$(RM) -f *.mo

maki.pot: $(C) $(H)
	$(QUIET_GEN) xgettext --from-code=UTF-8 --keyword=_ --keyword=N_ --output=$@.tmp $+; \
		if $(MSGCMP) --use-untranslated $@ $@.tmp; \
		then \
			$(RM) -f $@.tmp; \
			$(TOUCH) $@; \
		else \
			$(MV) $@.tmp $@; \
		fi

%.mo: %.po
	$(QUIET_GEN) msgfmt --output-file=$@ $+
