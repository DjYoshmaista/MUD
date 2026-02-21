# ARCHITECTURE.md

## Overview
This project is a Multi-User Dungeon (MUD) server written in C for Arch Linux, using a thread-per-connection model combined with an event-driven, sharded world design.

## Core Principles
- Correctness-first, strict compilation
- Event-driven gameplay logic
- One worker thread per shard (zone/area)
- No cross-thread pointers; IDs used for cross-thread references
- Hybrid memory model: malloc/free for persistent entities, arenas for transient data
- Structured logging and observability from day one

## Concurrency Model
- One connection thread per client
- One worker thread per shard (zone)
- Hybrid event bus:
  - global control queue (connect/disconnect/system)
  - per-shard queues for gameplay

## Data Model
- Entities identified by compact integer IDs
- Pointers used only within shard ownership boundary
- Registries:
  - dense vectors for ID-indexed access
  - hash maps for flexible lookups

## Memory Management
- Persistent entities: malloc/free with strict ownership
- Transient allocations: arena allocators (per thread)
- Arena supports mark/rewind for nested temporary allocations

## Error Handling
Two-tier API:
- Low-level: explicit return codes
- High-level: Result structs (code + context)

## Logging
- Structured logs with:
  - timestamp, severity, module, thread ID, connection ID, entity ID, EventID
- Outputs:
  - human-readable per-module logs
  - aggregated JSON log (debug default on, release default off)

## Utilities
Custom minimal implementations:
- Buffer + String
- Vector (macro-based typed)
- Hash map (separate chaining via bucket -> entry pool index)
- StringView
- Arena allocator
- Priority queue timers
- Async logging system

## Scheduling
- Timers based on CLOCK_MONOTONIC
- Absolute deadlines via priority queue
