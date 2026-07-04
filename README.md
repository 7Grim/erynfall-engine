# Erynfall Engine

Old School RuneScape-style MMO built with Godot 4 C++ (client) and dedicated C++20 (server).

## Quick Start

### Server

```bash
# Prerequisites: cmake, g++, libsqlite3-dev
cd server && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)
./erynfall-server --port 43594 --db saves.db
```

The server boots on port 43594 (OSRS classic port) with a 600ms tick rate.

### Client

```bash
# Prerequisites: Godot 4.6+, scons, g++
git clone --recurse-submodules https://github.com/7Grim/erynfall-engine.git
cd erynfall-engine/client

# Build the GDExtension module
scons -j$(nproc)              # debug
scons target=release -j$(nproc)  # release

# Open in Godot Editor
godot client/project.godot
```

## Architecture

```
Client (Godot 4 C++) ←TCP→ Server (C++20)
         ↕                        ↕
    Godot Scene Tree         Game Loop (600ms tick)
    Input/Rendering          Entity/World/Skill Systems
    UI Panels                SQLite Persistence
```

| Component | Stack | Details |
|-----------|-------|---------|
| **Client** | Godot 4 + GDExtension C++20 | Rendering, input, UI, networking |
| **Server** | CMake + C++20 + SQLite3 | Tick loop, world state, entities, persistence |
| **Protocol** | Custom binary over TCP | 1-byte length prefix, 1-byte opcode, variable payload |
| **Shared** | Header-only C++ | Protocol buffers, opcodes, types, constants, XP tables |

## Project Structure

| Directory | Purpose |
|-----------|---------|
| `client/` | Godot 4 C++ client (rendering, input, UI, networking) |
| `client/src/` | C++ GDExtension source (TileMap, PlayerController) |
| `client/godot-cpp/` | [git submodule] Godot C++ bindings |
| `client/scenes/` | Godot scene files |
| `server/` | Dedicated C++ game server (tick loop, world state, entities) |
| `server/src/` | Core, network, world, entity, database, systems |
| `shared/` | Protocol definitions, constants, data structures (used by both) |
| `docs/` | Design, architecture, and phase roadmap |

## Development Phases

See [PHASES.md](docs/PHASES.md) for the full roadmap.

| Phase | Focus | Status |
|-------|-------|--------|
| 0 | Setup — builds, render cube, server boots | ✅ Complete |
| 1 | The Grid — tile map, camera, tick loop, click-to-move | 🔲 Not Started |
| 2 | The Loop — 1 skill, inventory, XP | 🔲 Not Started |
| 3 | The Fight — melee combat, 1 NPC, HP, death | 🔲 Not Started |
| 4 | The Gear — equipment, shops, food | 🔲 Not Started |
| 5 | The World — bank, larger map, multiple zones | 🔲 Not Started |
| 6 | The Grind — XP curve, multiple skills, progression | 🔲 Not Started |
| 7 | Multiplayer — client-server, state sync | 🔲 Not Started |

## Contributing

1. Check the [GitHub Project board](https://github.com/orgs/7Grim/projects) for current phase tasks
2. Each issue has acceptance criteria — meet them before opening a PR
3. Target `main` for PRs (single-branch workflow for now)
