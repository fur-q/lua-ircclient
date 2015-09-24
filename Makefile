LIBNAME := ircclient.so

PKGCONF ?= pkg-config
LUAVER  ?= 5.2
LUAPC   ?= lua$(LUAVER)

CPPFLAGS := -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -fPIC `$(PKGCONF) --cflags $(LUAPC)`
LDFLAGS  ?= -shared
LDADD    ?= -lircclient

PREFIX ?= /usr/local
LIBDIR := $(PREFIX)/lib/lua/$(LUAVER)

.PHONY: all install clean

all: $(LIBNAME)

$(LIBNAME): ircclient.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

install: $(LIBNAME)
	mkdir -p $(LIBDIR)
	cp $(LIBNAME) $(LIBDIR)

clean:
	$(RM) $(LIBNAME)
