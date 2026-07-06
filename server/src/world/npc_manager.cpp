#include "world/npc_manager.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace erynfall {

NPCManager::NPCManager() : rng_(std::random_device{}()) {}

void NPCManager::initialize() {
    // Spawn goblins (6 in the southern area)
    createNpc(NPCType::GOBLIN, 5, 22);
    createNpc(NPCType::GOBLIN, 7, 24);
    createNpc(NPCType::GOBLIN, 10, 26);
    createNpc(NPCType::GOBLIN, 13, 23);
    createNpc(NPCType::GOBLIN, 8, 27);
    createNpc(NPCType::GOBLIN, 12, 28);
    
    // Spawn cows (4 in the eastern pasture)
    createNpc(NPCType::COW, 20, 15);
    createNpc(NPCType::COW, 23, 16);
    createNpc(NPCType::COW, 22, 18);
    createNpc(NPCType::COW, 25, 14);
    
    // Spawn chickens (3 near center)
    createNpc(NPCType::CHICKEN, 16, 10);
    createNpc(NPCType::CHICKEN, 18, 12);
    createNpc(NPCType::CHICKEN, 17, 9);
    
    std::cout << "[World] Spawned " << npcs_.size() << " NPCs" << std::endl;
}

NPC& NPCManager::createNpc(NPCType type, int spawnX, int spawnY) {
    NPC npc;
    npc.id = nextId_++;
    npc.type = type;
    npc.spawnPoint = {spawnX, spawnY, 0};
    npc.position = npc.spawnPoint;
    npc.hitpoints = getNPCDef(type).hitpoints;
    npc.alive = true;
    npc.attackingPlayerFd = 0;
    npc.ticksSinceLastAttack = 0;
    npc.wanderTimer = 5 + (rng_() % 10);
    npc.idleTimer = 0;
    npc.isWandering = false;
    npc.respawnTimer = 0;
    npcs_.push_back(npc);
    return npcs_.back();
}

const NPC* NPCManager::npcById(uint32_t id) const {
    for (auto& npc : npcs_) {
        if (npc.id == id) return &npc;
    }
    return nullptr;
}

NPC* NPCManager::mutableNpcById(uint32_t id) {
    for (auto& npc : npcs_) {
        if (npc.id == id) return &npc;
    }
    return nullptr;
}

const NPC* NPCManager::npcAt(int x, int y) const {
    for (auto& npc : npcs_) {
        if (npc.alive && npc.position.x == x && npc.position.y == y) {
            return &npc;
        }
    }
    return nullptr;
}

NPC* NPCManager::mutableNpcAt(int x, int y) {
    for (auto& npc : npcs_) {
        if (npc.alive && npc.position.x == x && npc.position.y == y) {
            return &npc;
        }
    }
    return nullptr;
}

void NPCManager::killNpc(uint32_t id) {
    for (auto& npc : npcs_) {
        if (npc.id == id) {
            npc.alive = false;
            npc.hitpoints = 0;
            npc.attackingPlayerFd = 0;
            npc.respawnTimer = npc.def().respawnTime;
            return;
        }
    }
}

void NPCManager::tickRespawns() {
    for (auto& npc : npcs_) {
        if (!npc.alive && npc.respawnTimer > 0) {
            npc.respawnTimer--;
            if (npc.respawnTimer == 0) {
                npc.alive = true;
                npc.hitpoints = npc.def().hitpoints;
                npc.position = npc.spawnPoint;
                npc.attackingPlayerFd = 0;
                npc.wanderTimer = 5 + (rng_() % 10);
            }
        }
    }
}

void NPCManager::tickWandering() {
    for (auto& npc : npcs_) {
        if (!npc.alive || npc.attackingPlayerFd != 0) continue;
        
        npc.idleTimer++;
        
        if (npc.isWandering) {
            // Move 1 step toward wander target
            int dx = npc.wanderTarget.x - npc.position.x;
            int dy = npc.wanderTarget.y - npc.position.y;
            int step_x = 0, step_y = 0;
            if (abs(dx) >= abs(dy)) {
                step_x = dx > 0 ? 1 : -1;
            } else {
                step_y = dy > 0 ? 1 : -1;
            }
            int nx = npc.position.x + step_x;
            int ny = npc.position.y + step_y;
            
            // Don't wander into water or out of bounds
            if (nx >= 0 && nx < 30 && ny >= 0 && ny < 30 && !(nx >= 10 && nx < 14 && ny >= 20 && ny < 25)) {
                npc.position = {nx, ny, 0};
            }
            
            if (npc.position == npc.wanderTarget || npc.idleTimer > 20) {
                npc.isWandering = false;
                npc.idleTimer = 0;
                npc.wanderTimer = 5 + (rng_() % 15);
            }
        } else {
            // Check if time to wander
            npc.wanderTimer--;
            if (npc.wanderTimer <= 0) {
                // Pick random nearby tile
                int radius = npc.def().wanderRadius;
                int wx = npc.spawnPoint.x + (rng_() % (radius * 2 + 1)) - radius;
                int wy = npc.spawnPoint.y + (rng_() % (radius * 2 + 1)) - radius;
                wx = std::max(0, std::min(29, wx));
                wy = std::max(0, std::min(29, wy));
                npc.wanderTarget = {wx, wy, 0};
                npc.isWandering = true;
                npc.idleTimer = 0;
            }
        }
    }
}

int NPCManager::aliveCount() const {
    int count = 0;
    for (auto& npc : npcs_) {
        if (npc.alive) count++;
    }
    return count;
}

} // namespace erynfall
