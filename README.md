# lua-ircclient

Lua bindings to [libircclient](http://www.ulduzsoft.com/linux/libircclient/).

## Installation

lua-ircclient targets Lua 5.1, LuaJIT and Lua 5.2, and libircclient 1.7 and 1.8.

On Debian, Ubuntu and other dpkg-based distros, install `dpkg-dev` and `dh-lua`, run
`dpkg-buildpackage -us -uc` to build a Debian package, then run `dpkg -i lua-ircclient_VERSION.deb`
to install it.

On any other *nix, use the provided Makefile. There are three variables which may need to be
overriden:

- `LUAVER` - the target Lua version. Defaults to `5.2`.
- `LUAPC` - the name of your system's pkg-config or pkgconf file for Lua. Defaults to
  `lua$(LUAVER)`.
- `PKGCONF` - the name of your system's pkg-config or pkgconf binary. Defaults to `pkg-config`.

Building on MinGW should work but is untested.

## Usage

See the [documentation](doc/ircclient.md) and [examples](examples).

The [libircclient documentation](http://www.ulduzsoft.com/libircclient/) may also be helpful.

## Licence

See [LICENCE](LICENCE).
