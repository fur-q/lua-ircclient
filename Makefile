PKGC ?= pkg-config
LUA  ?= lua5.2

CPPFLAGS := -std=c99 -Wall -pedantic -fPIC `$(PKGC) --cflags $(LUA)`
LDFLAGS  += -shared
LDADD    := -lircclient

ircclient.so: ircclient.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

clean:
	$(RM) ircclient.so
