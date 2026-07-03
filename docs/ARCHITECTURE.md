# Erynfall — Architecture Document

> How systems connect. Updated per phase.

## System Overview

```
┌──────────────────────────────────────────────────────────┐
│                        CLIENT                             │
│                   Godot 4 + C++ GDExtension               │
│                                                           │
│  ┌─────────┐  ┌─────────┐  ┌──────┐  ┌───────────────┐  │
│  │  Input   │  │  Scene  │  │  UI  │  │   Networking   │  │
│  │ Handler  │→│  Tree   │  │Panels│  │  (TCP Client)  │  │
│  └─────────┘  └─────────┘  └──────┘  └───────┬───────┘  │
│       │             │          │              │           │
│       ▼             ▼          ▼              │           │
│  ┌──────────────────────────────────┐          │           │
│  │         Game State (Client)     │          │           │
│  │  - Local player position         │          │           │
│  │  - Viewport entities             │          │           │
│  │  - Inventory (local)             │          │           │
│  │  - Skill levels (local)          │          │           │
│  └──────────────────────────────────┘          │           │
└─────────────────────────────────────────────────┼─────────┘
                                                  │ TCP
                                                  │ Port 43594
                                                  ▼
┌──────────────────────────────────────────────────────────┐
│                        SERVER                             │
│                    Custom C++20 + CMake                     │
│                                                           │
│  ┌──────────────────────────────────────────────────────┐│
│  │                   600ms GAME LOOP                     ││
│  │                                                        ││
│  │   Process queued actions → Update entities →           ││
│  │   Resolve combat → Process skills → Send updates       ││
│  └──────────────────────────────────────────────────────┘│
│       │              │              │              │       │
│       ▼              ▼              ▼              ▼       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐   │
│  │ Network  │ │  Entity  │ │   Map &  │ │Persistence │   │
│  │ Manager  │ │ Manager  │ │  World   │ │   (DB)     │   │
│  │(TCP accept│ │(Players, │ │(Tiles,   │ │(SQLite/    │   │
│  │ packets,  │ │ NPCs,    │ │ Regions, │ │ PostgreSQL)│   │
│  │ regions)  │ │ Items)   │ │ Objects) │ │            │   │
│  └──────────┘ └──────────┘ └──────────┘ └────────────┘   │
└──────────────────────────────────────────────────────────┘
```

## Client Architecture (Godot 4)

### Scene Tree

```
Root (Window)
├── ViewportContainer
│   └── GameViewport
│       └── World3D
│           ├── Environment (skybox, lighting)
│           ├── Terrain (tile mesh instances)
│           ├── Entities (player, NPCs — as 3D nodes)
│           └── GroundItems (dropped items)
├── UI
│   ├── InventoryPanel
│   ├── EquipmentPanel
│   ├── SkillsPanel
│   ├── MinimapPanel
│   ├── ChatPanel
│   └── ActionBar (skill shortcuts)
└── NetworkClient (TCP connection)
```

### Client Tick System

The client does NOT run a 600ms tick. It runs at Godot's frame rate (60fps) for smooth rendering. The tick discipline is:

1. **Player clicks tile** → send action packet to server immediately
2. **Server processes on next tick** → sends position update back
3. **Client interpolates** movement between tile positions for smooth visual

This is exactly how OSRS works — client renders at higher framerate, but game state changes on tick boundaries.

### C++ Modules

| Module | Purpose |
|--------|---------|
| `core/` | Client-side game loop, state management, timing |
| `world/` | Tile rendering, camera control, entity positioning |
| `entities/` | Player and NPC 3D representations, animations |
| `ui/` | Inventory, skills, chat, minimap panels |
| `input/` | Click-to-move (raycasting to tile grid), camera orbit |
| `net/` | TCP connection, packet send/receive, state sync |

## Server Architecture (C++20)

### Game Loop

```
┌─────────────────────────────────────────────┐
│              600ms TICK                      │
│                                              │
│  1. Accept new connections                   │
│  2. Read all pending client packets          │
│  3. Process action queue:                   │
│     - Movement requests                      │
│     - Skill actions (chop, mine, fish)       │
│     - Combat actions (attack, eat, equip)    │
│     - UI actions (bank, shop, item swap)     │
│  4. Update all entities:                    │
│     - NPC AI ticks                           │
│     - Movement interpolation                 │
│     - Combat resolution                      │
│     - Skill timers                           │
│  5. Update world:                           │
│     - Respawn resources (trees, rocks)        │
│     - Ground item despawn timers             │
│  6. Broadcast regional updates to clients    │
│  7. Periodic player save (every ~100 ticks)  │
└─────────────────────────────────────────────┘
```

### Entity System

