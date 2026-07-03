# Erynfall Engine — Network Protocol

> Binary protocol over TCP. All packets: 1-byte length prefix (if < 256 bytes) + 1-byte opcode + payload.

## Packet Format

```
[Length: 1 byte][Opcode: 1 byte][Payload: variable]
```

If payload exceeds 255 bytes, a 2-byte length prefix is used instead.

## Client → Server Packets

| Opcode | Name | Payload | Description |
|--------|------|---------|-------------|
| 0x01 | LoginRequest | username(UTF8), password_hash(32B), client_version(u16) | Initial connection handshake |
| 0x02 | WalkTile | dest_x(u16), dest_y(u16) | Click-to-move request |
| 0x03 | ActionObject | object_id(u16), tile_x(u16), tile_y(u16), action(u8) | Interact with world object (tree, rock, bank) |
| 0x04 | ActionNPC | npc_index(u16), action(u8) | Interact with NPC (attack, talk, trade) |
| 0x05 | ActionGroundItem | item_index(u16), action(u8) | Pick up or examine ground item |
| 0x06 | InventoryAction | slot(u8), action(u8), target_slot(u8) | Equip, drop, swap inventory items |
| 0x07 | EquipmentAction | slot(u8), action(u8) | Unequip equipment |
| 0x08 | ChatMessage | channel(u8), text(UTF8) | Player chat |
| 0x09 | KeepAlive | | Heartbeat (sent every 30 ticks) |

## Server → Client Packets

| Opcode | Name | Payload | Description |
|--------|------|---------|-------------|
| 0x81 | LoginResponse | status(u8), player_data(variable) | Login success/fail + initial state |
| 0x82 | RegionUpdate | region_data(variable) | Delta update for player's viewport |
| 0x83 | PlayerUpdate | position, skills, inventory delta | Self-state correction from server |
| 0x84 | InventorySync | full_inventory(variable) | Full inventory replacement (on login, major change) |
| 0x85 | EquipmentSync | equipment(variable) | Full equipment sync |
| 0x86 | SkillUpdate | skill_id(u8), xp(u32), level(u8) | Single skill XP/level change |
| 0x87 | Animation | entity_id(u16), anim_id(u16) | Trigger animation on entity |
| 0x88 | GroundItemAdd | item_id(u16), x(u16), y(u16), quantity(u16) | New ground item visible |
| 0x89 | GroundItemRemove | item_index(u16) | Ground item removed (picked up / despawned) |
| 0x90 | SystemMessage | type(u8), text(UTF8) | Info/error text ("You need a woodcutting axe") |
| 0x91 | PlayerAppearance | player_id(u16), appearance(variable) | Other player's equipment visuals |
| 0x92 | PlayerAddRemove | player_id(u16), action(u8), position | Player entered/left viewport |
| 0x93 | NPCUpdate | npc_index(u16), position, animation, hp | NPC state change |
| 0x94 | KeepAliveResponse | | Heartbeat reply |
| 0x95 | LogoutResponse | status(u8) | Disconnect acknowledgement |

## Login Flow

```
Client                              Server
  │                                    │
  │──── LoginRequest ────────────────→│
  │     (username, password, version)   │
  │                                    │ Check credentials
  │                                    │ Load player data
  │←─── LoginResponse ───────────────│
  │     (status + player state)        │
  │                                    │
  │──── WalkTile ────────────────────→│
  │←─── PlayerUpdate ────────────────│
  │←─── RegionUpdate ─────────────────│
  │                                    │
```

## Keepalive

Client sends KeepAlive every 30 ticks (18 seconds). If server doesn't receive one within 60 ticks, disconnect. Prevents zombie connections.
