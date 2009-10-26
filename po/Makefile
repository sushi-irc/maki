include ../Makefile.common

LANGUAGES = ar de es pt
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
		if $(DIFF) -u $@ $@.tmp | $(GREP) '^[+-]msg' > /dev/null 2>&1; \
		then \
			$(MV) $@.tmp $@; \
		else \
			$(RM) -f $@.tmp; \
			$(TOUCH) $@; \
		fi

%.mo: %.po
	$(QUIET_GEN) msgfmt --output-file=$@ $+
