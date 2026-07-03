# Erynfall — Design Document

> Living document. Updated as decisions are made. Once a section is marked ✅, it's locked unless a critical issue forces a change.

## Vision

An OSRS mechanically-identical MMO in an original setting (Wild West + Hell Realm). The feel must be indistinguishable from playing 2007-era RuneScape — same tick timing, same click interactions, same progression philosophy.

## Core Pillars (Non-Negotiable)

| Pillar | Description |
|--------|-------------|
| **600ms Tick** | Everything runs on a 600ms game loop. Actions queue and resolve on tick boundaries. This IS the OSRS feel. |
| **Click-Driven** | Every action is initiated by clicking. No WASD movement. Click tile → walk. Click tree → chop. Click NPC → interact. |
| **Fixed Camera** | Orbital camera around the player, not free-look. Camera angle is set by player preference (drag to rotate), not by gameplay. |
| **Tile-Based World** | World is a discrete grid. Players, NPCs, items, objects all occupy tile positions. Movement is tile-to-tile. |
| **Grind as Gameplay** | Progression is slow and deliberate. Skills take real time. Levels feel earned. No fast travel, no XP boosts. |
| **Risk/Reward** | Death has consequences (drop items). Dangerous areas have better rewards. The wilderness dynamic. |
| **Simplicity** | Low-poly 3D. Minimal UI chrome. Text-heavy interactions. If it doesn't serve the gameplay, cut it. |

## Setting

- **Theme**: Wild West frontier + Hell Realm
- **Antagonists**: Demons
- **Aesthetic**: Dusty frontier towns → volcanic wastelands
- **Player archetype**: Outlaw / Demon Hunter

> Full setting/lore details live in [7Grim/erynfall-plan](https://github.com/7Grim/erynfall-plan). This doc focuses on mechanics.

## Gameplay Systems

### Skills ✅ (Locked)

OSRS-identical skill system. All 23 skills with exact XP formulas.

**MVP Skills (Phase 2+):**
- Woodcutting (first skill)
- Mining
- Fishing
- Cooking
- Firemaking

**Combat Skills (Phase 3+):**
- Attack, Strength, Defence, Hitpoints
- Ranged (later)
- Magic (later)

**Later Skills:**
- Smithing, Crafting, Fletching, Herblore
- Agility, Thieving, Slayer
- Farming, Runecrafting, Construction
- Hunter, Summoning

### Combat ✅ (Locked)

OSRS-identical melee combat system:
- 3 attack styles: Stab, Slash, Crush
- Attack speed based on weapon (ticks per attack)
- Max hit formula based on style + equipment + prayers
- Auto-retaliate by default, manual targeting by clicking
- Special attacks (later)

### Economy ✅ (Locked)

- Gold as currency
- Shop-based buying/selling (no Grand Exchange for MVP)
- Item drop tables on NPCs
- No trade between players for MVP (later: limited trade)

### Inventory & Banking ✅ (Locked)

- 28-slot inventory
- Equipment slots: Head, Cape, Body, Legs, Weapon, Shield, Gloves, Boots, Amulet, Ring
- Bank storage (unlimited slots, maybe tabbed later)
- Item stacking (noted items for bank, raw items in inv)

### Death ✅ (Locked)

- Die → respawn at selected spawn point
- Drop all items except 3 most valuable (kept on death)
- If in safe area → keep all items
- Gravestone mechanic (later, for longer timer)

## Technical Decisions ✅ (Locked)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Client Engine | Godot 4 + C++ GDExtension | Best balance of C++ learning + rapid dev |
| Server Engine | Custom C++20 + CMake | Full control over tick loop and game state |
| Networking | TCP (not UDP) | OSRS uses TCP. Tick rate is slow enough. Reliable delivery. |
| Tick Rate | 600ms | OSRS standard. Non-negotiable. |
| Persistence | SQLite (dev) / PostgreSQL (prod) | Simple for dev, scalable for prod |
| Graphics | Low-poly 3D | Matches OSRS aesthetic, achievable for solo dev |
| Camera | Fixed orbital | OSRS feel. Not free-look. |
| Movement | Click-to-move on tile grid | OSRS feel. No WASD. |
| Tile Size | 1 tile = 1 unit | Matches OSRS convention |
| Map Format | JSON heightmap + object placement | Simple, human-readable, no proprietary format |

## What This Is NOT

- ❌ Not an MMO on day one (single-player first)
- ❌ Not a custom engine (Godot for client, custom server only)
- ❌ Not microservices (single server process)
- ❌ Not real-time combat (tick-based, like OSRS)
- ❌ Not a fast-paced game (deliberate, grindy)
- ❌ Not mobile-first (desktop PC primary)

## Open Questions

| Question | Status | Notes |
|----------|--------|-------|
| Exact art style reference? | 🔲 Open | Need mockups/screenshots to align on |
| Map editor or hand-coded maps? | 🔲 Open | JSON by hand for MVP, tool later |
| Sound system scope? | 🔲 Open | Minimal — basic SFX only for MVP |
| Localization? | 🔲 Open | English only for MVP |
| Chat system scope? | 🔲 Open | Local-only for singleplayer, server chat later |
