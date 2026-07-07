#include "world/world_objects.h"
#include <algorithm>

namespace erynfall {

WorldObjectManager::WorldObjectManager() = default;

const WorldObject* WorldObjectManager::objectAt(int x, int y) const {
    for (const auto& obj : objects_) {
        if (obj.position.x == x && obj.position.y == y && !obj.depleted) {
            return &obj;
        }
    }
    return nullptr;
}

WorldObject* WorldObjectManager::mutableObjectAt(int x, int y) {
    for (auto& obj : objects_) {
        if (obj.position.x == x && obj.position.y == y) {
            return &obj;
        }
    }
    return nullptr;
}

WorldObject* WorldObjectManager::mutableObjectById(uint32_t id) {
    for (auto& obj : objects_) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

bool WorldObjectManager::chop(uint32_t objectId, uint16_t playerFd) {
    for (auto& obj : objects_) {
        if (obj.id != objectId) continue;
        if (obj.objectType != ObjectType::Tree) return false;
        if (obj.depleted) return false;
        if (obj.activePlayer != 0 && obj.activePlayer != playerFd) return false;

        obj.hp--;
        if (obj.hp <= 0) {
            obj.depleted = true;
            obj.activePlayer = 0;
        } else {
            obj.activePlayer = playerFd;
        }
        return true;
    }
    return false;
}

bool WorldObjectManager::mine(uint32_t objectId, uint16_t playerFd) {
    for (auto& obj : objects_) {
        if (obj.id != objectId) continue;
        if (obj.objectType != ObjectType::Rock) return false;
        if (obj.depleted) return false;
        if (obj.activePlayer != 0 && obj.activePlayer != playerFd) return false;

        obj.hp--;
        if (obj.hp <= 0) {
            obj.depleted = true;
            obj.activePlayer = 0;
        } else {
            obj.activePlayer = playerFd;
        }
        return true;
    }
    return false;
}

bool WorldObjectManager::fish(uint32_t objectId, uint16_t playerFd) {
    for (auto& obj : objects_) {
        if (obj.id != objectId) continue;
        if (obj.objectType != ObjectType::FishingSpot) return false;
        if (obj.depleted) return false;
        if (obj.activePlayer != 0 && obj.activePlayer != playerFd) return false;

        // Fishing spots deplete after every successful catch (hp treated as 1).
        obj.hp--;
        if (obj.hp <= 0) {
            obj.depleted = true;
            obj.activePlayer = 0;
        } else {
            obj.activePlayer = playerFd;
        }
        return true;
    }
    return false;
}

void WorldObjectManager::tick(uint64_t currentTick) {
    for (auto& obj : objects_) {
        if (!obj.depleted || obj.respawnTick == 0) continue;
        if (currentTick < obj.respawnTick) continue;

        switch (obj.objectType) {
            case ObjectType::Tree: {
                auto& def = TREE_DEFS[obj.subtype];
                obj.hp = def.hp;
                break;
            }
            case ObjectType::Rock: {
                auto& def = ROCK_DEFS[obj.subtype];
                obj.hp = def.hp;
                break;
            }
            case ObjectType::FishingSpot: {
                auto& def = FISHING_SPOT_DEFS[obj.subtype];
                obj.hp = 1;  // Fishing spots deplete per catch
                (void)def;
                break;
            }
        }
        obj.depleted = false;
        obj.activePlayer = 0;
    }
}

void WorldObjectManager::populateForest(
    const std::vector<std::pair<int,int>>& positions,
    const std::vector<TreeType>& types)
{
    for (size_t i = 0; i < positions.size(); ++i) {
        TreeType type = (i < types.size()) ? types[i] : TreeType::Normal;
        auto& def = TREE_DEFS[static_cast<int>(type)];
        WorldObject obj;
        obj.id = nextId_++;
        obj.objectType = ObjectType::Tree;
        obj.subtype = static_cast<uint8_t>(type);
        obj.position = {static_cast<int16_t>(positions[i].first),
                        static_cast<int16_t>(positions[i].second), 0};
        obj.hp = def.hp;
        obj.depleted = false;
        obj.respawnTick = 0;
        obj.activePlayer = 0;
        objects_.push_back(obj);
    }
}

void WorldObjectManager::populateRocks(
    const std::vector<std::pair<int,int>>& positions,
    const std::vector<RockType>& types)
{
    for (size_t i = 0; i < positions.size(); ++i) {
        RockType type = (i < types.size()) ? types[i] : RockType::Copper;
        auto& def = ROCK_DEFS[static_cast<int>(type)];
        WorldObject obj;
        obj.id = nextId_++;
        obj.objectType = ObjectType::Rock;
        obj.subtype = static_cast<uint8_t>(type);
        obj.position = {static_cast<int16_t>(positions[i].first),
                        static_cast<int16_t>(positions[i].second), 0};
        obj.hp = def.hp;
        obj.depleted = false;
        obj.respawnTick = 0;
        obj.activePlayer = 0;
        objects_.push_back(obj);
    }
}

void WorldObjectManager::populateFishingSpots(
    const std::vector<std::pair<int,int>>& positions,
    const std::vector<FishingSpotType>& types)
{
    for (size_t i = 0; i < positions.size(); ++i) {
        FishingSpotType type = (i < types.size()) ? types[i] : FishingSpotType::ShrimpPool;
        auto& def = FISHING_SPOT_DEFS[static_cast<int>(type)];
        WorldObject obj;
        obj.id = nextId_++;
        obj.objectType = ObjectType::FishingSpot;
        obj.subtype = static_cast<uint8_t>(type);
        obj.position = {static_cast<int16_t>(positions[i].first),
                        static_cast<int16_t>(positions[i].second), 0};
        obj.hp = 1;  // Each spot depletes per catch
        obj.depleted = false;
        obj.respawnTick = 0;
        obj.activePlayer = 0;
        objects_.push_back(obj);
        (void)def;
    }
}

} // namespace erynfall
