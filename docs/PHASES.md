# Erynfall — Phase Roadmap

> Sequential phases. No skipping. Each phase produces a playable build.
> Done criteria must be met before moving to the next phase.

## Phase 0 — Setup
**Goal:** Build pipeline works, cube renders on screen.

### Issues
- [ ] Initialize Godot 4 C++ project structure with GDExtension build
- [ ] Initialize CMake-based C++20 server project
- [ ] Set up shared header-only library (protocol, constants)
- [ ] Render a colored cube on a 3D viewport (client)
- [ ] Server compiles and prints "Listening on port 43594" then exits cleanly
- [ ] Client and server share protocol constants without duplication
- [ ] CI: GitHub Actions — server builds on push, client builds on push
- [ ] README with build instructions works for a fresh clone

### Done Criteria
- [ ] `cmake --build server/build` succeeds
- [ ] `godot --headless client/` runs without errors
- [ ] A cube is visible in the Godot viewport
- [ ] Shared constants compile in both client and server
- [ ] Someone cloning the repo can build both in under 10 minutes

**Estimated time: 1-2 weeks**

---

## Phase 1 — The Grid
**Goal:** Player moves on a tile grid with OSRS-accurate 600ms tick feel.

### Issues
- [ ] Define tile size constants and coordinate system in shared/
- [ ] Generate a flat 30×30 tile mesh (colored tiles, no height variation yet)
- [ ] Implement fixed orbital camera (click-drag to rotate, scroll to zoom)
- [ ] Implement click-to-move: raycast click to tile → move cube there
- [ ] Implement server 600ms tick loop (accurate timing with steady clock)
- [ ] Implement server-side tile map (2D array of tile types)
- [ ] Client → server: walk packet. Server → client: position update
- [ ] Client-side movement interpolation (smooth visual between tiles)
- [ ] Camera follows player (stays centered on player tile)
- [ ] Basic tile types: grass (green), water (blue, impassable), tree tile (occupied)

### Done Criteria
- [ ] Click any grass tile → cube walks there smoothly
- [ ] Click water tile → nothing happens (impassable)
- [ ] Movement takes the correct number of ticks (1 tile per tick)
- [ ] Camera orbits around player when dragged
- [ ] Server processes walk requests on 600ms boundaries

**Estimated time: 2-4 weeks**

---

## Phase 2 — The Loop
**Goal:** One skill works end-to-end. Click tree → chop → get logs → gain XP → level up.

### Issues
- [ ] Define XP table (OSRS-identical level 1-99 curve)
- [ ] Define item definitions (logs, axe types) in JSON
- [ ] Create tree objects on the map (3D placeholder meshes)
- [ ] Click tree → send "action on object" packet to server
- [ ] Server skill system: woodcutting action resolves on tick
- [ ] Chopping has a tick timer (based on axe + woodcutting level)
- [ ] On success: add logs to inventory, grant XP, trigger animation
- [ ] On failure: nothing (OSRS style — you just keep trying)
- [ ] Tree depletes after N chops, respawns after timer
- [ ] Client inventory panel: 28 slots, show item icons
- [ ] Client skills panel: show woodcutting level + XP bar
- [ ] Level-up notification message
- [ ] Server → client: inventory sync, skill update

### Done Criteria
- [ ] Click tree → chopping animation plays → logs appear in inventory
- [ ] XP bar fills as you chop → level up message appears
- [ ] Tree disappears after depletion → respawns after timer
- [ ] Inventory shows correct item count
- [ ] Skills panel shows current level and XP progress

**Estimated time: 3-5 weeks**

---

## Phase 3 — The Fight
**Goal:** Melee combat works. Kill an NPC. Die. Respawn.

### Issues
- [ ] Define NPC definitions (goblin, cow) in JSON
- [ ] Spawn NPCs on the map (static positions, wandering radius)
- [ ] Click NPC → attack (auto-attack continues until death/flee)
- [ ] Attack speed based on weapon (ticks between attacks)
- [ ] Max hit calculation (strength + weapon + equipment bonus)
- [ ] NPC attacks back on its own tick timer
- [ ] HP system (hitpoints skill, damage applied per hit)
- [ ] Zero HP → death. Player respawns at spawn, drops items
- [ ] NPC death → drop loot on ground, respawn timer
- [ ] Combat animations (attack, block, hit splat)
- [ ] Damage numbers / hit splats above entities
- [ ] Player HP display (red bar above head, or UI panel)

### Done Criteria
- [ ] Click goblin → auto-attack loop starts
- [ ] Both player and NPC take damage on their attack ticks
- [ ] NPC dies → loot appears on ground → NPC respawns
- [ ] Player dies → respawn at home → 3 most valuable items kept
- [ ] Hit splats show damage numbers

**Estimated time: 3-5 weeks**

---

