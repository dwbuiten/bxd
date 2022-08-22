TARGET = bxd
PREFIX=/usr/local

CFLAGS = -D_FILE_OFFSET_BITS=64 -O3 -std=c99 -Wall -Wextra -g
LDLIBS = -ltermbox
OBJECTS = src/lcs.o src/draw.o src/io.o src/util.o src/main.o
RM = @rm -f
INSTALL = @install -v

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDLIBS)

distclean: clean

clean:
	$(RM) $(OBJECTS) $(TARGET)

install: bxd
	$(INSTALL) $(TARGET) $(PREFIX)/bin/

uninstall:
	$(RM) $(PREFIX)/bin/$(TARGET)

