# lua-ircclient documentation

### The ircclient module

##### get_version()

Returns the libircclient version major and minor version numbers.

##### strerror(err)

Returns a string description of the [error](#errors) *err*.

##### get_nick(mask)

Returns the nick portion of the provided IRC mask.

##### get_host(mask)

Returns the host portion of the provided IRC mask.

##### color_strip(msg)

Strips all mIRC formatting from *msg* and returns the stripped message.

##### color_convert_to_mirc()

Converts libircclient's internal formatting codes in *msg* to mIRC formatting and returns the
converted message.

##### color_convert_from_mirc(msg)

Converts mIRC formatting codes in *msg* to libircclient's internal formatting and returns the
converted message.

##### create_session()

Creates and returns a new IRC session.

### Session functions

Unless otherwise specified, all session functions return *true* on success or *nil,
[error](#errors)* on failure.

If a function returns *true*, it means the command was sent to the server. An [event
handler](#sessionregisterevt-callback) must be registered for the appropriate events to find out if
the command was successful.

For example, when joining a channel, a [JOIN](#join) event will be triggered on success, a *474*
event will be triggered if you are banned from the channel, a *475* event will be triggered if you
have provided the wrong key, and so forth.

##### session:register(evt, callback)

Registers the function *callback* for the event *evt*. 

See [events](#events) for a list of events and callback function parameters.

##### session:connect(args)

Connects the session to the given server. *args* is a table with the following fields:

- **host**: the server address
- **port**: the server port (optional, default: *6667*)
- **nick**: the session's nickname
- **user**: the session's ident (optional, default: same as nick)
- **realname**: the session's real name, shown in whois (optional, default: same as nick)
- **password**: the server's password (optional)
- **ipv6**: if *true*, connect via IPv6 (optional, default: *false*)

##### session:disconnect()

Disconnects from the active server.

##### session:is_connected()

Returns *true* if the session is connected to a server, and *false* if not.

##### session:join(chan, key)

Joins the channel *chan* with the optional key *key*.

##### session:part(chan)

Leaves the channel *chan*.

##### session:invite(nick, chan)

Invites the user *nick* to the channel *chan*.

##### session:names(chan)

Requests a list of nicknames on the channel *chan*.

##### session:list(chan)

Requests a list of channels. *chan* may optionally contain a comma-separated list of channel names
to search for.

##### session:topic(chan, topic)

If *topic* is set, sets the topic on *chan*. Otherwise, requests the topic on *chan*.

##### session:channel_mode(chan, mode)

If *mode* is set, sets the modes on *chan*. Otherwise, requests the current channel modes on *chan*.

##### session:user_mode(mode)

If *mode* is set, sets the modes on yourself. Otherwise, requests your current user modes.

##### session:kick(nick, chan, reason)

Kicks *nick* from *chan* with optional *reason*.

##### session:msg(target, msg)

Sends the message *msg* to the user or channel *target*.

##### session:me(target, msg)

Sends a CTCP ACTION message *msg* to the user or channel *target*.

##### session:notice(target, msg)

Sends the notice *msg* to the user or channel *target*.

##### session:ctcp_request(nick, req)

Sends the CTCP request *req* to *nick*.

##### session:ctcp_reply(nick, rep)

Sends the CTCP reply *rep* to *nick*.

##### session:nick(newnick)

Changes your nick to *newnick*.

##### session:whois(nick)

Requests whois information on *nick*, which may be a nick or a comma-separated list of nicks.

##### session:quit(reason)

Disconnects from the IRC server, with the optional reason *reason*.

##### session:send_raw(format, ...)

Send raw data to the IRC server, e.g. a command not supported by the library. The arguments are the
same as for *string.format*.

##### session:dcc_accept(dccid, callback)

Accepts the DCC CHAT or DCC SEND request *dccid*.

*callback* is a function which will be triggered upon receipt of DCC events. It takes the following
parameters:

- **status**: *true* if successful, otherwise *[error](#errors)*
- **length**: the length of *data* 
- **data**: for DCC CHAT, a DCC CHAT message; for DCC SEND, a portion of the received file

##### session:dcc_decline(dccid)

Declines the DCC CHAT or DCC SEND request *dccid*.

##### session:dcc_chat(nick, callback)

Sends a DCC CHAT request to *nick*. Returns the DCC session ID on success, or *nil,
[error](#errors)* on failure.

*callback* is a function which will be triggered upon receipt of DCC CHAT messages. It takes the
following parameters:

- **status**: *true* if successful, otherwise *[error](#errors)*
- **length**: the length of the message
- **data**: the message

##### session:dcc_msg(dccid, msg)

Sends the message *msg* to the DCC CHAT session *dccid*.

##### session:dcc_sendfile(nick, filename, callback)

Sends a DCC SEND request to *nick* for the file *filename*. Returns the DCC session ID on success,
or *nil, [error](#errors)* on failure. 

*callback* is a function which will be triggered after a successfully sent packet or an error. It
takes the following parameters:

- **status**: *true* if successful, otherwise *[error](#errors)*
- **length**: the length of the packet sent

##### session:option_set(opt)

Sets the [option](#options) *opt*.

##### session:option_reset(opt)

Resets the [option](#options) *opt* to its default value.

##### session:run()

Enters the main loop. This function will not return until the connection is terminated, either
remotely or by calling [session.quit](#sessionquitreason). To use a different main loop, see
[session.add_descriptors](#sessionadd_descriptorsrfd-wfd).

##### session:add_descriptors(rfd, wfd)

Adds the session's readable file descriptors to the table *rfd* and writable file descriptors to the
table *wfd*, and returns both tables. 

##### session:process_descriptors(rfd, wfd)

Processes the session's events on the readable file descriptors in the table *rfd* and writable file
descriptors in the table *wfd*, and returns both tables. 

See examples/epoll.lua or examples/cqueues.lua for sample usage.

### Events

Event handlers are registered with [session.register](#sessionregisterevt-callback) and called when
the corresponding event is received from the IRC server. Only one handler may be registered for each
event.

Unless otherwise specified, the first two parameters to the callback function are always the
following: 

- session: the session which triggered the callback
- origin: the IRC mask (or nick, if [options.STRIPNICKS](#optionsstripnicks) is set) of the user who
  triggered the event

Any additional parameters are specified for each event.

##### ERROR

Triggered when an error is thrown by a callback function. By default, this is set to *print*.

If there is an error in the error handler, a fatal (unhandled) error will be thrown.

Parameters:

- the error

##### CONNECT

Triggered when the session connects to the IRC server.

##### NICK

Triggered when a user in one of the channels you have joined (including yourself) changes their
nick.

Additional parameters:

- the new nickname

##### QUIT

Triggered when a user in one of the channels you have joined quits IRC.

Additional parameters:

- quit message

##### JOIN

Triggered when a user joins a channel you are in, or when you join a new channel.

Additional parameters:

- channel name

##### PART

Triggered when a user leaves a channel you are in, or when you leave a channel.

Additional parameters:

- channel name
- reason (optional)

##### MODE

Triggered when someone changes the modes of a channel you are in.

Additional parameters:

- channel name
- modes changed
- nick (optional)

##### UMODE

Triggered when your user modes are changed.

Additional parameters:

- nick
- modes changed

##### TOPIC

Triggered when a channel's topic is changed.

Additional parameters:

- channel name
- new topic (optional)

##### KICK

Triggered when a user is kicked from a channel.

Additional parameters:

- channel name
- nick of user who was kicked
- kick message (optional)

##### CHANNEL

Triggered when a message is sent to a channel you are in.

Additional parameters:

- channel name
- message (optional)

##### PRIVMSG

Triggered when a message is sent privately to you.

Additional parameters:

- your nick
- message (optional)

##### NOTICE

Triggered when a NOTICE is sent privately to you.

Additional parameters:

- your nick
- message (optional)

##### CHANNEL_NOTICE

Triggered when a NOTICE is sent to a channel you are in.

Additional parameters:

- channel name
- message (optional)

##### INVITE

Triggered upon receipt of an invite message.

Additional parameters:

- your nick
- channel name

##### CTCP_REQ

Triggered upon receipt of a CTCP request.

Additional parameters:

- message

##### CTCP_REP

Triggered upon receipt of a CTCP reply.

Additional parameters:

- message

##### CTCP_ACTION

Triggered when a CTCP ACTION (/me) message is sent to a channel you are in.

Additional parameters:

- message

##### UNKNOWN

Triggered upon receipt of an unknown non-numeric message. Parameters depend on the message.

##### (numeric)

Triggered upon receipt of any numeric message from the server (see RFC 1429 for an incomplete list).
Parameters depend on the message.

##### SEND

Triggered upon receipt of a DCC SEND request.

Parameters:

- user's nick
- user's IP address
- filename
- filesize
- DCC id (see [session.dcc_accept](#sessiondcc_acceptid-callback) and
  [session.dcc_decline](#sessiondcc_declineid))

##### CHAT

Triggered upon receipt of a DCC CHAT request.

Parameters:

- user's nick
- user's IP address
- DCC id (see [session.dcc_accept](#sessiondcc_acceptid-callback) and
  [session.dcc_decline](#sessiondcc_declineid))

### Options

Options are provided in the *ircclient.options* table.

##### options.DEBUG

If set, and if libircclient is compiled with debug symbols, send additional debug information to
STDOUT.

##### options.STRIPNICKS

If set, automatically strip the host part of IRC masks passed to callbacks.

##### options.SSL_NO_VERIFY

If set, do not verify server SSL certificates (e.g. for self-signed certificates).

### Errors

Errors are provided in the *ircclient.errors* table.

##### errors.INVAL

An invalid argument was provided to a function.

This error should never occur when using the bindings. If it does, report it as a bug.

##### errors.RESOLV

The hostname provided to [session.connect](#sessionconnectargs) could not be resolved into a valid
IP address.

##### errors.SOCKET

A new socket could not be created or made nonblocking.

##### errors.CONNECT

A socket could not connect to an IRC server or DCC client.

##### errors.CLOSED

A connection was closed by an IRC server or DCC client.

##### errors.NOMEM

Either no memory could be allocated (which is usually a fatal error) or the command buffer is full.

##### errors.ACCEPT

An incoming DCC CHAT or DCC SEND connection could not be accepted.

##### errors.NODCCSEND

A filename supplied to [session.dcc_sendfile](#sessiondcc_acceptid-callback) could not be sent.

##### errors.READ

Either a DCC file could not be read or a DCC socket returned a read error.

##### errors.WRITE

Either a DCC file could not be written or a DCC socket returned a write error.

##### errors.STATE

A function was called at the wrong time, e.g. [session.join](#sessionjoinchan-key) was called on a
disconnected session.

##### errors.TIMEOUT

A DCC request timed out.

##### errors.OPENFILE

The file specified in [session.dcc_sendfile](#sessiondcc_acceptid-callback) could not be opened.

##### errors.TERMINATED

The IRC server connection was terminated.

##### errors.NOIPV6

A function which requires IPv6 support was called, but libircclient was not compiled with IPv6
support.

##### errors.SSL_NOT_SUPPORTED

A function which requires SSL support was called, but libircclient was not compiled with SSL
support.

##### errors.SSL_INIT_FAILED

The SSL library could not be initialised.

##### errors.CONNECT_SSL_FAILED

An SSL handshake failed.

##### errors.SSL_CERT_VERIFY_FAILED

The server is using an invalid or self-signed certificate and
[options.SSL_NO_VERIFY](#optionsssl_no_verify) was not set.

