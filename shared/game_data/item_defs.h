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

// Item definitions lookup (extendable)
inline constexpr ItemDef getItemDef(uint16_t id) {
    switch (id) {
        case ItemID::NORMAL_LOGS: return { id, "Normal logs",       false, 0xFF, 1    };
        case ItemID::OAK_LOGS:    return { id, "Oak logs",          false, 0xFF, 2    };
        case ItemID::WILLOW_LOGS: return { id, "Willow logs",      false, 0xFF, 4    };
        case ItemID::MAPLE_LOGS:  return { id, "Maple logs",        false, 0xFF, 8    };
        case ItemID::YEW_LOGS:    return { id, "Yew logs",          false, 0xFF, 16   };
        case ItemID::MAGIC_LOGS:  return { id, "Magic logs",       false, 0xFF, 32   };
        case ItemID::BRONZE_AXE:  return { id, "Bronze axe",       false, 3,    16   };
        case ItemID::IRON_AXE:    return { id, "Iron axe",         false, 3,    56   };
        case ItemID::STEEL_AXE:   return { id, "Steel axe",        false, 3,    200  };
        case ItemID::MITHRIL_AXE: return { id, "Mithril axe",      false, 3,    800  };
        case ItemID::ADAMANT_AXE: return { id, "Adamant axe",      false, 3,    1600 };
        case ItemID::RUNE_AXE:    return { id, "Rune axe",         false, 3,    6400 };
        case ItemID::DRAGON_AXE:  return { id, "Dragon axe",       false, 3,    40000};
        case ItemID::COINS:       return { id, "Coins",             true,  0xFF, 1    };
        default:                  return { id, "Unknown item",      false, 0xFF, 0    };
    }
}

} // namespace erynfall
