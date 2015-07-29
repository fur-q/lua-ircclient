#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <libircclient.h>

#if LUA_VERSION_NUM > 501
#define REGISTER(L, idx) luaL_setfuncs(L, idx, 0)
#else
#define REGISTER(L, idx) luaL_register(L, 0, idx)
#define lua_rawlen(L, idx) lua_objlen(L, idx)
#define lua_getuservalue(L, idx) lua_getfenv(L, idx)
#define lua_setuservalue(L, idx) lua_setfenv(L, idx)
#define lua_rawgetp(L, idx, ptr) lua_pushlightuserdata(L, ptr); lua_gettable(L, (idx) - 1)
#endif

struct lsession {
    irc_session_t * session;
};

struct cb_ctx {
    lua_State * L;
    int ref;
};

/* LIBRARY FUNCTIONS */

static int lib_get_version(lua_State * L) {
    unsigned int high, low;
    irc_get_version(&high, &low);
    lua_pushinteger(L, high);
    lua_pushinteger(L, low);
    return 2;
}

static int lib_strerror(lua_State * L) {
    int err = luaL_checkint(L, 1);
    lua_pushstring(L, irc_strerror(err));
    return 1;
}

static inline int util_target(lua_State * L, void (func)(const char *, char *, size_t)) {
    const char * origin = luaL_checkstring(L, 1);
    unsigned int bufsize = luaL_optint(L, 2, 128);
    char buffer[bufsize];
    func(origin, buffer, bufsize);
    lua_pushstring(L, buffer);
    return 1;
}

static int lib_target_get_nick(lua_State * L) {
    return util_target(L, irc_target_get_nick);
}

static int lib_target_get_host(lua_State * L) {
    return util_target(L, irc_target_get_host);
}

static inline int util_color(lua_State * L, char * (func)(const char *)) {
    const char * message = luaL_checkstring(L, 1);
    char * out = func(message);
    lua_pushstring(L, out);
    free(out);
    return 1;
}

static int lib_color_strip_from_mirc(lua_State * L) {
    return util_color(L, irc_color_strip_from_mirc);
}

static int lib_color_convert_from_mirc(lua_State * L) {
    return util_color(L, irc_color_convert_from_mirc);
}

static int lib_color_convert_to_mirc(lua_State * L) {
    return util_color(L, irc_color_convert_to_mirc);
}

/* CALLBACK WRAPPERS */

static inline lua_State * util_getsession_cb(irc_session_t * session) {
    struct cb_ctx * cb = irc_get_ctx(session);
    if (!cb)
        return 0;
    lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->ref);
    lua_rawgetp(cb->L, -1, (void *)cb);
    lua_getuservalue(cb->L, -1);
    return cb->L;
}

static void cb_event(irc_session_t * session, const char * evt_s, unsigned int evt_i, const char * origin,
              const char ** params, unsigned int count) {
    lua_State * L = util_getsession_cb(session);
    if (!L)
        return;
    lua_getfield(L, -1, "events");
    if (evt_s) 
        lua_getfield(L, -1, evt_s);
    else
        lua_rawgeti(L, -1, evt_i);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 5);
        return;
    }
    lua_pushvalue(L, -4);
    lua_pushstring(L, origin);
    for (int i = 0; i < count; i++)
        lua_pushstring(L, params[i]);
    if (lua_pcall(L, count+2, 0, 0)) {
        // FIXME do something with this error
        lua_pop(L, 1);
    }
    lua_pop(L, 4);
}

static void cb_event_dcc(irc_session_t * session, const char * evt, const char * nick, const char * addr, 
                  const char * filename, unsigned long size, irc_dcc_t id) {
    lua_State * L = util_getsession_cb(session);
    if (!L)
        return;
    lua_getfield(L, -1, "events");
    lua_getfield(L, -1, evt);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 5);
        return;
    }
    lua_pushvalue(L, -4);
    int top = lua_gettop(L) - 1;
    lua_pushstring(L, nick);
    lua_pushstring(L, addr);
    lua_pushstring(L, filename);
    if (size)
        lua_pushinteger(L, size);
    lua_pushinteger(L, id);
    if (lua_pcall(L, lua_gettop(L) - top, 0, 0)) {
        // FIXME do something with this error
        lua_pop(L, 1);
    }
    lua_pop(L, 4);
}

