-- Multi-server example using https://github.com/Neopallium/lua-epoll

local irc = require "ircclient"
local epoll = require "epoll"
local bit = bit32 or require "bit"

local s1 = irc.create_session()
s1:register("connect", function(s) s:join("#test") end)
s1:register("channel", function(s, origin, chan, text)
    if text:match("^foo") then
        s:msg(chan, "bar")
    end
end)
s1:connect{ host = "127.0.0.1", nick = "test1" }

local s2 = irc.create_session()
s2:register("connect", function(s) s:join("#test") end)
s2:register("channel", function(s, origin, chan, text)
    if text:match("^bar") then
        s:msg(chan, "baz")
    end
end)
s2:connect{ host = "127.0.0.1", nick = "test2" }

local loop = epoll.new()

local addfd = function(t, evt)
    evt = bit.bor(evt, epoll.EPOLLONESHOT)
    for i, v in ipairs(t) do
        local ok, err = loop:add(v, evt, v)
        if not ok and err == "EEXIST" then
            loop:mod(v, evt, v)
        end
    end
end

local cb = function(fd, evt)
    local rfd, wfd = {}, {}
    if evt == epoll.EPOLLIN then
        rfd[1] = fd
    elseif evt == epoll.EPOLLOUT then
        wfd[1] = fd
    end
    s1:process_descriptors(rfd, wfd)
    s2:process_descriptors(rfd, wfd)
end

while true do
    local rfd1, wfd1 = s1:add_descriptors({}, {})
    local rfd2, wfd2 = s2:add_descriptors({}, {})
    addfd(rfd1, epoll.EPOLLIN)
    addfd(rfd2, epoll.EPOLLIN)
    addfd(wfd1, epoll.EPOLLOUT)
    addfd(wfd2, epoll.EPOLLOUT)
    loop:wait_callback(cb, -1)
end

