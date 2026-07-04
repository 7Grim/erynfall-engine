#pragma once

#include <cstdint>
#include <cstdlib>

namespace erynfall {

struct Position {
    int16_t x = 0;
    int16_t y = 0;
    int8_t  z = 0;  // Height level

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    int tileDistanceTo(const Position& other) const {
        int dx = x - other.x;
        int dy = y - other.y;
        return std::abs(dx) + std::abs(dy);  // Manhattan distance
    }
};

} // namespace erynfall