static void cb_eventname(irc_session_t * session, const char * event, const char * origin, 
                  const char ** params, unsigned int count) {
    cb_event(session, event, 0, origin, params, count);
}

static void cb_eventcode(irc_session_t * session, unsigned int event, const char * origin,
                  const char ** params, unsigned int count) {
    cb_event(session, 0, event, origin, params, count);
}

static void cb_event_chat(irc_session_t * session, const char * nick, const char * addr, irc_dcc_t id) {
    cb_event_dcc(session, "CHAT", nick, addr, 0, 0, id); 
}

static void cb_event_send(irc_session_t * session, const char * nick, const char * addr, 
                const char * filename, unsigned long size, irc_dcc_t id) {
    cb_event_dcc(session, "SEND", nick, addr, filename, size, id);
}

static void cb_dcc(irc_session_t * session, irc_dcc_t id, int status, void * ctx, const char * data, 
            unsigned int length) {
    lua_State * L = util_getsession_cb(session);
    if (!L)
        return;
    lua_getfield(L, -1, "dcc");
    lua_rawgeti(L, -1, id);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 5);
        return;
    }
    if (status)
        lua_pushinteger(L, status);
    else
        lua_pushboolean(L, 1);
    lua_pushinteger(L, length);
    if (data)
        lua_pushlstring(L, data, length);
    else
        lua_pushnil(L);
    if (lua_pcall(L, 3, 0, 0)) {
        // FIXME do something with this error
        lua_pop(L, 1);
    }
    if (status) {
        lua_pushnil(L);
        lua_rawseti(L, -2, id);
    }
    lua_pop(L, 4);
}

/* SESSION FUNCTIONS */

static inline irc_session_t * util_getsession(lua_State * L) {
    struct lsession * ls = luaL_checkudata(L, 1, "irc_session");
    return ls->session;
}

static inline int util_pushstatus(lua_State * L, irc_session_t * session, int status) {
    if (status) {
        lua_pushnil(L);
        lua_pushinteger(L, irc_errno(session));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

#define STATUS(ok) util_pushstatus(L, session, ok)

static int session_connect(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "host");
    lua_getfield(L, 2, "port");
    lua_getfield(L, 2, "nick");
    lua_getfield(L, 2, "user");
    lua_getfield(L, 2, "realname");
    lua_getfield(L, 2, "password");
    lua_getfield(L, 2, "ipv6");
    const char * host = luaL_checkstring(L, 3);
    unsigned int port = luaL_optnumber(L, 4, 6667);
    const char * nick = luaL_checkstring(L, 5);
    const char * user = luaL_optstring(L, 6, nick);
    const char * real = luaL_optstring(L, 7, nick);
    const char * pass = luaL_optstring(L, 8, 0);
    if (lua_toboolean(L, 9))
        return STATUS(irc_connect6(session, host, port, pass, nick, user, real));
    return STATUS(irc_connect(session, host, port, pass, nick, user, real));
}

static int session_disconnect(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    irc_disconnect(session);
    return 0;
}

static int session_is_connected(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    lua_pushboolean(L, irc_is_connected(session));
    return 1;
}

static int session_cmd_join(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * key = luaL_optstring(L, 3, 0);
    return STATUS(irc_cmd_join(session, chan, key));
}

static int session_cmd_part(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_checkstring(L, 2);
    return STATUS(irc_cmd_part(session, chan));
}

static int session_cmd_invite(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * nick = luaL_checkstring(L, 2);
    const char * chan = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_invite(session, nick, chan));
}

static int session_cmd_names(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_checkstring(L, 2);
    return STATUS(irc_cmd_names(session, chan));
}

static int session_cmd_list(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_optstring(L, 2, 0);
    return STATUS(irc_cmd_list(session, chan));
}

static int session_cmd_topic(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * topic = luaL_optstring(L, 3, 0);
    return STATUS(irc_cmd_topic(session, chan, topic));
}