## Phase 4 — The Gear
**Goal:** Equipment matters. Shops exist. Food heals.

### Issues
- [ ] Equipment slots UI (head, cape, body, legs, weapon, shield, gloves, boots, amulet, ring)
- [ ] Equip/unequip from inventory (right-click or button)
- [ ] Equipment bonuses applied to combat stats
- [ ] Food items (eat from inventory → heal HP)
- [ ] Shop NPCs (click → buy/sell interface)
- [ ] Buy items from shop (gold deducted, item added to inventory)
- [ ] Sell items to shop (item removed, gold added)
- [ ] Weapon variety (different attack speeds, styles, damage bonuses)
- [ ] Player gold tracking and display

### Done Criteria
- [ ] Equip bronze sword → attack damage increases
- [ ] Eat shrimp → HP restored
- [ ] Buy item from shop → gold deducted, item in inventory
- [ ] Sell logs to shop → gold added
- [ ] Different weapons have different attack speeds

**Estimated time: 3-4 weeks**

---

## Phase 5 — The World
**Goal:** Multiple zones, bank, larger world, more reasons to explore.

### Issues
- [ ] Map expansion: multiple zones connected (town, forest, mine, combat area)
- [ ] Height variation in terrain (different Z levels per tile)
- [ ] Bank building (NPC or object) → bank UI → store/retrieve items
- [ ] Pathfinding (A* on tile grid, handle obstacles)
- [ ] Multiple NPC types per zone
- [ ] Multiple resource types (trees → woodcutting, rocks → mining, fishing spots)
- [ ] New skills: mining, fishing
- [ ] Zone transitions (walking between areas)
- [ ] Minimap UI (shows nearby terrain, player dot)

### Done Criteria
- [ ] Walk between 3+ distinct zones
- [ ] Bank stores items across sessions
- [ ] Mining and fishing work with same pattern as woodcutting
- [ ] Minimap shows current area

**Estimated time: 4-6 weeks**

---

## Phase 6 — The Grind
**Goal:** Progression feels meaningful. Multiple skills, XP curve, reasons to keep playing.

### Issues
- [ ] Full XP table (all 23 skills, level 1-99)
- [ ] Multiple gathering skills active (woodcutting, mining, fishing, cooking)
- [ ] Processing skills (smelting ore → bars, cooking raw fish → cooked fish)
- [ ] Equipment crafting (smithing bars → weapons/armor)
- [ ] Skill total level display
- [ ] Unlock progression (higher-level resources require higher skill)
- [ ] Multiple item tiers per skill level range
- [ ] Economy balance (item values, shop prices, drop tables)

### Done Criteria
- [ ] Mine ore → smelt bars → smith sword → equip → use in combat
- [ ] Fish → cook → eat to heal
- [ ] Skill total accurately reflects all skill levels
- [ ] Higher-level resources require higher skill to gather
- [ ] There is a clear progression loop: gather → process → use

**Estimated time: 4-8 weeks**

---

## Phase 7 — Multiplayer
**Goal:** Two clients see each other. Game state syncs correctly.

### Issues
- [ ] Server accepts multiple TCP connections (up to 16 for dev)
- [ ] Player login flow (username/password, load from DB, assign to world)
- [ ] Regional update system (each player gets updates for their viewport)
- [ ] Player appearance sync (equipment visuals seen by others)
- [ ] Chat system (messages broadcast to nearby players)
- [ ] Player-to-player visibility (see other players in viewport)
- [ ] Conflict resolution (two players clicking same tree/NPC)
- [ ] Server persistence: save all players on clean shutdown
- [ ] Player disconnect handling (save state, remove from world)
- [ ] Basic anti-cheat: server validates all actions, reject invalid packets

### Done Criteria
- [ ] Two clients connect, see each other moving in real-time
- [ ] Both clients see correct regional updates
- [ ] Chat messages appear for nearby players
- [ ] One player disconnects → they disappear for the other
- [ ] Reconnect → player state restored
- [ ] Server can run 24/7 without leaking memory

**Estimated time: 6-10 weeks**

---

## Timeline Summary

| Phase | Est. Time | Cumulative |
|-------|-----------|------------|
| 0 — Setup | 1-2 weeks | 2 weeks |
| 1 — The Grid | 2-4 weeks | 6 weeks |
| 2 — The Loop | 3-5 weeks | 11 weeks |
| 3 — The Fight | 3-5 weeks | 16 weeks |
| 4 — The Gear | 3-4 weeks | 20 weeks |
| 5 — The World | 4-6 weeks | 26 weeks |
| 6 — The Grind | 4-8 weeks | 34 weeks |
| 7 — Multiplayer | 6-10 weeks | 44 weeks |

**Total estimated: 8-10 months to a multiplayer vertical slice.**

These estimates assume 10-15 hours/week of development time. Actual time will vary.
