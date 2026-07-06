#pragma once

#include "game_data/item_defs.h"
#include "types/position.h"
#include <cstdint>
#include <vector>

namespace erynfall {

// World object: a tree (or future: rock, fishing spot, etc.)
struct WorldObject {
    uint32_t id;              // Unique object ID
    TreeType treeType;        // Which tree
    Position position;        // Tile position
    int hp;                   // Remaining chops
    bool depleted;            // Is it cut down?
    uint64_t respawnTick;     // Tick to respawn (0 = alive)
    uint32_t choppingPlayer; // FD of player currently chopping (0 = none)
};

class WorldObjectManager {
public:
    WorldObjectManager();

    // Get all objects (for sync)
    const std::vector<WorldObject>& objects() const { return objects_; }

    // Get object at position (nullptr if none)
    const WorldObject* objectAt(int x, int y) const;

    // Get mutable object at position
    WorldObject* mutableObjectAt(int x, int y);

    // Get mutable object by ID
    WorldObject* mutableObjectById(uint32_t id);

    // Chop a tree: reduce HP, mark depleted if 0
    // Returns: true if successful chop, false if can't chop
    bool chop(uint32_t objectId, uint16_t playerFd);

    // Respawn check: called each tick
    void tick(uint64_t currentTick);

    // Place initial trees on the map
    void populateForest(const std::vector<std::pair<int,int>>& treePositions,
                        const std::vector<TreeType>& treeTypes);

    // Total object count
    size_t count() const { return objects_.size(); }

private:
    std::vector<WorldObject> objects_;
    uint32_t nextId_ = 1;
};

} // namespace erynfall