static int session_cmd_channel_mode(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * mode = luaL_optstring(L, 3, 0);
    return STATUS(irc_cmd_channel_mode(session, chan, mode));
}

static int session_cmd_user_mode(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * mode = luaL_optstring(L, 2, 0);
    return STATUS(irc_cmd_user_mode(session, mode));
}

static int session_cmd_kick(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * nick = luaL_checkstring(L, 2);
    const char * chan = luaL_checkstring(L, 3);
    const char * reason = luaL_optstring(L, 4, 0);
    return STATUS(irc_cmd_kick(session, nick, chan, reason));
}

static int session_cmd_msg(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_msg(session, target, msg));
}

static int session_cmd_me(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_me(session, target, msg));
}

static int session_cmd_notice(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_notice(session, target, msg));
}

static int session_cmd_ctcp_request(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_ctcp_request(session, target, msg));
}

static int session_cmd_ctcp_reply(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_cmd_ctcp_reply(session, target, msg));
}

static int session_cmd_nick(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * newnick = luaL_checkstring(L, 2);
    return STATUS(irc_cmd_nick(session, newnick));
}

// TODO concat multiple args
static int session_cmd_whois(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * nick = luaL_checkstring(L, 2);
    return STATUS(irc_cmd_whois(session, nick));
}

static int session_cmd_quit(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * reason = luaL_optstring(L, 2, 0);
    return STATUS(irc_cmd_quit(session, reason));
}

static int session_send_raw(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    lua_getglobal(L, "string");
    lua_getfield(L, 3, "format");
    lua_insert(L, 2);
    lua_pop(L, 1);
    lua_call(L, lua_gettop(L) - 1, 1);
    const char * msg = lua_tostring(L, 2);
    return STATUS(irc_send_raw(session, msg));
}

static inline void util_dccid_add(lua_State * L, irc_dcc_t id) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "dcc");
    lua_pushvalue(L, -3);
    lua_rawseti(L, -2, id);
}

static int session_dcc_accept(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    irc_dcc_t id = luaL_checkint(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    int ok = irc_dcc_accept(session, id, 0, cb_dcc);
    if (!ok)
        util_dccid_add(L, id);
    return STATUS(ok);
}

static int session_dcc_decline(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    irc_dcc_t id = luaL_checkint(L, 2);
    return STATUS(irc_dcc_decline(session, id));
}

static int session_dcc_chat(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * nick = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    irc_dcc_t id = 0;
    if (irc_dcc_chat(session, 0, nick, cb_dcc, &id)) {
        lua_pushnil(L);
        lua_pushinteger(L, irc_errno(session));
        return 2;
    }
    util_dccid_add(L, id);
    lua_pushinteger(L, id);
    return 1;
}

static int session_dcc_msg(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    irc_dcc_t id = luaL_checkint(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    return STATUS(irc_dcc_msg(session, id, msg));
}

static int session_dcc_sendfile(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    const char * nick = luaL_checkstring(L, 2);
    const char * file = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);
    irc_dcc_t id = 0;
    if (irc_dcc_sendfile(session, 0, nick, file, cb_dcc, &id)) {
        lua_pushnil(L);
        lua_pushinteger(L, irc_errno(session));
        return 2;
    }
    util_dccid_add(L, id);
    lua_pushinteger(L, id);
    return 1;
}

static int session_dcc_destroy(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    irc_dcc_t id = luaL_checkint(L, 2);
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "dcc");
    lua_pushnil(L);
    lua_rawseti(L, -2, id);
    return STATUS(irc_dcc_destroy(session, id));
}

static int session_option_set(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    unsigned int option = luaL_checkint(L, 2);
    irc_option_set(session, option);
    return 0;
}

static int session_option_reset(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    unsigned int option = luaL_checkint(L, 2);
    irc_option_reset(session, option);
    return 0;
}

static int session_run(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    return STATUS(irc_run(session));
}

