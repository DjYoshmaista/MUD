# MUD Master TODO (Module-by-Module)

This document is a phased TODO list for building a MUD in C using a **thread-per-connection** approach while keeping the **server/world logic event-driven**.

---

## Phase 0 — Ground Rules (Project Discipline)

1. **Build profiles & tooling**
   - Define debug vs release builds
   - Decide sanitizer usage (ASan/UBSan/TSan) and when
   - Decide warning policy and CI checks

2. **Portability scope**
   - Decide Linux-only vs cross-platform target
   - Pick socket/event API strategy accordingly

3. **Coding conventions**
   - Naming conventions (files, functions, types)
   - Ownership/lifetime rules
   - Error handling conventions (return codes, errno, logging patterns)

---

## Phase 1 — Foundations (Non-Gameplay Core)

4. **Core types & utilities module**
   - Dynamic strings/buffers
   - Collections (vector/list/map)
   - Time utilities
   - Safe parsing helpers

5. **Logging & diagnostics module**
   - Structured logging (timestamps, connection id, thread id)
   - Crash strategy (assertions, fail-fast vs soft errors)

6. **Configuration module**
   - Load config from file/env
   - Runtime toggles (port, max clients, MOTD path)

---

## Phase 2 — Networking & Concurrency (Server Shell)

7. **Network acceptor (listener) module**
   - Accept loop
   - Connection admission policy (max connections, IP bans, rate limiting)

8. **Connection I/O module**
   - Input buffering and line framing
   - Output queueing and flushing
   - Telnet compatibility decisions

9. **Threading model module**
   - Thread lifecycle per connection
   - Synchronization strategy (what is shared and what is not)
   - Shutdown coordination

10. **Event system module (server-side event-driven mechanism)**
   - Event queue design (per-thread vs global)
   - Event types (input line, disconnect, timer tick, world action request)
   - Timer scheduling

---

## Phase 3 — Session & Protocol (From Socket to Player)

11. **Session module**
   - Connection state machine (connected → negotiating → login → in-game)
   - Per-session rate limiting / flood control

12. **Authentication & account module**
   - Guest vs account login
   - Password storage strategy
   - Lockouts and audit logging

13. **Text/UI module**
   - Prompts, paging (“more”), line wrapping
   - Color/ANSI policy
   - Message routing (tell/room/broadcast)

---

## Phase 4 — Core Data Model (World & Entities)

14. **Entity model module**
   - Define “what is an entity”
   - Identity scheme (IDs)
   - Persistence hooks

15. **World state module**
   - Room registry, zone/area structure
   - Occupancy tracking
   - Indexing/search (by id, by name)

16. **Movement & spatial rules module**
   - Exits/links
   - Validation rules (locked doors, permissions)

17. **Inventory & object containment module**
   - Character inventory, containers, room contents
   - Stacking rules

18. **Stats & progression module**
   - Attributes, skills, XP/levels

19. **Command system module**
   - Parser
   - Command registry
   - Permissions

---

## Phase 5 — Simulation & Gameplay Loops

20. **World tick / scheduler integration module**
   - Regen
   - Periodic effects
   - Decay/cleanup

21. **NPC & AI module**
   - State machine per NPC
   - Simple behaviors

22. **Combat module**
   - Target selection
   - Rounds, cooldowns
   - Damage model

---

## Phase 6 — Persistence & Content

23. **World data loading module**
   - Rooms/items/npcs from files/db
   - Validation pass

24. **Player save/load module**
   - Atomic writes
   - Versioning/migrations

25. **Content authoring support**
   - Builder commands or offline tooling
   - Linting/validation tools

---

## Phase 7 — Administration, Security, Testing

26. **Admin tools module**
   - Shutdown, kick, reload, set flags

27. **Security hardening module**
   - Input sanitization, limits
   - Anti-flood/anti-spam

28. **Test harness module**
   - Offline unit tests for parsing, containers, persistence
   - Integration tests (fake clients)

---
