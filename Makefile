PREFIX=/usr/local

CFLAGS = -D_FILE_OFFSET_BITS=64 -O3 -std=c99 -Wall -Wextra -g

OBJECTS = lcs.o main.o draw.o io.o util.o

bxd: $(OBJECTS)
	$(CC) -o $@ $^ -ltermbox

distclean: clean

clean:
	@rm -f $(OBJECTS) bxd

install: bxd
	@install -v bxd $(PREFIX)/bin/

uninstall:
	@rm -fv $(PREFIX)/bin/bxd

all: bxd
