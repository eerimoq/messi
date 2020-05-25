CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

LIBRARY = libmessi.a
PREFIX ?= /usr/local

.PHONY: library install clean

library: $(LIBRARY)

install:
	find include -type f -exec install -Dm 644 "{}" "$(PREFIX)/{}" \;
	mkdir -p $(PREFIX)/lib
	install -m 644 $(LIBRARY) $(PREFIX)/lib

clean:
	rm $(LIBRARY)

$(LIBRARY):
	$(CC) -Wall -O2 -Iinclude src/messi.c -c -o messi.o
	$(AR) cr $(LIBRARY) messi.o
