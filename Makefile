include ../Makefile.common

CFLAGS  += -I../sashimi $(shell pkg-config --cflags dbus-1 glib-2.0) -fgnu89-inline
LDFLAGS += -L../sashimi $(shell pkg-config --libs dbus-1 glib-2.0) -lsashimi

SOURCES = $(wildcard maki_*.c)
OBJECTS = maki.o $(SOURCES:%.c=%.o)

all: maki

clean:
	$(RM) maki

$(OBJECTS): %.o: %.c
	$(QUIET_CC) $(CC) $(CFLAGS) -c -o $@ $+

maki: $(OBJECTS)
	$(QUIET_LD) $(CC) $(LDFLAGS) -o $@ $+