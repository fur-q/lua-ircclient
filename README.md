# lua-ircclient

Lua bindings to [libircclient](http://www.ulduzsoft.com/linux/libircclient/).

## Installation

lua-ircclient targets Lua 5.1, LuaJIT and Lua 5.2, and libircclient 1.7 and 1.8.

The makefile uses Lua 5.2 by default. To use a different version, run `LUA=lua5.1 make`, where `LUA`
is the name of the corresponding pkg-config file.

## Usage

Full documentation is provided in [doc/ircclient.md](blob/master/doc/ircclient.md). The
[libircclient docs](http://www.ulduzsoft.com/libircclient/) may also be helpful.

## Licence

See [LICENCE](blob/master/LICENCE).
