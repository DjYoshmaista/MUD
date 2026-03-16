# CURRENT TODO

## Purpose
This file tracks the current work needed to stabilize the logging subsystem and define `src/mud_log_thread.c` in a way that fits the architecture of this MUD project.

This checklist is ordered by dependency. Earlier items unblock later items.

## Status Legend
- `[ ]` Not started
- `[~]` In progress
- `[x]` Completed
- `[!]` Blocked or needs design choice

## Completed Review Work
- `[x]` Read all project Markdown docs in `docs/`
- `[x]` Read build configuration in `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`, and `CMakePresets.json`
- `[x]` Reviewed current logging, sink, utility, and test files
- `[x]` Verified current build state with CMake

## Active TODO List

### 1. Break the logging header include cycle
- Status: `[~]`
- Why first: the original header cycle was partially addressed, but the logging boundary is still not clean and the public header still exposes an internal dispatch function using an unknown type.
- Main files:
  - `include/mud_log.h`
  - `include/mud_log_thread.h`
  - `include/mud_log_sink.h`
  - `src/mud_log.c`
- Hint:
  - Decide which header owns each type.
  - Keep high-level logging API in `mud_log.h`.
  - Keep sink record/sink type definitions in `mud_log_sink.h`.
  - Keep async-thread API in `mud_log_thread.h`.
  - Prefer forward declarations in headers and concrete includes in `.c` files when possible.
  - Current blockers:
    - `mud_log.h` still declares `mud_log_dispatch_record(const MudLogRecord*)` without a visible `MudLogRecord`.
    - `mud_log_thread.h` still contains internal `LogState`, which should not live in a public async-thread header.

### 2. Repair invalid logging declarations and public type boundaries
- Status: `[~]`
- Why second: even after the include cycle is fixed, several logging declarations are syntactically or structurally invalid and will still stop the build.
- Main files:
  - `include/mud_log.h`
  - `include/mud_log_thread.h`
  - `include/mud_log_sink.h`
- Hint:
  - Remove or redesign declarations that expose types before they exist.
  - Replace invalid opaque typedef syntax.
  - Fix malformed function declarations.
  - Decide whether internal-only structs belong in public headers at all.
  - Current blockers:
    - `mud_log_thread.h` contains malformed `struct MudLogQueue *;`
    - `mud_log_thread.h` still declares `mud_log_thread_enqueue*`
    - `mud_log_thread.h` still contains invalid `LogState` flexible-array layout
    - `mud_log_types.h` duplicates sink/record ownership instead of clarifying it

### 3. Fix `mud_log.c` locking and extract a real dispatch helper
- Status: `[ ]`
- Why third: the async thread should not reach into `g_log` directly. `mud_log.c` needs a safe dispatch function and must not double-lock the logger mutex.
- Main files:
  - `src/mud_log.c`
  - `include/mud_log.h`
  - `include/mud_log_sink.h`
- Hint:
  - Build one function whose job is "send this record to the registered sinks under the logger lock."
  - Reuse that helper from direct logging and from the async logging thread.
  - Make one clear policy for when the mutex is locked and unlocked.
  - Current blockers:
    - `mud_log_writev()` still double-locks `g_log.mutex`
    - `mud_log_dispatch_record()` is still a stub

### 4. Resolve logging type ownership cleanly
- Status: `[ ]`
- Why now: `include/mud_log_types.h` was added, but it currently duplicates `MudLogRecord` and `MudLogSink` instead of reducing coupling. The project must choose one canonical definition site for each type.
- Main files:
  - `include/mud_log_types.h`
  - `include/mud_log_sink.h`
  - `include/mud_log.h`
- Hint:
  - Either remove `mud_log_types.h` entirely and keep sink/record ownership in `mud_log_sink.h`, or move the types fully and update every dependent header consistently.
  - Do not keep duplicate definitions of the same public types.

### 5. Define the queue-entry contract for async logging
- Status: `[ ]`
- Main files:
  - `include/mud_log_thread.h`
  - `src/mud_log_thread.c`
- Hint:
  - Decide exactly which fields are copied by value into the queue entry.
  - Keep strings embedded in the queue entry to avoid dangling stack pointers.
  - Treat flush synchronization pointers as borrowed temporary state.

### 6. Implement the ring buffer internals
- Status: `[ ]`
- Main files:
  - `src/mud_log_thread.c`
- Hint:
  - Implement in this order: queue create, queue destroy, queue push, queue pop.
  - Protect queue state with one mutex.
  - Use condition variables for empty/full waiting.

### 7. Implement consumer-side record dispatch in `mud_log_thread.c`
- Status: `[ ]`
- Main files:
  - `src/mud_log_thread.c`
  - `src/mud_log.c`
- Hint:
  - Convert queue entries into temporary `MudLogRecord` values.
  - Dispatch to one sink or all sinks depending on `target_sink`.
  - Let `mud_log.c` own the sink list; let `mud_log_thread.c` own the thread/queue.

### 8. Implement flush and shutdown semantics
- Status: `[ ]`
- Main files:
  - `src/mud_log_thread.c`
  - `src/mud_log.c`
- Hint:
  - `FLUSH` should act like a barrier.
  - `SHUTDOWN` should be a sentinel that tells the consumer to stop after draining earlier work.
  - Define what should happen when flush is called while the thread is not running.

### 9. Finish the public async logger API
- Status: `[ ]`
- Main files:
  - `include/mud_log_thread.h`
  - `src/mud_log_thread.c`
  - `src/mud_log.c`
- Hint:
  - Finish `init`, `shutdown`, `enqueue`, `enqueue_to_sink`, `flush`, and `is_running`.
  - Make failure behavior explicit and consistent.

### 10. Restore missing logging/helper functions needed by the API
- Status: `[ ]`
- Main files:
  - `src/mud_log.c`
  - `src/mud_utils.c`
- Hint:
  - Either implement missing declared functions or remove declarations that are not part of the current design.
  - Keep the public header surface honest.

### 11. Wire the thread logger into the build
- Status: `[ ]`
  - Main files:
  - `src/CMakeLists.txt`
  - top-level `CMakeLists.txt`
- Hint:
  - Add `mud_log_thread.c` to `mud_core` only after the headers compile cleanly.
  - Add a proper thread dependency in CMake instead of relying on accidental linker behavior.
  - Current blocker:
    - `src/CMakeLists.txt` still does not include `mud_log_thread.c`

### 12. Add logging/thread tests
- Status: `[ ]`
- Main files:
  - `tests/CMakeLists.txt`
  - `tests/modules/` (new file)
  - `tests/run_all_tests.c`
  - `tests/run_single_module.c`
- Hint:
  - Start with lifecycle tests, queueing tests, flush tests, and fallback behavior tests.
  - Make sure test runners initialize logging consistently.

## Suggested Working Order
1. Complete items 1-4 before editing most of `src/mud_log_thread.c`
2. Complete items 5-9 to get the async logger logically correct
3. Complete items 10-12 to make the module buildable and testable

## Learning Notes
- In C, headers should expose only what other translation units need to know.
- If two headers include each other, that often means type ownership is unclear.
- Prefer making `.c` files depend on headers, not making headers depend on everything.
- In threaded code, define ownership and synchronization rules before writing the function bodies.
