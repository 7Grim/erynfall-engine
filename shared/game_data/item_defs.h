#pragma once

#include "types/item.h"
#include "types/skill.h"
#include <array>
#include <cstdint>

namespace erynfall {

// Item IDs
enum ItemID : uint16_t {
    // Logs
    NORMAL_LOGS    = 0,
    OAK_LOGS       = 1,
    WILLOW_LOGS    = 2,
    MAPLE_LOGS     = 3,
    YEW_LOGS       = 4,
    MAGIC_LOGS     = 5,

    // Axes
    BRONZE_AXE     = 10,
    IRON_AXE       = 11,
    STEEL_AXE      = 12,
    MITHRIL_AXE    = 13,
    ADAMANT_AXE    = 14,
    RUNE_AXE       = 15,
    DRAGON_AXE     = 16,

    // Swords (equipSlot = Weapon)
    BRONZE_SWORD   = 20,
    IRON_SWORD     = 21,
    STEEL_SWORD    = 22,

    // Shields (equipSlot = Shield)
    BRONZE_SHIELD  = 30,
    IRON_SHIELD    = 31,

    // Body armour (equipSlot = Body)
    BRONZE_PLATEBODY = 40,
    IRON_PLATEBODY   = 41,

    // Leg armour (equipSlot = Legs)
    BRONZE_PLATELEGS = 50,
    IRON_PLATELEGS   = 51,

    // Helmets (equipSlot = Head)
    BRONZE_HELM = 60,
    IRON_HELM   = 61,

    // Gloves (equipSlot = Gloves)
    BRONZE_GLOVES = 70,
    IRON_GLOVES   = 71,

    // Boots (equipSlot = Boots)
    BRONZE_BOOTS = 80,
    IRON_BOOTS   = 81,

    // Amulets (equipSlot = Amulet)
    AMULET_OF_STRENGTH = 90,
    AMULET_OF_DEFENCE   = 91,

    // Food (heals HP, not equippable)
    SHRIMP         = 100,
    ANCHOVIES      = 101,
    SARDINE        = 102,
    TROUT          = 103,
    SALMON         = 104,
    LOBSTER        = 105,
    SWORDFISH      = 106,
    COOKED_SHRIMP  = 110,
    COOKED_ANCHOVIES = 111,
    COOKED_SARDINE = 112,
    COOKED_TROUT   = 113,
    COOKED_SALMON  = 114,
    COOKED_LOBSTER = 115,
    COOKED_SWORDFISH = 116,

    // Pickaxes
    COPPER_PICKAXE = 200,
    IRON_PICKAXE   = 201,
    STEEL_PICKAXE  = 202,

    // Ores
    COPPER_ORE     = 210,
    IRON_ORE       = 211,
    GOLD_ORE       = 212,
    MITHRIL_ORE    = 213,

    // Raw fish (caught via Fishing)
    RAW_SHRIMP     = 300,
    RAW_SARDINE    = 301,
    RAW_TROUT      = 302,

    // Fishing rod
    FISHING_ROD    = 310,

