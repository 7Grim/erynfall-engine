#pragma once

#include "types/item.h"
#include "constants/game_constants.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace erynfall {

// Equipment container — one item per equip slot
class Equipment {
public:
    Equipment() { slots_.fill({0, 0}); }

    // Equip an item into the correct slot. Returns the previously equipped item.
    // If slot is wrong or level req not met, returns {0,0} (failure).
    ItemStack equip(uint16_t itemId, uint8_t playerAttackLevel, uint8_t playerDefenceLevel);

    // Unequip item from a slot. Returns the unequipped item.
    ItemStack unequip(EquipSlot slot);

    // Get item in a slot
    const ItemStack& slot(EquipSlot s) const { return slots_[static_cast<int>(s)]; }
    ItemStack& slot(EquipSlot s) { return slots_[static_cast<int>(s)]; }

    // Check if a specific item is equipped anywhere
    bool hasItem(uint16_t itemId) const;

    // Get the equipped weapon bonus (strength bonus to max hit)
    int getStrengthBonus() const;

    // Get the equipped weapon attack speed (ticks between attacks)
    int getAttackSpeed() const;

    // Get total defence bonus from all equipped armour
    int getDefenceBonus() const;

    // Serialize to buffer (10 slots × 4 bytes = 40 bytes)
    // Format per slot: uint16 itemId, uint16 quantity (always 0 or 1)
    void serialize(uint8_t* buf) const;

    // Total slots
    static constexpr int SLOT_COUNT = 10;

private:
    std::array<ItemStack, SLOT_COUNT> slots_;
};

// --- Inline implementations ---

inline bool Equipment::hasItem(uint16_t itemId) const {
    for (const auto& slot : slots_) {
        if (slot.id == itemId && slot.quantity > 0) return true;
    }
    return false;
}

inline int Equipment::getStrengthBonus() const {
    // Weapon slot
    auto& weapon = slots_[static_cast<int>(EquipSlot::Weapon)];
    if (weapon.quantity == 0) return 0;
    // Strength bonus comes from weapon type
    // We'll use the item value as a proxy for power tier
    switch (weapon.id) {
        case 20: return 4;   // Bronze sword
        case 21: return 7;   // Iron sword
        case 22: return 14;  // Steel sword
        default: return 3;    // Bare fists fallback
    }
}

inline int Equipment::getAttackSpeed() const {
    auto& weapon = slots_[static_cast<int>(EquipSlot::Weapon)];
    if (weapon.quantity == 0) return 5; // Bare fists
    switch (weapon.id) {
        case 20: return 4;   // Bronze sword
        case 21: return 4;   // Iron sword
        case 22: return 4;   // Steel sword
        default: return 4;   // Default weapon speed
    }
}

inline int Equipment::getDefenceBonus() const {
    int bonus = 0;
    // Shield slot
    auto& shield = slots_[static_cast<int>(EquipSlot::Shield)];
    switch (shield.id) {
        case 30: bonus += 3;  // Bronze shield
        case 31: bonus += 5;  // Iron shield
        default: break;
    }
    // Body slot (future: add armor items)
    auto& body = slots_[static_cast<int>(EquipSlot::Body)];
    if (body.quantity > 0) {
        switch (body.id) {
            case 40: bonus += 4;  // Bronze platebody
            case 41: bonus += 7;  // Iron platebody
            default: break;
        }
    }
    // Legs slot (future: add leg items)
    auto& legs = slots_[static_cast<int>(EquipSlot::Legs)];
    if (legs.quantity > 0) {
        switch (legs.id) {
            case 50: bonus += 2;  // Bronze platelegs
            case 51: bonus += 4;  // Iron platelegs
            default: break;
        }
    }
    // Helmet slot
    auto& head = slots_[static_cast<int>(EquipSlot::Head)];
    if (head.quantity > 0) {
        switch (head.id) {
            case 60: bonus += 2;  // Bronze helm
            case 61: bonus += 3;  // Iron helm
            default: break;
        }
    }
    return bonus;
}

inline void Equipment::serialize(uint8_t* buf) const {
    for (int i = 0; i < SLOT_COUNT; ++i) {
        int off = i * 4;
        buf[off + 0] = static_cast<uint8_t>(slots_[i].id & 0xFF);
        buf[off + 1] = static_cast<uint8_t>((slots_[i].id >> 8) & 0xFF);
        buf[off + 2] = static_cast<uint8_t>(slots_[i].quantity & 0xFF);
        buf[off + 3] = static_cast<uint8_t>((slots_[i].quantity >> 8) & 0xFF);
    }
}

} // namespace erynfall
