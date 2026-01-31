include config.mk

CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -g -Iinclude -Isrc

BIN = xf
SRC = src
OBJ = obj

# Discover all source files
SRCS = $(wildcard $(SRC)/*.c)

# Create matching build/*.o paths
OBJS = $(SRCS:$(SRC)/%.c=$(OBJ)/%.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ):
	mkdir -p $@

# Pattern rule: obj/xyz.o ← src/xyz.c
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BIN) $(OBJ)/*

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f $(BIN) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(BIN)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f $(BIN).1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/$(BIN).1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MANDIR)/man1/$(BIN).1

.PHONY: all clean install uninstall
