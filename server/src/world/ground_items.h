#pragma once

#include "types/position.h"
#include <cstdint>
#include <vector>
#include <string>

namespace erynfall {

// Ground item — dropped loot or items on the floor
struct GroundItem {
    uint32_t id;          // Unique ID
    uint16_t itemId;      // Item definition ID
    uint16_t quantity;
    Position position;
    uint32_t expireTimer;  // Ticks until despawn (0 = permanent)
    int ownerFd;           // Owner FD for trade protection (0 = public)
};

// Manages ground items
class GroundItemManager {
public:
    GroundItemManager();
    
    // Add item to ground, returns new item ID
    uint32_t addItem(uint16_t itemId, uint16_t quantity, int x, int y,
                     int ownerFd = 0, uint32_t expireTicks = 600);
    
    // Remove item by ID
    bool removeItem(uint32_t id);
    
    // Pick up item at position for player (returns true if picked up)
    bool pickItemAt(int x, int y, int playerFd, uint16_t& outItemId, uint16_t& outQty);
    
    // Get all items (for syncing)
    const std::vector<GroundItem>& all() const { return items_; }
    
    // Get item by ID
    const GroundItem* itemById(uint32_t id) const;
    
    // Tick — decrement expire timers, remove expired
    void tick();
    
    // Count items at position
    int countAt(int x, int y) const;

private:
    std::vector<GroundItem> items_;
    uint32_t nextId_ = 1;
};

} // namespace erynfall
