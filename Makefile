PKGC   ?= pkg-config
LUA_PC ?= lua5.2

CPPFLAGS := -std=c99 -Wall -pedantic -fPIC `$(PKGC) --cflags $(LUA_PC)`
LDADD    := -lircclient

ircclient.so: ircclient.c
	$(CC) -shared $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

clean:
	$(RM) ircclient.so
