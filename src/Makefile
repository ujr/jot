.POSIX:

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Og -g -I../lib/lua54
LDFLAGS = -L../lib/lua54
LDLIBS  = -llua -lm -ldl

JOTSRC = main.c jotlib.c log.c cmdargs.c pikchr.c wildmatch.c walkdir.c pathbuf.c blob.c utils.c memory.c pathlib.c loglib.c mkdn.c mkdnhtml.c
JOTINC = jot.h jotlib.h log.h cmdargs.h pikchr.h wildmatch.h walkdir.h pathbuf.h blob.h utils.h memory.h mkdn.h

all: jot

check checks: jot
	./jot -v checks || true  # avoid error message from make

jot: $(JOTSRC) $(JOTINC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(JOTSRC) $(LDLIBS)

pikchr: pikchr.c
	$(CC) -DPIKCHR_SHELL -o $@ $? -lm

mkdn: mkdn.h mkdn.c mkdnhtml.c
	$(CC) -Wall -Wextra -O0 -g -o $@ -DMKDN_SHELL mkdn.c mkdnhtml.c blob.c utils.c memory.c log.c pikchr.c -lm

clean:
	rm -f *.o jot mkdn pikchr

.PHONY: clean
