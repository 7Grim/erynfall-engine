#pragma once

#include "types/item.h"
#include "constants/game_constants.h"
#include <cstdint>
#include <array>

namespace erynfall {

// Player inventory — 28 slots
class Inventory {
public:
    Inventory() { slots_.fill({0, 0}); }

    // Add an item. Returns true if added, false if full.
    bool addItem(uint16_t itemId, uint16_t quantity = 1);

    // Remove an item. Returns true if removed, false if not found.
    bool removeItem(uint16_t itemId, uint16_t quantity = 1);

    // Get slot contents
    const ItemStack& slot(int index) const { return slots_[index]; }

    // Check if player has at least N of an item
    bool hasItem(uint16_t itemId, uint16_t quantity = 1) const;

    // Count total quantity of an item across all slots
    uint16_t count(uint16_t itemId) const;

    // Find first slot containing this item (-1 if none)
    int findItem(uint16_t itemId) const;

    // Find first empty slot (-1 if full)
    int firstEmpty() const;

    // Serialize to buffer (for network sync)
    // Format: 28 * (uint16 id, uint16 qty) = 112 bytes
    void serialize(uint8_t* buf) const;

    // Total items
    int size() const { return INVENTORY_SIZE; }

private:
    std::array<ItemStack, INVENTORY_SIZE> slots_;
};

} // namespace erynfall
