#pragma once

#include "game_data/item_defs.h"
#include "types/position.h"
#include <cstdint>
#include <vector>

namespace erynfall {

// World object type discriminant
enum class ObjectType : uint8_t {
    Tree         = 0,
    Rock         = 1,
    FishingSpot  = 2,
};

// World object: a tree, rock, or fishing spot.
// objectType is the discriminant; subtype is interpreted per-type:
//   Tree        -> TreeType
//   Rock        -> RockType
//   FishingSpot -> FishingSpotType
struct WorldObject {
    uint32_t id;              // Unique object ID
    ObjectType objectType;    // Which kind of object
    uint8_t subtype;          // TreeType / RockType / FishingSpotType as raw byte
    Position position;        // Tile position
    int hp;                   // Remaining hits before depletion
    bool depleted;            // Is it depleted?
    uint64_t respawnTick;     // Tick to respawn (0 = alive)
    uint32_t activePlayer;    // FD of player currently interacting (0 = none)
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

    // Hit a tree: reduce HP, mark depleted if 0
    // Returns: true if successful chop, false if can't chop
    bool chop(uint32_t objectId, uint16_t playerFd);

    // Mine a rock: reduce HP, mark depleted if 0
    bool mine(uint32_t objectId, uint16_t playerFd);

    // Fish a spot: depletes the spot temporarily
    bool fish(uint32_t objectId, uint16_t playerFd);

    // Respawn check: called each tick
    void tick(uint64_t currentTick);

    // Place initial trees on the map
    void populateForest(const std::vector<std::pair<int,int>>& treePositions,
                        const std::vector<TreeType>& treeTypes);

    // Place rocks on the map
    void populateRocks(const std::vector<std::pair<int,int>>& rockPositions,
                       const std::vector<RockType>& rockTypes);

    // Place fishing spots on the map
    void populateFishingSpots(const std::vector<std::pair<int,int>>& spotPositions,
                              const std::vector<FishingSpotType>& spotTypes);

    // Total object count
    size_t count() const { return objects_.size(); }

private:
    std::vector<WorldObject> objects_;
    uint32_t nextId_ = 1;
};

} // namespace erynfall
