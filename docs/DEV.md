# DEV.md

## Development Philosophy
- Learn by building core systems manually
- Strict CI, sanitizers, and tests from the beginning
- Phase-based progression to maintain stability

## Phase 0 (Completed)
Defined:
- build system (CMake + Ninja, Clang primary)
- strict warnings and sanitizer modes
- logging, config, error handling policies
- threading and shard architecture

## Phase 1 — Foundations
Goal: Build reusable primitives and infrastructure

Modules:
1. Minimal test runner
2. StringView
3. Buffer + String
4. Vector (macro-based typed)
5. Arena allocator (per thread, mark/rewind)
6. Hash map (separate chaining with entry pool)
7. Priority queue timers
8. Async logger thread

Testing strategy:
- one test binary per module
- one meta-runner executes all
- sanitizers used in debug modes

## Phase 2 — Networking & Concurrency Shell
- listener/acceptor
- connection threads
- input/output buffering
- event bus skeleton
- per-shard queues

## Phase 3 — Session & Protocol
- connection state machine
- login/auth system
- message routing and UI layer

## Phase 4 — Core World Model
- entity definitions
- registries and indexing
- movement rules
- inventory and containment
- command system

## Phase 5 — Simulation
- world tick integration
- NPC AI
- combat system

## Phase 6 — Persistence & Content
- world loading
- player save/load
- content authoring tools

## Phase 7 — Administration & Security
- admin commands
- anti-abuse mechanisms
- testing harness expansion

## Immediate Next Step
Start Phase 1 with:
1) minimal test runner
2) StringView module
3) Buffer/String module
