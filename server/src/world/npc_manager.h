#pragma once

#include "types/position.h"
#include "game_data/npc_defs.h"
#include <vector>
#include <cstdint>
#include <random>

namespace erynfall {

// Runtime NPC instance
struct NPC {
    uint32_t id;              // Unique instance ID
    NPCType type;             // NPCDef type
    Position spawnPoint;      // Where it spawns
    Position position;        // Current position
    uint16_t hitpoints;       // Current HP
    bool alive;               // Is it alive?
    
    // Combat state
    int attackingPlayerFd;    // FD of player being attacked (0 = none)
    int ticksSinceLastAttack; // Tick counter for attack timing
    
    // Wander state
    int wanderTimer;          // Ticks until next wander
    int idleTimer;            // Ticks idle before next wander
    bool isWandering;
    Position wanderTarget;
    
    // Respawn state
    uint32_t respawnTimer;    // Ticks until respawn (counts down when dead)
    
    const NPCDef& def() const { return getNPCDef(type); }
};

// Manages all NPCs in the world
class NPCManager {
public:
    NPCManager();
    
    // Spawn initial NPCs — returns list of spawn positions
    void initialize();
    
    // Get NPC by instance ID
    const NPC* npcById(uint32_t id) const;
    NPC* mutableNpcById(uint32_t id);
    
    // Get NPC at a tile position
    const NPC* npcAt(int x, int y) const;
    NPC* mutableNpcAt(int x, int y);
    
    // Get all NPCs (for ticking/syncing)
    const std::vector<NPC>& all() const { return npcs_; }
    std::vector<NPC>& mutableAll() { return npcs_; }
    
    // Kill an NPC — sets alive=false, starts respawn timer, returns loot
    void killNpc(uint32_t id);
    
    // Respawn NPCs whose timers expired
    void tickRespawns();
    
    // Wander NPCs
    void tickWandering();
    
    // Count alive NPCs
    int aliveCount() const;

private:
    std::vector<NPC> npcs_;
    uint32_t nextId_ = 1;
    std::mt19937 rng_;
    
    NPC& createNpc(NPCType type, int spawnX, int spawnY);
};

} // namespace erynfall