static int session_add_descriptors(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);
    int rfd_count = lua_rawlen(L, 2), wfd_count = lua_rawlen(L, 3);
    int max = 0;
    fd_set rfd, wfd;
    FD_ZERO(&rfd);
    FD_ZERO(&wfd);
    if (irc_add_select_descriptors(session, &rfd, &wfd, &max)) {
        lua_pushnil(L);
        return 1;
    }
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, &rfd)) {
            lua_pushinteger(L, i);
            lua_rawseti(L, 2, ++rfd_count);
        }
        if (FD_ISSET(i, &wfd)) {
            lua_pushinteger(L, i);
            lua_rawseti(L, 3, ++wfd_count);
        }
    }
    return 2;
}

static int session_process_descriptors(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);
    fd_set rfd, wfd;
    FD_ZERO(&rfd);
    FD_ZERO(&wfd);
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        FD_SET(lua_tointeger(L, -1), &rfd);
        lua_pop(L, 1);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        FD_SET(lua_tointeger(L, -1), &wfd);
        lua_pop(L, 1);
    }
    return STATUS(irc_process_select_descriptors(session, &rfd, &wfd));
}

static int session_set_callback(lua_State * L) {
    util_getsession(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    if (!(lua_isfunction(L, 3) || lua_isnil(L, 3))) {
        const char * msg = lua_pushfstring(L, "function expected, got %s", lua_typename(L, 3));
        return luaL_argerror(L, 3, msg);
    }
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "events");
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "upper");
    lua_pushvalue(L, 2);
    lua_call(L, 1, 1);
    lua_pushvalue(L, 3);
    lua_settable(L, 5);
    return 0;
}

static int session_destroy(lua_State * L) {
    irc_session_t * session = util_getsession(L);
    struct cb_ctx * ctx = irc_get_ctx(session);
    free(ctx);
    irc_destroy_session(session);
    return 0;
}

static int session_create(lua_State * L) {
    irc_callbacks_t cbx = {
        cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, 
        cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, 
        cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, cb_eventname, 
        cb_eventcode, cb_event_chat, cb_event_send
    };
    struct lsession * ls = lua_newuserdata(L, sizeof(struct lsession *));
    ls->session = irc_create_session(&cbx);
    if (!ls->session) {
        lua_pushnil(L);
        lua_pushstring(L, irc_strerror(irc_errno(0))); // ??
        return 2;
    }
    // set context
    int tag = lua_tonumber(L, lua_upvalueindex(1));
    struct cb_ctx * cb = malloc(sizeof(struct cb_ctx));
    cb->L = L;
    cb->ref = tag;
    irc_set_ctx(ls->session, (void *)cb);
    // set session context entry
    lua_rawgeti(L, LUA_REGISTRYINDEX, tag);
    lua_pushlightuserdata(L, (void *)cb);
    lua_pushvalue(L, 1);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    // set metatable
    static const luaL_Reg ircsession[] = {
        { "connect", session_connect },
        { "disconnect", session_disconnect },
        { "is_connected", session_is_connected },
        { "join", session_cmd_join },
        { "part", session_cmd_part },
        { "invite", session_cmd_invite },
        { "names", session_cmd_names },
        { "list", session_cmd_list },
        { "topic", session_cmd_topic },
        { "channel_mode", session_cmd_channel_mode },
        { "user_mode", session_cmd_user_mode },
        { "kick", session_cmd_kick },
        { "msg", session_cmd_msg },
        { "notice", session_cmd_notice },
        { "me", session_cmd_me },
        { "ctcp_action", session_cmd_me },
        { "ctcp_request", session_cmd_ctcp_request },
        { "ctcp_reply", session_cmd_ctcp_reply },
        { "nick", session_cmd_nick },
        { "whois", session_cmd_whois },
        { "quit", session_cmd_quit },
        { "send_raw", session_send_raw },
        { "dcc_decline", session_dcc_decline },
        { "dcc_accept", session_dcc_accept },
        { "dcc_sendfile", session_dcc_sendfile },
        { "dcc_chat", session_dcc_chat },
        { "dcc_msg", session_dcc_msg },
        { "dcc_destroy", session_dcc_destroy },
        { "add_descriptors", session_add_descriptors },
        { "process_descriptors", session_process_descriptors },
        { "run", session_run },
        { "option_set", session_option_set },
        { "option_reset", session_option_reset },
        { "register", session_set_callback },
        { NULL, NULL }
    };
    luaL_newmetatable(L, "irc_session");
    lua_newtable(L);
    REGISTER(L, ircsession);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, session_set_callback);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, session_destroy);
    lua_setfield(L, -2, "__gc");
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    lua_setmetatable(L, -2);
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "events");
    lua_newtable(L);
    lua_setfield(L, -2, "dcc");
    lua_setuservalue(L, -2);
    return 1;
}

