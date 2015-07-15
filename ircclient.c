#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "ircclient.h"

#if LUA_VERSION_NUM > 501
#define REGISTER(L, r) luaL_setfuncs(L, r, 0)
#else
#define REGISTER(L, r) luaL_register(L, NULL, r)
#endif

/* LIBRARY FUNCTIONS */
// TODO: irc_option_set, irc_option_reset

static int l_get_version(lua_State * L) {
    unsigned int high, low;
    irc_get_version(&high, &low);
    lua_pushinteger(L, high);
    lua_pushinteger(L, low);
    return 2;
}

static int l_target_get_nick(lua_State *L) {
    const char *origin = luaL_checkstring(L, 1);
    char buffer[128];
    size_t size = 128;
    irc_target_get_nick(origin, buffer, size);
    lua_pushstring(L, buffer);
    return 1;
}

static int l_target_get_host(lua_State *L) {
    const char *origin = luaL_checkstring(L, 1);
    char buffer[128];
    size_t size = 128;
    irc_target_get_host(origin, buffer, size);
    lua_pushstring(L, buffer);
    return 1;
}

static int l_color_strip_from_mirc(lua_State * L) {
    const char * message = luaL_checkstring(L, 1);
    char * out = irc_color_strip_from_mirc(message);
    lua_pushstring(L, out);
    free(out);
    return 1;
}

static int l_color_convert_from_mirc(lua_State * L) {
    const char * message = luaL_checkstring(L, 1);
    char * out = irc_color_convert_from_mirc(message);
    lua_pushstring(L, out);
    free(out);
    return 1;
}

/* CALLBACK WRAPPERS */
// FIXME DRY

void callback_generic(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    struct callback * cb = irc_get_ctx(session);
    if (cb == 0)
        return;
    lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->ref);
    // lua_rawgetp(L, -2, (void *)cb);
    lua_pushlightuserdata(cb->L, (void *)cb);
    lua_gettable(cb->L, -2);
    luaL_getmetafield(cb->L, -1, "__callbacks");
    lua_getfield(cb->L, -1, event);
    if (lua_isnil(cb->L, -1))
        goto cleanup;
    lua_pushstring(cb->L, origin);
    for (int i = 0; i < count; i++) {
        lua_pushstring(cb->L, params[i]);
    }
    lua_pcall(cb->L, count+1, LUA_MULTRET, 0);
cleanup:
    lua_pop(cb->L, 3);
}

void callback_numeric(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
    struct callback * cb = irc_get_ctx(session);
    if (cb == 0)
        return;
    lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->ref);
    // lua_rawgetp(L, -2, (void *)cb);
    lua_pushlightuserdata(cb->L, (void *)cb);
    lua_gettable(cb->L, -2);
    luaL_getmetafield(cb->L, -1, "__callbacks");
    lua_rawgeti(cb->L, -1, event);
    if (lua_isnil(cb->L, -1))
        goto cleanup;
    lua_pushstring(cb->L, origin);
    for (int i = 0; i < count; i++) {
        lua_pushstring(cb->L, params[i]);
    }
    lua_pcall(cb->L, count+1, LUA_MULTRET, 0);
cleanup:
    lua_pop(cb->L, 3);
}

/* SESSION FUNCTIONS */
// TODO irc_send_raw, select_descriptors, dcc

static irc_session_t * session_get(lua_State * L) {
    struct lsession * ls = luaL_checkudata(L, 1, "irc_session");
    return ls->session;
}

static int session_connect(lua_State * L) {
    irc_session_t * session = session_get(L);
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
        lua_pushboolean(L, !irc_connect6(session, host, port, pass, nick, user, real));
    else
        lua_pushboolean(L, !irc_connect(session, host, port, pass, nick, user, real));
    return 1;
}

static int session_disconnect(lua_State * L) {
    irc_session_t * session = session_get(L);
    irc_disconnect(session);
    return 0;
}

static int session_is_connected(lua_State * L) {
    irc_session_t * session = session_get(L);
    lua_pushboolean(L, !irc_is_connected(session));
    return 1;
}

static int session_cmd_join(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * key = luaL_optstring(L, 3, 0);
    lua_pushboolean(L, !irc_cmd_join(session, chan, key));
    return 1;
}

static int session_cmd_part(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_checkstring(L, 2);
    lua_pushboolean(L, !irc_cmd_part(session, chan));
    return 1;
}

static int session_cmd_invite(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * nick = luaL_checkstring(L, 2);
    const char * chan = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_invite(session, nick, chan));
    return 1;
}

static int session_cmd_names(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_checkstring(L, 2);
    lua_pushboolean(L, !irc_cmd_names(session, chan));
    return 1;
}

static int session_cmd_list(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_optstring(L, 2, 0);
    lua_pushboolean(L, !irc_cmd_list(session, chan));
    return 1;
}