    // Misc
    COINS         = 998,
};

// Tree type definitions
enum class TreeType : uint8_t {
    Normal  = 0,
    Oak     = 1,
    Willow  = 2,
    Maple   = 3,
    Yew     = 4,
    Magic   = 5,
};

struct TreeDef {
    TreeType type;
    const char* name;
    int woodcuttingLevel;      // Required woodcutting level
    int chopTicksMin;           // Min ticks to chop
    int chopTicksMax;           // Max ticks to chop
    uint16_t logItem;           // Item ID of logs produced
    uint32_t xpPerChop;         // XP granted per successful chop
    int hp;                     // Chops before depleted
    int respawnTicks;          // Ticks to respawn after depletion
};

// All tree definitions (OSRS-accurate values)
inline constexpr std::array<TreeDef, 6> TREE_DEFS = {{
    { TreeType::Normal,  "Tree",     1,  4,  4,  ItemID::NORMAL_LOGS,  25,   1,  30  },
    { TreeType::Oak,     "Oak",      15, 8,  14, ItemID::OAK_LOGS,     37,   3,  50  },
    { TreeType::Willow,  "Willow",   30, 10, 18, ItemID::WILLOW_LOGS,  68,   4,  60  },
    { TreeType::Maple,   "Maple",    45, 14, 22, ItemID::MAPLE_LOGS,   100,  5,  80  },
    { TreeType::Yew,     "Yew",      60, 24, 36, ItemID::YEW_LOGS,     175,  8,  120 },
    { TreeType::Magic,   "Magic",    75, 36, 54, ItemID::MAGIC_LOGS,   250, 10,  200 },
}};

// Axe definitions
struct AxeDef {
    const char* name;
    int woodcuttingBonus;        // Reduces chop ticks
    int equipLevel;              // Required attack level to equip
    uint16_t itemId;
};

inline constexpr std::array<AxeDef, 7> AXE_DEFS = {{
    { "Bronze axe",  0,  1,  ItemID::BRONZE_AXE  },
    { "Iron axe",    1,  1,  ItemID::IRON_AXE    },
    { "Steel axe",   2,  5,  ItemID::STEEL_AXE   },
    { "Mithril axe",3, 20,  ItemID::MITHRIL_AXE  },
    { "Adamant axe",4, 30,  ItemID::ADAMANT_AXE  },
    { "Rune axe",    5, 40,  ItemID::RUNE_AXE    },
    { "Dragon axe", 6, 60,  ItemID::DRAGON_AXE   },
}};

// ---------------------------------------------------------------
// Mining: rocks + pickaxes + ores
// ---------------------------------------------------------------

enum class RockType : uint8_t {
    Copper   = 0,
    Iron     = 1,
    Gold     = 2,
    Mithril  = 3,
};

struct RockDef {
    RockType rockType;
    const char* name;
    int miningLevel;          // Required mining level
    int mineTicksMin;         // Min ticks per successful mine
    int mineTicksMax;         // Max ticks per successful mine
    uint16_t oreItem;         // Item ID of ore produced
    uint32_t xpPerMine;       // XP granted per successful mine
    int hp;                   // Mines before depleted
    int respawnTicks;         // Ticks to respawn after depletion
};

// All rock definitions (indexable by RockType)
inline constexpr std::array<RockDef, 4> ROCK_DEFS = {{
    { RockType::Copper,  "Copper rock",   1,   4,  8,  ItemID::COPPER_ORE,   20,   3,  40  },
    { RockType::Iron,    "Iron rock",    10,   6, 12,  ItemID::IRON_ORE,     35,   4,  60  },
    { RockType::Gold,    "Gold rock",    30,  10, 18,  ItemID::GOLD_ORE,     70,   5,  90  },
    { RockType::Mithril, "Mithril rock", 45,  14, 24,  ItemID::MITHRIL_ORE, 100,   6, 120 },
}};

// Pickaxe definitions
struct PickaxeDef {
    const char* name;
    int miningBonus;         // Reduces mine ticks
    int equipLevel;          // Required attack level to equip
    uint16_t itemId;
};

inline constexpr std::array<PickaxeDef, 3> PICKAXE_DEFS = {{
    { "Copper pickaxe",  0,  1, ItemID::COPPER_PICKAXE },
    { "Iron pickaxe",    1,  5, ItemID::IRON_PICKAXE   },
    { "Steel pickaxe",   2, 20, ItemID::STEEL_PICKAXE  },
}};

// ---------------------------------------------------------------
// Fishing: fishing spots + raw fish + rod
// ---------------------------------------------------------------

enum class FishingSpotType : uint8_t {
    ShrimpPool  = 0,
    SardinePool = 1,
    TroutPool   = 2,
};

struct FishingSpotDef {
    FishingSpotType spotType;
    const char* name;
    int fishingLevel;        // Required fishing level
    int fishTicksMin;        // Min ticks per catch
    int fishTicksMax;        // Max ticks per catch
    std::array<uint16_t, 2> fishItem;  // Possible fish items (index 0 = common)
    uint32_t xpPerCatch;     // XP granted per catch
    int respawnTicks;        // Ticks before the spot can be fished again
};

// All fishing spot definitions (indexable by FishingSpotType)
inline constexpr std::array<FishingSpotDef, 3> FISHING_SPOT_DEFS = {{
    { FishingSpotType::ShrimpPool,  "Fishing spot (shrimp)",   1,  4,  8,  { ItemID::RAW_SHRIMP,  0             },  10, 30 },
    { FishingSpotType::SardinePool, "Fishing spot (sardine)",  5,  6, 12,  { ItemID::RAW_SARDINE, ItemID::RAW_SHRIMP },  20, 30 },
    { FishingSpotType::TroutPool,   "Fishing spot (trout)",   20,  8, 16,  { ItemID::RAW_TROUT,   ItemID::RAW_SARDINE },  35, 40 },
}};

// Food definitions (for healing)
struct FoodDef {
    const char* name;
    uint16_t itemId;
    uint8_t healAmount;    // HP restored when eaten
    uint16_t value;        // Gold value
};

inline constexpr std::array<FoodDef, 7> FOOD_DEFS = {{
    { "Shrimp",          ItemID::COOKED_SHRIMP,       3,   2   },
    { "Anchovies",       ItemID::COOKED_ANCHOVIES,    2,   3   },
    { "Sardine",         ItemID::COOKED_SARDINE,      4,   5   },
    { "Trout",           ItemID::COOKED_TROUT,        7,   10  },
    { "Salmon",          ItemID::COOKED_SALMON,       9,   15  },
    { "Lobster",         ItemID::COOKED_LOBSTER,     12,   30  },
    { "Swordfish",       ItemID::COOKED_SWORDFISH,   14,   50  },
}};

// Shop definitions (stock items for shop NPCs)
struct ShopItem {
    uint16_t itemId;
    int16_t quantity;      // -1 = infinite stock
    uint16_t buyPrice;    // Price player pays to buy
    uint16_t sellPrice;   // Price player gets when selling
};

// General store stock
inline constexpr std::array<ShopItem, 15> GENERAL_STORE_STOCK = {{
    { ItemID::BRONZE_SWORD,    -1, 30,   8   },
    { ItemID::BRONZE_SHIELD,   -1, 20,   5   },
    { ItemID::BRONZE_PLATEBODY, -1, 40,  12  },
    { ItemID::BRONZE_PLATELEGS, -1, 30,  9   },
    { ItemID::BRONZE_HELM,     -1, 15,   4   },
    { ItemID::BRONZE_GLOVES,   -1, 10,   3   },
    { ItemID::BRONZE_BOOTS,    -1, 12,   3   },
    { ItemID::IRON_SWORD,      -1, 100,  28  },
    { ItemID::IRON_SHIELD,     -1, 70,   20  },
    { ItemID::IRON_PLATEBODY,  -1, 150,  42  },
    { ItemID::IRON_PLATELEGS,  -1, 120,  35  },
    { ItemID::IRON_HELM,       -1, 80,   22  },
    { ItemID::IRON_GLOVES,     -1, 50,   14  },
    { ItemID::IRON_BOOTS,      -1, 55,   15  },
    { ItemID::COOKED_SHRIMP,   -1, 4,    1   },
}};

// Item definitions lookup (extendable)
inline constexpr ItemDef getItemDef(uint16_t id) {
    switch (id) {
        // Logs
        case ItemID::NORMAL_LOGS: return { id, "Normal logs",       false, 0xFF, 1    };
        case ItemID::OAK_LOGS:    return { id, "Oak logs",          false, 0xFF, 2    };
        case ItemID::WILLOW_LOGS: return { id, "Willow logs",      false, 0xFF, 4    };
        case ItemID::MAPLE_LOGS:  return { id, "Maple logs",        false, 0xFF, 8    };
        case ItemID::YEW_LOGS:    return { id, "Yew logs",          false, 0xFF, 16   };
        case ItemID::MAGIC_LOGS:  return { id, "Magic logs",         false, 0xFF, 32   };
        // Axes
        case ItemID::BRONZE_AXE:  return { id, "Bronze axe",       false, 3,    16   };
        case ItemID::IRON_AXE:    return { id, "Iron axe",         false, 3,    56   };
        case ItemID::STEEL_AXE:   return { id, "Steel axe",        false, 3,    200  };
        case ItemID::MITHRIL_AXE: return { id, "Mithril axe",      false, 3,    800  };
        case ItemID::ADAMANT_AXE: return { id, "Adamant axe",      false, 3,    1600 };
        case ItemID::RUNE_AXE:    return { id, "Rune axe",         false, 3,    6400 };
        case ItemID::DRAGON_AXE:  return { id, "Dragon axe",       false, 3,    40000};
        // Swords (Weapon slot)
        case ItemID::BRONZE_SWORD:return { id, "Bronze sword",     false, 3,    16   };
        case ItemID::IRON_SWORD:  return { id, "Iron sword",       false, 3,    56   };
        case ItemID::STEEL_SWORD: return { id, "Steel sword",      false, 3,    200  };
        // Shields (Shield slot)
        case ItemID::BRONZE_SHIELD:return{ id, "Bronze shield",    false, 5,    8    };
        case ItemID::IRON_SHIELD: return { id, "Iron shield",      false, 5,    28   };
        // Body armour (Body slot)
        case ItemID::BRONZE_PLATEBODY: return { id, "Bronze platebody", false, 4, 40 };
        case ItemID::IRON_PLATEBODY:   return { id, "Iron platebody",   false, 4, 150 };
        // Leg armour (Legs slot)
        case ItemID::BRONZE_PLATELEGS: return { id, "Bronze platelegs", false, 6, 30 };
        case ItemID::IRON_PLATELEGS:   return { id, "Iron platelegs",   false, 6, 120 };
        // Helmets (Head slot)
        case ItemID::BRONZE_HELM: return { id, "Bronze helm", false, 0, 15 };
        case ItemID::IRON_HELM:   return { id, "Iron helm",   false, 0, 80 };
        // Gloves (Gloves slot)
        case ItemID::BRONZE_GLOVES: return { id, "Bronze gloves", false, 7, 10 };
        case ItemID::IRON_GLOVES:   return { id, "Iron gloves",   false, 7, 50 };
        // Boots (Boots slot)
        case ItemID::BRONZE_BOOTS: return { id, "Bronze boots", false, 8, 12 };
        case ItemID::IRON_BOOTS:   return { id, "Iron boots",   false, 8, 55 };
        // Amulets (Amulet slot)
        case ItemID::AMULET_OF_STRENGTH: return { id, "Amulet of strength", false, 2, 200 };
        case ItemID::AMULET_OF_DEFENCE:   return { id, "Amulet of defence",   false, 2, 200 };
        // Raw food
        case ItemID::SHRIMP:   return { id, "Shrimp",   false, 0xFF, 2  };
        case ItemID::ANCHOVIES:return { id, "Anchovies",false, 0xFF, 3  };
        case ItemID::SARDINE:  return { id, "Sardine",  false, 0xFF, 5  };
        case ItemID::TROUT:    return { id, "Trout",    false, 0xFF, 10 };
        case ItemID::SALMON:   return { id, "Salmon",   false, 0xFF, 15 };
        case ItemID::LOBSTER:  return { id, "Lobster",  false, 0xFF, 30 };
        case ItemID::SWORDFISH:return { id, "Swordfish",false, 0xFF, 50 };
        // Cooked food
        case ItemID::COOKED_SHRIMP:     return { id, "Shrimp (cooked)",     false, 0xFF, 2   };
        case ItemID::COOKED_ANCHOVIES:  return { id, "Anchovies (cooked)",  false, 0xFF, 3   };
        case ItemID::COOKED_SARDINE:    return { id, "Sardine (cooked)",    false, 0xFF, 5   };
        case ItemID::COOKED_TROUT:      return { id, "Trout (cooked)",      false, 0xFF, 10  };
        case ItemID::COOKED_SALMON:     return { id, "Salmon (cooked)",     false, 0xFF, 15  };
        case ItemID::COOKED_LOBSTER:    return { id, "Lobster (cooked)",    false, 0xFF, 30  };
        case ItemID::COOKED_SWORDFISH:  return { id, "Swordfish (cooked)",  false, 0xFF, 50  };
        // Pickaxes
        case ItemID::COPPER_PICKAXE: return { id, "Copper pickaxe", false, 3, 16   };
        case ItemID::IRON_PICKAXE:   return { id, "Iron pickaxe",   false, 3, 56   };
        case ItemID::STEEL_PICKAXE:  return { id, "Steel pickaxe",  false, 3, 200  };
        // Ores
        case ItemID::COPPER_ORE:  return { id, "Copper ore",  false, 0xFF, 5   };
        case ItemID::IRON_ORE:    return { id, "Iron ore",    false, 0xFF, 15  };
        case ItemID::GOLD_ORE:    return { id, "Gold ore",    false, 0xFF, 50  };
        case ItemID::MITHRIL_ORE: return { id, "Mithril ore", false, 0xFF, 100 };
        // Raw fish
        case ItemID::RAW_SHRIMP:  return { id, "Raw shrimp",  false, 0xFF, 5   };
        case ItemID::RAW_SARDINE: return { id, "Raw sardine", false, 0xFF, 10  };
        case ItemID::RAW_TROUT:   return { id, "Raw trout",   false, 0xFF, 20  };
        // Fishing rod
        case ItemID::FISHING_ROD: return { id, "Fishing rod", false, 3, 30  };
        // Coins
        case ItemID::COINS:       return { id, "Coins",             true,  0xFF, 1    };
        default:                  return { id, "Unknown item",      false, 0xFF, 0    };
    }
}

// Get food heal amount for an item (returns 0 if not food)
inline uint8_t getFoodHeal(uint16_t itemId) {
    for (const auto& food : FOOD_DEFS) {
        if (food.itemId == itemId) return food.healAmount;
    }
    return 0;
}

// Check if an item is edible
inline bool isFood(uint16_t itemId) {
    return getFoodHeal(itemId) > 0;
}

} // namespace erynfall
