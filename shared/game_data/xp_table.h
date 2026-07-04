#pragma once

#include "types/skill.h"
#include <cmath>
#include <array>
#include <cstdint>

namespace erynfall {

// OSRS XP table: xpForLevel[i] = total XP needed to reach level i
// Level 1 = 0 XP, Level 2 = 83 XP, etc.
// This is the exact OSRS formula.
inline constexpr uint32_t xpForLevel(uint8_t level) {
    if (level <= 1) return 0;

    double total = 0;
    for (int i = 1; i < level; ++i) {
        total += std::floor(static_cast<double>(i) + 300.0 * std::pow(2.0, static_cast<double>(i) / 7.0));
    }
    return static_cast<uint32_t>(std::floor(total / 4.0));
}

inline uint8_t levelFromXP(uint32_t xp) {
    for (uint8_t level = 99; level >= 2; --level) {
        if (xp >= xpForLevel(level)) {
            return level;
        }
    }
    return 1;
}

} // namespace erynfall
