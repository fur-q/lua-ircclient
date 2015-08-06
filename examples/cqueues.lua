local cqueues = require "cqueues"
local irc = require "ircclient"

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

local mainloop = function()
    while true do
        local rfd, wfd = s1:add_descriptors({}, {})
        s2:add_descriptors(rfd, wfd)
        local fds = {}
        for i, v in ipairs(rfd) do
            fds[#fds+1] = { pollfd = v, events = "r" }
        end
        for i, v in ipairs(wfd) do
            fds[#fds+1] = { pollfd = v, events = "w" }
        end
        local out = { cqueues.poll(unpack(fds)) }
        rfd, wfd = {}, {}
        for i, v in ipairs(out) do
            if v.events == "r" then
                rfd[#rfd+1] = v.pollfd
            else
                wfd[#wfd+1] = v.pollfd
            end
        end
        s1:process_descriptors(rfd, wfd)
        s2:process_descriptors(rfd, wfd)
    end
end

local cq = cqueues.new()
cq:wrap(mainloop)
assert(cq:loop())

