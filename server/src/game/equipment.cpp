#include "game/equipment.h"
#include "game_data/item_defs.h"
#include <algorithm>

namespace erynfall {

ItemStack Equipment::equip(uint16_t itemId, uint8_t playerAttackLevel, uint8_t playerDefenceLevel) {
    // Look up the item definition to find the equip slot
    auto itemDef = getItemDef(itemId);
    if (itemDef.equipSlot == static_cast<uint16_t>(EquipSlot::NONE)) {
        return {0, 0}; // Not equippable
    }

    EquipSlot targetSlot = static_cast<EquipSlot>(itemDef.equipSlot);
    if (targetSlot == EquipSlot::NONE || static_cast<int>(targetSlot) >= SLOT_COUNT) {
        return {0, 0};
    }

    // Check weapon requirements (attack level)
    // Check armour requirements (defence level)
    switch (itemId) {
        // Weapons — require attack level
        case ItemID::BRONZE_SWORD:
            if (playerAttackLevel < 1) return {0, 0};
            break;
        case ItemID::IRON_SWORD:
            if (playerAttackLevel < 1) return {0, 0};
            break;
        case ItemID::STEEL_SWORD:
            if (playerAttackLevel < 5) return {0, 0};
            break;
        // Shields — require defence level
        case ItemID::BRONZE_SHIELD:
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        case ItemID::IRON_SHIELD:
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        // Body armour
        case 40: // Bronze platebody
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        case 41: // Iron platebody
            if (playerDefenceLevel < 5) return {0, 0};
            break;
        // Leg armour
        case 50: // Bronze platelegs
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        case 51: // Iron platelegs
            if (playerDefenceLevel < 5) return {0, 0};
            break;
        // Helmets
        case 60: // Bronze helm
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        case 61: // Iron helm
            if (playerDefenceLevel < 1) return {0, 0};
            break;
        default:
            break;
    }

    // Swap: save old item, put new one in
    int slotIdx = static_cast<int>(targetSlot);
    ItemStack previous = slots_[slotIdx];
    slots_[slotIdx] = {itemId, 1};

    return previous; // Returns {0,0} if slot was empty, or old item if swapped
}

ItemStack Equipment::unequip(EquipSlot slot) {
    if (slot == EquipSlot::NONE || static_cast<int>(slot) >= SLOT_COUNT) {
        return {0, 0};
    }
    int slotIdx = static_cast<int>(slot);
    ItemStack previous = slots_[slotIdx];
    slots_[slotIdx] = {0, 0};
    return previous;
}

} // namespace erynfall
