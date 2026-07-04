#pragma once

#include "types/position.h"
#include <cstdint>
#include <vector>

namespace erynfall {

enum class TileType : uint8_t {
    Grass     = 0,
    Water     = 1,   // Impassable
    Dirt      = 2,
    Sand      = 3,
    Stone     = 4,
    Lava      = 5,   // Impassable, damaging
};

inline bool tileIsPassable(TileType type) {
    return type != TileType::Water && type != TileType::Lava;
}

// The game world: a 2D tile grid.
class WorldMap {
public:
    WorldMap(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    // Tile access
    TileType tileAt(int x, int y) const;
    void setTile(int x, int y, TileType type);

    // Check if position is walkable (in bounds + passable tile)
    bool isWalkable(int x, int y) const;
    bool isWalkable(Position pos) const;

    // Fill the entire map with a tile type
    void fill(TileType type);

    // Total tile count
    int size() const { return width_ * height_; }

private:
    int width_, height_;
    std::vector<TileType> tiles_;
};

} // namespace erynfall
