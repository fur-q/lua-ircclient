local irc = require "ircclient"

local session = irc.create_session()
session:option_set(irc.options.STRIPNICKS)
session:register("connect", function(s) 
    s:join("#test") 
end)
session:register("channel", function(s, nick, chan, text)
    s:msg(chan, text)
end)
session:connect{ host = "127.0.0.1", nick = "test" }
session:run()
