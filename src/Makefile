.POSIX:

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Og -g -I../lib/lua54
LDFLAGS = -L../lib/lua54
LDLIBS  = -llua -lm -ldl

JOTSRC = main.c jotlib.c log.c cmdargs.c pikchr.c wildmatch.c walkdir.c pathbuf.c blob.c utils.c pathlib.c loglib.c mkdn.c mkdnhtml.c
JOTINC = jot.h jotlib.h log.h cmdargs.h pikchr.h wildmatch.h walkdir.h pathbuf.h blob.h utils.h mkdn.h

all: jot

jot: $(JOTSRC) $(JOTINC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(JOTSRC) $(LDLIBS)

pikchr: pikchr.c
	$(CC) -DPIKCHR_SHELL -o $@ $? -lm

mkdn: mkdn.h mkdn.c mkdnhtml.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -DMKDN_SHELL mkdn.c mkdnhtml.c blob.c utils.c log.c pikchr.c -lm

clean:
	rm -f *.o jot pikchr

.PHONY: clean
