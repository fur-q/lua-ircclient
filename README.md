# lua-ircclient

Lua bindings to [libircclient](http://www.ulduzsoft.com/linux/libircclient/).

## Installation

lua-ircclient targets Lua 5.1, LuaJIT and Lua 5.2, and libircclient 1.7 and 1.8.

On Debian, Ubuntu and other dpkg-based distros, run `dpkg-buildpackage -us -uc` to build a Debian
package, then `dpkg -i lua-ircclient_VERSION.deb` to install it.

On anything else, use the provided Makefile. There are three variables you may need to change:

- `LUAVER` - your target Lua version. Should be `5.1` or `5.2`.
- `LUAPC` - the name of your system's pkg-config or pkgconf file for Lua. Defaults to
  `lua$(LUAVER)`.
- `PKGCONF` - the name of your system's pkg-config or pkgconf binary.

The library should build with MinGW, but it is untested.

## Usage

Full documentation is provided in [doc/ircclient.md](doc/ircclient.md). The
[libircclient docs](http://www.ulduzsoft.com/libircclient/) may also be helpful.

## Licence

See [LICENCE](LICENCE).
