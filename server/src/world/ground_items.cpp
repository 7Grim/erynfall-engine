#include "world/ground_items.h"

namespace erynfall {

GroundItemManager::GroundItemManager() = default;

uint32_t GroundItemManager::addItem(uint16_t itemId, uint16_t quantity, int x, int y,
                                     int ownerFd, uint32_t expireTicks) {
    GroundItem item;
    item.id = nextId_++;
    item.itemId = itemId;
    item.quantity = quantity;
    item.position = {x, y, 0};
    item.expireTimer = expireTicks;
    item.ownerFd = ownerFd;
    items_.push_back(item);
    return item.id;
}

bool GroundItemManager::removeItem(uint32_t id) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->id == id) {
            items_.erase(it);
            return true;
        }
    }
    return false;
}

bool GroundItemManager::pickItemAt(int x, int y, int playerFd,
                                   uint16_t& outItemId, uint16_t& outQty) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->position.x == x && it->position.y == y) {
            // Owner protection: only owner can pick up for first 60 ticks (36s)
            if (it->ownerFd != 0 && it->expireTimer > 540 && it->ownerFd != playerFd) {
                continue;
            }
            outItemId = it->itemId;
            outQty = it->quantity;
            items_.erase(it);
            return true;
        }
    }
    return false;
}

const GroundItem* GroundItemManager::itemById(uint32_t id) const {
    for (auto& item : items_) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

void GroundItemManager::tick() {
    for (auto it = items_.begin(); it != items_.end(); ) {
        if (it->expireTimer > 0) {
            it->expireTimer--;
            if (it->expireTimer == 0) {
                it = items_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

int GroundItemManager::countAt(int x, int y) const {
    int count = 0;
    for (auto& item : items_) {
        if (item.position.x == x && item.position.y == y) count++;
    }
    return count;
}

} // namespace erynfall
