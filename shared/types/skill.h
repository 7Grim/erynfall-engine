#pragma once

#include <cstdint>

namespace erynfall {

// OSRS-identical skill IDs
enum class Skill : uint8_t {
    Attack       = 0,
    Strength     = 1,
    Defence      = 2,
    Hitpoints    = 3,
    Ranged       = 4,
    Prayer       = 5,
    Magic        = 6,
    Cooking      = 7,
    Woodcutting  = 8,
    Fletching    = 9,
    Fishing      = 10,
    Firemaking   = 11,
    Crafting     = 12,
    Smithing     = 13,
    Mining       = 14,
    Herblore     = 15,
    Agility      = 16,
    Thieving     = 17,
    Slayer       = 18,
    Farming      = 19,
    Runecrafting = 20,
    Hunter       = 21,
    Construction = 22,
    COUNT        = 23,  // Total skill count
};

struct SkillState {
    uint8_t  level = 1;
    uint32_t xp = 0;

    // Calculate level from XP (OSRS formula)
    static uint8_t levelFromXP(uint32_t xp);

    // XP needed for a given level (OSRS curve)
    static uint32_t xpForLevel(uint8_t level);
};

} // namespace erynfall
