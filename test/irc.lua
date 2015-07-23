local irc = require "ircclient"
local assert = require "test.assert"

local target = "foo!~bar@baz.quux"
local col_int = "[B]foo[/B] [U]bar[/U] [COLOR=RED]baz[/COLOR]"
local col_mirc = "\2foo\2 \31bar\31 \00304baz\15"

assert.equal(irc.get_nick(target), "foo")
assert.equal(irc.get_host(target), "!~bar@baz.quux")
assert.equal(irc.color_convert_to_mirc(col_int), col_mirc)
assert.equal(irc.color_convert_from_mirc(col_mirc), col_int)
assert.equal(irc.color_strip(col_mirc), "foo bar baz")

assert.type(irc.errors.INVAL, "number")
assert.equal(irc.errors[irc.errors.INVAL], "INVAL")
assert.type(irc.options.DEBUG, "number")
assert.equal(irc.options[irc.options.DEBUG], "DEBUG")

local session = irc.create_session()
assert.type(session, "userdata")
local mt = debug.getmetatable(session)
local uv = (debug.getfenv or debug.getuservalue)(session)
assert.equal(mt.__metatable, false)
assert.type(uv.events, "table")
assert.type(uv.dcc, "table")

if not getfenv then -- skip test on 5.1
    local _, ref = debug.getupvalue(irc.create_session, 1)
    local reg = debug.getregistry()[ref]
    assert.type(reg, "table")
    assert.equal(getmetatable(reg).__mode, "v")
    assert.equal(select(2, next(reg)), session)
    session = nil
    collectgarbage()
    assert.equal(next(reg), nil)
end

print("All tests passed")
