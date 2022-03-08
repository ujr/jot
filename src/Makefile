.POSIX:

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -Og -g -I../lib/lua54
LDFLAGS = -L../lib/lua54
LDLIBS  = -llua -lm -ldl

JOTSRC = main.c jotlib.c log.c cmdargs.c pikchr.c wildmatch.c walkdir.c blob.c utils.c memory.c pathlib.c loglib.c markdown.c mkdnhtml.c
JOTINC = jot.h jotlib.h log.h cmdargs.h pikchr.h wildmatch.h walkdir.h blob.h utils.h memory.h markdown.h

all: jot

check checks: jot
	./jot checks || true  # avoid error message from make

jot: $(JOTSRC) $(JOTINC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(JOTSRC) $(LDLIBS)

pikchr: pikchr.c
	$(CC) $(CFLAGS) -DPIKCHR_SHELL -o $@ $? -lm

mkdn: markdown.h markdown.c mkdnhtml.c
	$(CC) $(CFLAGS) -o $@ -DMKDN_SHELL markdown.c mkdnhtml.c blob.c utils.c memory.c log.c pikchr.c -lm

clean:
	rm -f *.o jot mkdn pikchr

.PHONY: clean
