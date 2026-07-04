#include "world/world_map.h"

namespace erynfall {

WorldMap::WorldMap(int width, int height)
    : width_(width), height_(height), tiles_(width * height, TileType::Grass) {}

TileType WorldMap::tileAt(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return TileType::Water;  // Out of bounds = impassable
    }
    return tiles_[y * width_ + x];
}

void WorldMap::setTile(int x, int y, TileType type) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    tiles_[y * width_ + x] = type;
}

bool WorldMap::isWalkable(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
    return tileIsPassable(tiles_[y * width_ + x]);
}

bool WorldMap::isWalkable(Position pos) const {
    return isWalkable(pos.x, pos.y);
}

void WorldMap::fill(TileType type) {
    std::fill(tiles_.begin(), tiles_.end(), type);
}

} // namespace erynfall
