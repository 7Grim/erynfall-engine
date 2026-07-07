#pragma once

#include "types/skill.h"
#include <cstdint>
#include <string>

namespace erynfall {

// NPC type IDs
enum NPCType : uint8_t {
    GOBLIN = 0,
    COW    = 1,
    CHICKEN = 2,
    SHOPKEEPER = 3,  // Phase 4: shop NPC (non-hostile)
};

// NPC definition — stats and behavior per type
struct NPCDef {
    NPCType type;
    const char* name;
    uint8_t combatLevel;
    uint16_t hitpoints;      // Max HP
    uint8_t maxHit;           // Max damage per hit (0-based OSRS style)
    uint8_t attackSpeed;     // Ticks between attacks
    uint8_t attackRange;     // Distance for melee
    uint16_t attackXP;        // XP granted per hit
    uint16_t deathXP;         // XP granted on kill
    uint32_t respawnTime;     // Ticks until respawn (0 = doesn't respawn)
    uint16_t lootItems[4];    // Item IDs dropped on death (0 = empty)
    uint8_t  lootQty[4];     // Quantity for each drop
    uint8_t wanderRadius;    // Tiles NPC wanders from spawn
    uint8_t bodyColorR;       // Visual color (placeholder cubes)
    uint8_t bodyColorG;
    uint8_t bodyColorB;
};

inline constexpr NPCDef NPC_DEFS[] = {
    // GOBLIN
    {
        NPCType::GOBLIN, "Goblin",
        2,           // combat level
        5,           // hitpoints
        1,           // max hit
        4,           // attack speed (ticks)
        1,           // attack range
        8,           // attack XP per hit
        20,          // death XP
        100,         // respawn ticks (60s)
        {998, 0, 0, 0}, // drops: 1 coin
        {5, 0, 0, 0},
        3,           // wander radius
        80, 180, 60, // green body
    },
    // COW
    {
        NPCType::COW, "Cow",
        2,           // combat level
        8,           // hitpoints
        1,           // max hit
        5,           // attack speed
        1,           // attack range
        10,          // attack XP per hit
        30,          // death XP
        100,         // respawn ticks
        {0, 0, 0, 0}, // no loot in Phase 3
        {0, 0, 0, 0},
        2,           // wander radius
        160, 140, 120, // brown body
    },
    // CHICKEN
    {
        NPCType::CHICKEN, "Chicken",
        1,           // combat level
        3,           // hitpoints
        0,           // max hit (chickens deal 0 damage)
        5,           // attack speed
        1,           // attack range
        5,           // attack XP per hit
        10,          // death XP
        50,          // respawn ticks (30s)
        {998, 0, 0, 0}, // drops: 1-5 coins
        {3, 0, 0, 0},
        2,           // wander radius
        240, 230, 200, // light tan body
    },
    // SHOPKEEPER (Phase 4: non-hostile shop NPC)
    {
        NPCType::SHOPKEEPER, "Shopkeeper",
        0,           // combat level (civilian)
        10,          // hitpoints (not attackable)
        0,           // max hit (deals no damage)
        99,          // attack speed (doesn't fight back)
        0,           // attack range (doesn't fight)
        0,           // attack XP
        0,           // death XP
        0,           // respawn ticks (doesn't respawn — persistent)
        {0, 0, 0, 0}, // no drops
        {0, 0, 0, 0},
        0,           // wander radius (stays in place)
        60, 60, 180, // blue body (distinctive)
    },
};

inline constexpr int NPC_DEF_COUNT = 4;

inline const NPCDef& getNPCDef(NPCType type) {
    return NPC_DEFS[static_cast<int>(type)];
}

} // namespace erynfall
