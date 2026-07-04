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

bool WorldObjectManager::chop(uint32_t objectId, uint16_t playerFd) {
    for (auto& obj : objects_) {
        if (obj.id != objectId) continue;
        if (obj.depleted) return false;
        if (obj.choppingPlayer != 0 && obj.choppingPlayer != playerFd) return false;

        obj.hp--;
        if (obj.hp <= 0) {
            obj.depleted = true;
            obj.choppingPlayer = 0;
        } else {
            obj.choppingPlayer = playerFd;
        }
        return true;
    }
    return false;
}

void WorldObjectManager::tick(uint64_t currentTick) {
    for (auto& obj : objects_) {
        if (obj.depleted && obj.respawnTick > 0 && currentTick >= obj.respawnTick) {
            // Respawn
            auto& def = TREE_DEFS[static_cast<int>(obj.treeType)];
            obj.hp = def.hp;
            obj.depleted = false;
            obj.choppingPlayer = 0;
        }
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
        obj.treeType = type;
        obj.position = {static_cast<int16_t>(positions[i].first),
                        static_cast<int16_t>(positions[i].second), 0};
        obj.hp = def.hp;
        obj.depleted = false;
        obj.respawnTick = 0;
        obj.choppingPlayer = 0;
        objects_.push_back(obj);
    }
}

} // namespace erynfall
