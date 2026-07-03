# Erynfall Engine

Old School RuneScape-style MMO built with Godot 4 C++ (client) and dedicated C++ (server).

## Quick Start

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/7Grim/erynfall-engine.git
cd erynfall-engine

# Server
cd server && mkdir build && cd build
cmake .. && make -j$(nproc)
./erynfall-server --port 43594 --data ../data/

# Client (requires Godot 4)
# Open client/project.godot in Godot Editor
```

## Architecture

```
Client (Godot 4 C++) ←TCP→ Server (C++20)
         ↕                        ↕
    Godot Scene Tree         Game Loop (600ms tick)
    Input/Rendering          Entity/World/Skill Systems
    UI Panels                SQLite Persistence
```

- **Client**: Godot 4 with GDExtension C++ modules
- **Server**: Pure CMake/C++20, no engine dependency
- **Protocol**: TCP, 600ms tick rate, delta-based regional updates
- **Persistence**: SQLite (dev), PostgreSQL (prod)
- **Shared**: Header-only protocol and data definitions

## Project Structure

| Directory | Purpose |
|-----------|---------|
| `client/` | Godot 4 C++ client (rendering, input, UI, networking) |
| `server/` | Dedicated C++ game server (tick loop, world state, entities) |
| `shared/` | Protocol definitions, constants, data structures (used by both) |
| `data/` | Game data — maps, item defs, NPC defs, XP tables |
| `docs/` | Design, architecture, and phase docs |
| `tools/` | Build scripts, world editor, utilities |

## Development Phases

See [PHASES.md](docs/PHASES.md) for the full roadmap.

| Phase | Focus | Status |
|-------|-------|--------|
| 0 | Setup — repo, build pipeline, render cube | 🔲 Not Started |
| 1 | The Grid — tile map, camera, tick loop, click-to-move | 🔲 Not Started |
| 2 | The Loop — 1 skill, inventory, XP | 🔲 Not Started |
| 3 | The Fight — melee combat, 1 NPC, HP, death | 🔲 Not Started |
| 4 | The Gear — equipment, shops, food | 🔲 Not Started |
| 5 | The World — bank, larger map, multiple zones | 🔲 Not Started |
| 6 | The Grind — XP curve, multiple skills, progression | 🔲 Not Started |
| 7 | Multiplayer — client-server, state sync | 🔲 Not Started |

## Contributing

1. Check the [GitHub Project board](https://github.com/orgs/7Grim/projects) for current phase tasks
2. Create a `feature/*` branch from `dev`
3. Each issue has acceptance criteria — meet them before opening a PR
4. Target `dev` for PRs, not `main` (main = last completed phase, always playable)
