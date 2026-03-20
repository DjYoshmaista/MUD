# Session Handoff - 2026-03-19

## Scope Reached

This session moved from test/DB infrastructure work into planning and defining the networking layer.

The user asked for:
- guidance on the networking layer design
- definitions for core networking files and elements
- high-level descriptions of `mud_socket.c` functions
- concrete example definitions for the more complex socket helpers

## Networking Architecture Decisions

Use these files and responsibilities:

- `include/mud_socket.h` / `src/net/mud_socket.c`
  Low-level POSIX socket setup and metadata helpers.
- `include/mud_connection.h` / `src/net/mud_connection.c`
  One accepted client connection, its buffers, timestamps, and state.
- `include/mud_net.h` / `src/net/mud_net.c`
  Listener aggregation, poll loop, accept/read/write dispatch, timeout reap.
- `include/mud_session.h` / `src/net/mud_session.c`
  Authenticated session lifecycle attached to a connection/account.
- `include/mud_net_security.h` / `src/net/mud_net_security.c`
  Loopback checks, auth backoff, size limits, constant-time session validation.

Use `poll()` as the first event backend. It is a better fit than `select()` for the current configured scale (`network.max_connections = 256`) and avoids `FD_SETSIZE` issues.

Security position:
- gameplay telnet traffic remains plaintext unless a separate TLS layer is added later
- admin HTTP stays loopback-bound by default
- `mud_crypto` is used for password hashing and session-token generation
- no custom transport encryption should be implemented

## Current Networking Files Present

Existing stub files:
- `include/mud_socket.h`
- `include/mud_connection.h`
- `include/mud_net.h`
- `include/mud_session.h`
- `include/mud_net_security.h`
- `src/net/mud_socket.c`
- `src/net/mud_net.c`

## Known Stub Problems To Fix First

These structural/compiler issues were identified before deeper implementation:

- `include/mud_net.h` and `include/mud_connection.h` both define `MudConnection`
  Only one file should own that type. Prefer `mud_connection.h`.
- `include/mud_session.h` embeds `MudNet net;`
  This is the wrong ownership direction. A session should attach to a connection/account, not contain the whole networking system.
- `include/mud_socket.h` has `MUD_SOCKET_HOLE_ADMIN_HTTP`
  Rename to `MUD_SOCKET_ROLE_ADMIN_HTTP`.
- `include/mud_socket.h` and `include/mud_net.h` use `typdef`
  Must be `typedef`.
- `include/mud_net.h` enum members use semicolons instead of commas.
- `include/mud_session.h` uses `__cpluplus`
  Must be `__cplusplus`.
- `src/net/mud_socket.c` and `src/net/mud_net.c` use nonexistent `mud_log_info(...)`
  Replace with existing logging macros from `include/mud_log.h`, e.g. `LOG_NET_INFO`, `LOG_NET_WARN`, `LOG_NET_ERROR`.
- `src/net/mud_net.c` refers to undefined or inconsistent symbols like `mud_time_now_ms()` and `mud_net_accept_read()`
  Add the time helper or rename consistently.

## High-Level Definitions Already Given

General definitions were provided for these `mud_socket.c` functions:
- `mud_socket_keepalive`
- `mud_socket_set_cloexec`
- `mud_socket_local_name`
- `mud_socket_close`
- `mud_socket_set_nonblocking`
- `mud_socket_set_nodelay`
- `mud_socket_peer_name`

The guidance was:
- `open_listener` handles address resolution, socket creation, bind/listen, option setup, and failure cleanup
- `set_*` helpers each own one socket flag/option
- `*_name` helpers convert kernel socket endpoint info into project-usable metadata
- `close` is the one standard shutdown path for socket fds

## Concrete Example Definitions Already Given

Two concrete example implementations were provided to the user in-chat:

### 1. `mud_socket_open_listener(...)`

Purpose:
- validate inputs
- normalize bind host
- use `getaddrinfo`
- iterate candidate addresses
- create socket
- set `FD_CLOEXEC`
- set `SO_REUSEADDR`
- set nonblocking
- `bind`
- `listen`
- store resulting fd in `MudSocketDef`

