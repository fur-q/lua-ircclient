#ifndef __LUAIRCCLIENT_H__
#define __LUAIRCCLIENT_H__

#include <libircclient.h>

struct lsession {
    irc_session_t * session;
};

struct callback {
    lua_State * L;
    int ref;
};

#endif
