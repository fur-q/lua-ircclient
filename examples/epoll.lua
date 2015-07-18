-- Multi-server example using https://github.com/Neopallium/lua-epoll

local irc = require "ircclient"
local epoll = require "epoll"

local s1 = irc.create_session()
s1:register("connect", function(s, ...) print(s) s:join("#test") end)
s1:register("channel", print)
s1:connect{ host = "127.0.0.1", nick = "test1" }
local s2 = irc.create_session()
s2:register("connect", function(s, ...) print(s) s:join("#test") end)
s2:register("channel", print)
s2:connect{ host = "127.0.0.1", nick = "test2" }

local loop = epoll.new()

while true do
    local rfd1, rfd2, wfd1, wfd2 = {}, {}, {}, {}
    s1:add_descriptors(rfd1, wfd1)
    s2:add_descriptors(rfd2, wfd2)
    for i, v in ipairs(wfd1) do
        loop:add(v, epoll.EPOLLOUT, v)
    end
    for i, v in ipairs(rfd1) do
        loop:add(v, epoll.EPOLLIN, v)
    end
    for i, v in ipairs(wfd2) do
        loop:add(v, epoll.EPOLLOUT, v)
    end
    for i, v in ipairs(rfd2) do
        loop:add(v, epoll.EPOLLIN, v)
    end
    local out = {}
    loop:wait(out, -1)
    local fd, evt = unpack(out)
    local rfd, wfd = {}, {}
    if evt == epoll.EPOLLIN then
        rfd[1] = fd
    elseif evt == epoll.EPOLLOUT then
        wfd[1] = fd
    end
    s1:process_descriptors(rfd, wfd)
    s2:process_descriptors(rfd, wfd)
    loop:del(fd)
end