static int session_cmd_topic(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * topic = luaL_optstring(L, 3, 0);
    lua_pushboolean(L, !irc_cmd_topic(session, chan, topic));
    return 1;
}

static int session_cmd_channel_mode(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * chan = luaL_checkstring(L, 2);
    const char * mode = luaL_optstring(L, 3, 0);
    lua_pushboolean(L, !irc_cmd_channel_mode(session, chan, mode));
    return 1;
}

static int session_cmd_user_mode(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * mode = luaL_optstring(L, 2, 0);
    lua_pushboolean(L, !irc_cmd_user_mode(session, mode));
    return 1;
}

static int session_cmd_kick(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * nick = luaL_checkstring(L, 2);
    const char * chan = luaL_checkstring(L, 3);
    const char * reason = luaL_optstring(L, 4, 0);
    lua_pushboolean(L, !irc_cmd_kick(session, nick, chan, reason));
    return 1;
}

static int session_cmd_msg(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_msg(session, target, msg));
    return 1;
}

static int session_cmd_me(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_me(session, target, msg));
    return 1;
}

static int session_cmd_notice(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_notice(session, target, msg));
    return 1;
}

static int session_cmd_ctcp_request(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_ctcp_request(session, target, msg));
    return 1;
}

static int session_cmd_ctcp_reply(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * target = luaL_checkstring(L, 2);
    const char * msg = luaL_checkstring(L, 3);
    lua_pushboolean(L, !irc_cmd_ctcp_reply(session, target, msg));
    return 1;
}

static int session_cmd_nick(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * newnick = luaL_checkstring(L, 2);
    lua_pushboolean(L, !irc_cmd_nick(session, newnick));
    return 1;
}

// FIXME takes comma-separated list; should join multiple args
static int session_cmd_whois(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * nick = luaL_checkstring(L, 2);
    lua_pushboolean(L, !irc_cmd_whois(session, nick));
    return 1;
}

static int session_cmd_quit(lua_State * L) {
    irc_session_t * session = session_get(L);
    const char * reason = luaL_optstring(L, 2, 0);
    lua_pushboolean(L, !irc_cmd_quit(session, reason));
    return 1;
}

static int session_run(lua_State * L) {
    irc_session_t * session = session_get(L);
    lua_pushboolean(L, !irc_run(session));
    return 1;
}

static int session_strerror(lua_State * L) {
    irc_session_t * session = session_get(L);
    lua_pushstring(L, irc_strerror(irc_errno(session)));
    return 1;
}

static int session_set_callback(lua_State * L) {
    session_get(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    luaL_getmetafield(L, 1, "__callbacks");
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "upper");
    lua_pushvalue(L, 2);
    lua_pcall(L, 1, 1, 0);
    lua_pushvalue(L, 3);
    lua_settable(L, 4);
    return 0;
}

static int session_destroy(lua_State * L) {
    irc_session_t * session = session_get(L);
    struct callback * ctx = irc_get_ctx(session);
    free(ctx);
    irc_destroy_session(session);
    return 0;
}

static int session_create(lua_State * L) {
    irc_callbacks_t cbx = {
        callback_generic, callback_generic, callback_generic, callback_generic,
        callback_generic, callback_generic, callback_generic, callback_generic,
        callback_generic, callback_generic, callback_generic, callback_generic,
        callback_generic, callback_generic, callback_generic, callback_generic,
        callback_generic, callback_generic, callback_numeric 
    };
    struct lsession * ls = lua_newuserdata(L, sizeof(struct lsession *));
    ls->session = irc_create_session(&cbx);
    if (!ls->session) {
        lua_pushnil(L);
        lua_pushstring(L, irc_strerror(irc_errno(0)));
        return 2;
    }
    // set context
    int tag = lua_tonumber(L, lua_upvalueindex(1));
    struct callback * cb = malloc(sizeof(struct callback));
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
        { "run", session_run },
        { "register", session_set_callback },
        { "strerror", session_strerror },
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
        { "me", session_cmd_me },
        { "notice", session_cmd_notice },
        { "ctcp_request", session_cmd_ctcp_request },
        { "ctcp_reply", session_cmd_ctcp_reply },
        { "nick", session_cmd_nick },
        { "whois", session_cmd_whois },
        { "quit", session_cmd_quit },
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
    lua_newtable(L);
    lua_setfield(L, -2, "__callbacks");
    lua_setmetatable(L, -2);
    return 1;
}

int luaopen_ircclient(lua_State * L) {
    static const luaL_Reg ircclient[] = {
        { "version", l_get_version },
        { "color_strip", l_color_strip_from_mirc },
        { "color_convert", l_color_convert_from_mirc },
        { "get_nick", l_target_get_nick },
        { "get_host", l_target_get_host },
        { NULL, NULL }
    };
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    int tag = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_newtable(L);
    REGISTER(L, ircclient);
    lua_pushnumber(L, tag);
    lua_pushcclosure(L, session_create, 1);
    lua_setfield(L, -2, "create_session");
    return 1;
}