/* SETUP */

static inline void util_setconst(lua_State * L, const char * name, int which) {
    lua_pushinteger(L, which);
    lua_setfield(L, -2, name);
    lua_pushstring(L, name);
    lua_rawseti(L, -2, which);
}

static inline void util_seterrors(lua_State * L) {
    lua_newtable(L);
    util_setconst(L, "INVAL", LIBIRC_ERR_INVAL);
    util_setconst(L, "RESOLV", LIBIRC_ERR_RESOLV);
    util_setconst(L, "SOCKET", LIBIRC_ERR_SOCKET);
    util_setconst(L, "CONNECT", LIBIRC_ERR_CONNECT);
    util_setconst(L, "CLOSED", LIBIRC_ERR_CLOSED);
    util_setconst(L, "NOMEM", LIBIRC_ERR_NOMEM);
    util_setconst(L, "ACCEPT", LIBIRC_ERR_ACCEPT);
    util_setconst(L, "NODCCSEND", LIBIRC_ERR_NODCCSEND);
    util_setconst(L, "READ", LIBIRC_ERR_READ);
    util_setconst(L, "WRITE", LIBIRC_ERR_WRITE);
    util_setconst(L, "STATE", LIBIRC_ERR_STATE);
    util_setconst(L, "TIMEOUT", LIBIRC_ERR_TIMEOUT);
    util_setconst(L, "OPENFILE", LIBIRC_ERR_OPENFILE);
    util_setconst(L, "TERMINATED", LIBIRC_ERR_TERMINATED);
    util_setconst(L, "NOIPV6", LIBIRC_ERR_NOIPV6);
    util_setconst(L, "SSL_NOT_SUPPORTED", LIBIRC_ERR_SSL_NOT_SUPPORTED);
    util_setconst(L, "SSL_INIT_FAILED", LIBIRC_ERR_SSL_INIT_FAILED);
    util_setconst(L, "CONNECT_SSL_FAILED", LIBIRC_ERR_CONNECT_SSL_FAILED);
    util_setconst(L, "SSL_CERT_VERIFY_FAILED", LIBIRC_ERR_SSL_CERT_VERIFY_FAILED);
    lua_setfield(L, -2, "errors");
}

static inline void util_setoptions(lua_State * L) {
    lua_newtable(L);
    util_setconst(L, "DEBUG", LIBIRC_OPTION_DEBUG);
    util_setconst(L, "STRIPNICKS", LIBIRC_OPTION_STRIPNICKS);
    util_setconst(L, "SSL_NO_VERIFY", LIBIRC_OPTION_SSL_NO_VERIFY);
    lua_setfield(L, -2, "options");
}

int luaopen_ircclient(lua_State * L) {
    static const luaL_Reg ircclient[] = {
        { "version", lib_get_version },
        { "strerror", lib_strerror },
        { "color_strip", lib_color_strip_from_mirc },
        { "color_convert_from_mirc", lib_color_convert_from_mirc },
        { "color_convert_to_mirc", lib_color_convert_to_mirc },
        { "get_nick", lib_target_get_nick },
        { "get_host", lib_target_get_host },
        { NULL, NULL }
    };
    lua_newtable(L);
    lua_newtable(L);
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    int tag = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    REGISTER(L, ircclient);
    util_seterrors(L);
    util_setoptions(L);
    lua_pushnumber(L, tag);
    lua_pushcclosure(L, session_create, 1);
    lua_setfield(L, -2, "create_session");
    return 1;
}

