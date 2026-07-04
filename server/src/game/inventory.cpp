#include "game/inventory.h"
#include "game_data/item_defs.h"
#include <cstring>

namespace erynfall {

bool Inventory::addItem(uint16_t itemId, uint16_t quantity) {
    auto def = getItemDef(itemId);

    if (def.stackable) {
        // Try to stack onto existing slot
        int slot = findItem(itemId);
        if (slot >= 0) {
            slots_[slot].quantity += quantity;
            return true;
        }
    }

    // Find empty slot(s) and add
    while (quantity > 0) {
        int slot = firstEmpty();
        if (slot < 0) return false;  // Full

        uint16_t add = (def.stackable) ? quantity : 1;
        slots_[slot] = {itemId, add};
        quantity -= add;
    }
    return true;
}

bool Inventory::removeItem(uint16_t itemId, uint16_t quantity) {
    while (quantity > 0) {
        int slot = findItem(itemId);
        if (slot < 0) return false;

        uint16_t remove = std::min(quantity, slots_[slot].quantity);
        slots_[slot].quantity -= remove;
        if (slots_[slot].quantity == 0) {
            slots_[slot] = {0, 0};
        }
        quantity -= remove;
    }
    return true;
}

bool Inventory::hasItem(uint16_t itemId, uint16_t quantity) const {
    return count(itemId) >= quantity;
}

uint16_t Inventory::count(uint16_t itemId) const {
    uint16_t total = 0;
    for (const auto& slot : slots_) {
        if (slot.id == itemId) {
            total += slot.quantity;
        }
    }
    return total;
}

int Inventory::findItem(uint16_t itemId) const {
    for (int i = 0; i < INVENTORY_SIZE; ++i) {
        if (slots_[i].id == itemId && slots_[i].quantity > 0) {
            return i;
        }
    }
    return -1;
}

int Inventory::firstEmpty() const {
    for (int i = 0; i < INVENTORY_SIZE; ++i) {
        if (slots_[i].id == 0 || slots_[i].quantity == 0) {
            return i;
        }
    }
    return -1;
}

void Inventory::serialize(uint8_t* buf) const {
    for (int i = 0; i < INVENTORY_SIZE; ++i) {
        // Little-endian uint16
        buf[i * 4 + 0] = static_cast<uint8_t>(slots_[i].id & 0xFF);
        buf[i * 4 + 1] = static_cast<uint8_t>((slots_[i].id >> 8) & 0xFF);
        buf[i * 4 + 2] = static_cast<uint8_t>(slots_[i].quantity & 0xFF);
        buf[i * 4 + 3] = static_cast<uint8_t>((slots_[i].quantity >> 8) & 0xFF);
    }
}

} // namespace erynfall