```
Entity (base)
├── Player (extends Entity)
│   ├── Credentials (username, password hash)
│   ├── Position (tile x, y, z)
│   ├── Appearance (equipment visuals)
│   ├── Skills (map of skill_id → level + xp)
│   ├── Inventory (28 slots)
│   ├── Equipment (10 slots)
│   ├── Bank (N slots)
│   ├── Combat state (in combat, target, timer)
│   └── Action queue (up to 2 queued actions)
├── NPC (extends Entity)
│   ├── NPC ID → definition lookup
│   ├── Position
│   ├── AI state (idle, wandering, fighting)
│   ├── Combat stats (from definition)
│   └── Respawn timer
└── GroundItem (extends Entity)
    ├── Item ID → definition lookup
    ├── Position
    ├── Quantity
    └── Despawn timer
```

### Regional Update System

The world is divided into **regions** (8×8 tiles). Each tick, the server only sends updates for the region(s) the player is in and adjacent regions. This keeps bandwidth minimal.

```
Player at (50, 50) receives updates for:
  Region (6,6) through Region (7,7) — 2×2 region viewport

Each update packet contains:
  - Player positions in viewport (delta — only changed positions)
  - NPC state changes in viewport
  - Ground item changes (add/remove)
  - Animation triggers (chopping, mining, combat)
```

### Packet Protocol

All packets are length-prefixed binary (1-byte opcode + variable payload).

**Client → Server:**
| Opcode | Packet | Purpose |
|--------|--------|---------|
| 0x01 | LoginRequest | Username, password, client version |
| 0x02 | WalkTile | Destination tile X, Y |
| 0x03 | ActionObject | Object ID, tile position (interact with tree/rock) |
| 0x04 | ActionNPC | NPC index, action type |
| 0x05 | ActionItem | Item slot, action type (eat, equip, drop) |
| 0x06 | ActionInventory | Inventory slot swap |
| 0x07 | ChatMessage | Text, channel |
| 0x08 | EquipItem | Slot → equipment slot |
| 0x09 | UnequipItem | Equipment slot → inventory |

**Server → Client:**
| Opcode | Packet | Purpose |
|--------|--------|---------|
| 0x81 | LoginResponse | Success/fail + player state init |
| 0x82 | RegionUpdate | Entity positions, state changes in viewport |
| 0x83 | PlayerUpdate | Self-position correction, skill XP, level-up |
| 0x84 | InventoryUpdate | Full or delta inventory sync |
| 0x85 | ChatMessage | Server/chat text |
| 0x86 | Animation | NPC/player animation trigger |
| 0x87 | GroundItemAdd | New ground item in viewport |
| 0x88 | GroundItemRemove | Item picked up or despawned |
| 0x89 | SystemMessage | "You need a woodcutting axe" etc. |

### Persistence

Players save to SQLite on a timer (every 100 ticks ≈ 60 seconds).

**Schema:**
```sql
CREATE TABLE players (
    id INTEGER PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    position_x INTEGER, position_y INTEGER, position_z INTEGER,
    appearance TEXT,        -- JSON blob
    skills TEXT,            -- JSON: {"woodcutting": {"xp": 1234, "level": 10}, ...}
    inventory TEXT,          -- JSON: [{item_id, quantity, slot}, ...]
    equipment TEXT,          -- JSON: [{item_id, slot}, ...]
    bank TEXT,               -- JSON: [{item_id, quantity}, ...]
    last_save TIMESTAMP,
    last_login TIMESTAMP
);
```

## Shared Code

Protocol opcodes, packet structures, and game data definitions are in `shared/` as header-only C++. Both client and server include these so packet formats stay in sync.

```
shared/
├── protocol/
│   ├── opcodes.h          # All opcode constants
│   ├── packets.h          # Packet struct definitions
│   └── buffer.h           # Read/write helpers for binary data
├── constants/
│   └── game_constants.h   # TICK_RATE_MS, TILE_SIZE, MAX_PLAYERS, etc.
├── types/
│   ├── skill.h            # Skill enum, XP table
│   ├── item.h             # Item struct
│   └── position.h         # Tile position (x, y, z)
└── game_data/
    ├── item_defs.h         # Static item definitions
    ├── npc_defs.h          # Static NPC definitions
    └── xp_table.h          # XP thresholds per level
```

## Build System

| Component | Build Tool | Dependencies |
|-----------|-----------|--------------|
| Server | CMake + C++20 | SQLite3 (lib), Threads |
| Client | Godot Editor (SCons/CMake) | Godot 4.x |
| Shared | Header-only | None |

## Deployment

### Development (Home Server LXC)
```
# Single process, minimal resources
./erynfall-server --port 43594 --db ./save.db --data ./data/
# ~200MB RAM, SQLite persistence
```

### Production (Later)
```
# systemd service for 24/7 uptime
# PostgreSQL for concurrent access
# Optional: nginx reverse proxy for multiple worlds
```
