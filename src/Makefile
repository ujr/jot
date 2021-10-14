.POSIX:

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Og -g -I../lib/lua54
LDFLAGS = -L../lib/lua54
LDLIBS  = -llua -lm -ldl

JOTSRC = main.c jotlib.c log.c cmdargs.c pikchr.c wildmatch.c
JOTINC = jot.h jotlib.h log.h cmdargs.h pikchr.h wildmatch.h

all: jot

jot: $(JOTSRC) $(JOTINC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(JOTSRC) $(LDLIBS)

pikchr: pikchr.c
	$(CC) -DPIKCHR_SHELL -o $@ $? -lm

clean:
	rm -f *.o jot pikchr

.PHONY: clean
