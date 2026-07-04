#pragma once

#include <cstdint>

namespace erynfall {

struct ItemDef {
    uint16_t id;
    const char* name;
    bool stackable;
    uint16_t equipSlot;      // 0xFF = not equippable
    uint16_t value;          // Gold value
};

enum class EquipSlot : uint8_t {
    Head    = 0,
    Cape    = 1,
    Amulet  = 2,
    Weapon  = 3,
    Body    = 4,
    Shield  = 5,
    Legs    = 6,
    Gloves  = 7,
    Boots   = 8,
    Ring    = 9,
    NONE    = 0xFF,
};

struct ItemStack {
    uint16_t id = 0;
    uint16_t quantity = 0;
};

} // namespace erynfall