Important details:
- use `AF_UNSPEC`
- use `SOCK_STREAM`
- use `AI_PASSIVE`
- centralize cleanup through `mud_socket_close(&fd)`

### 2. `mud_socket_peer_name(...)`

Purpose:
- validate inputs
- call `getpeername`
- call `getnameinfo` with `NI_NUMERICHOST | NI_NUMERICSERV`
- copy host into output buffer
- parse service into `uint16_t`

Important details:
- use `sockaddr_storage`
- avoid reverse DNS
- initialize outputs before failure paths
- keep IPv4/IPv6 support generic

## Recommended Implementation Order

1. Finish `mud_socket.c`
   Implement:
   - `mud_socket_set_nonblocking`
   - `mud_socket_set_cloexec`
   - `mud_socket_set_nodelay`
   - `mud_socket_keepalive`
   - `mud_socket_close`
   - `mud_socket_peer_name`
   - `mud_socket_local_name`

2. Create/fix `mud_connection.c`
   Implement:
   - `mud_connection_reset`
   - `mud_connection_init`
   - `mud_connection_destroy`
   - `mud_connection_queue_bytes`
   - `mud_connection_queue_str`
   - `mud_connection_queue_flush`

3. Finish `mud_net.c`
   Implement:
   - `mud_net_init_from_config`
   - `mud_net_load_listeners`
   - `mud_net_alloc_conn_slot`
   - `mud_net_poll_once`
   - `mud_net_handle_readable`
   - `mud_net_handle_writable`
   - `mud_net_close_conn`
   - `mud_net_reap_timeouts`

4. Implement `mud_net_security.c`
   Implement:
   - `mud_net_addr_is_loopback`
   - `mud_net_may_attempt_auth`
   - `mud_net_validate_line_size`
   - `mud_net_validate_admin_origin`
   - `mud_net_constant_time_session_check`

5. Implement `mud_session.c`
   Implement:
   - `mud_session_begin`
   - `mud_session_is_valid`
   - `mud_session_end`
   - `mud_session_attach_account`

## C Networking Advice Already Given

These points were emphasized:

- use nonblocking sockets everywhere
- `accept`, `recv`, and `send` may all return partial results
- `recv == 0` means peer disconnected cleanly
- `EAGAIN` / `EWOULDBLOCK` are normal in nonblocking code
- socket input is raw bytes, not guaranteed C strings
- keep one owner per resource
- avoid custom encryption; use `mud_crypto` only for passwords/tokens/session checks
- enforce hard caps for input line size and output queue growth

## Existing Config / Logging / Crypto Surfaces Relevant To Networking

Relevant files already in the project:
- `config/mud.conf`
- `include/mud_log.h`
- `include/mud_crypto.h`
- `src/utils/mud_crypto.c`

Relevant config keys already present:
- `network.telnet.enabled`
- `network.telnet.port`
- `network.admin_http.enabled`
- `network.admin_http.port`
- `network.admin_http.bind`
- `network.max_connections`
- `network.connection_timeout_seconds`
- `auth.max_failed_logins`
- `auth.lockout_seconds`
- `auth.session_ttl_seconds`

## Non-Network Work Completed Earlier In The Same Conversation

This may matter for future sessions:

- DB statement helpers were extracted into the public `mud_db` API:
  - `mud_db_prepare`
  - `mud_db_bind_text`
  - `mud_db_bind_int64`
  - `mud_db_step`
  - `mud_db_column_*`
  - `mud_db_changes`
  - `mud_db_finalize`
- `test/modules/test_db.c` was updated to use those helpers instead of direct SQLite calls
- fixture generation for `data/db_dest/test.db` was automated using:
  - `test/generate_test_db.c`
  - CTest fixture setup in `test/CMakeLists.txt`
- Full test suite status at that point:
  `ctest --test-dir build/debug --output-on-failure` passed with `12/12` tests

## Good Next Prompt For A New Session

Use this prompt in a fresh instance:

`Read docs/SESSION_HANDOFF_2026-03-19.md, inspect the networking stubs under include/ and src/net/, fix the type/include issues first, then implement mud_socket.c cleanly and compile the project.`
