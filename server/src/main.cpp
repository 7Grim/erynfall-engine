#include "core/game_loop.h"
#include "net/tcp_server.h"
#include "world/world_map.h"
#include "world/world_objects.h"
#include "world/npc_manager.h"
#include "world/ground_items.h"
#include "db/database.h"
#include "game/inventory.h"
#include "game/skill_set.h"
#include "game/equipment.h"
#include "protocol/opcodes.h"
#include "protocol/buffer.h"

#include "constants/game_constants.h"
#include "game_data/item_defs.h"
#include "game_data/xp_table.h"
#include "game_data/npc_defs.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cmath>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>

using namespace erynfall;

// Connected player state
struct ConnectedPlayer {
    int fd = 0;
    Position position{0, 0, 0};
    Position target{0, 0, 0};
    bool walking = false;
    uint64_t last_move_tick = 0;

    // Phase 2: game state
    Inventory inventory;
    SkillSet skills;

    // Woodcutting action state
    bool chopping = false;
    uint32_t choppingObjectId = 0;
    int chopsRemaining = 0;
    int chopTimer = 0;

    // Mining action state
    bool mining = false;
    uint32_t miningObjectId = 0;
    int mineTimer = 0;

    // Fishing action state
    bool fishing = false;
    uint32_t fishingObjectId = 0;
    int fishTimer = 0;

    // Phase 3: Combat state
    bool inCombat = false;
    uint32_t attackingNpcId = 0;       // Which NPC we're attacking
    int combatAttackTimer = 0;         // Ticks until our next attack
    bool dead = false;
    uint32_t deathTimer = 0;           // Ticks until auto-respawn (5s = 8 ticks)

    // Phase 4: Equipment, gold, HP, shop state
    Equipment equipment;
    uint32_t gold = 0;                 // Coins in inventory (stackable)
    uint8_t currentHP = 10;             // Current hitpoints (max from skill level)
    uint8_t maxHP = 10;                // Cached max HP from skill level
    bool shopOpen = false;              // Is the player viewing a shop?

    // Give starting gear on join
    void init() {
        inventory.addItem(ItemID::BRONZE_AXE, 1);
        inventory.addItem(ItemID::BRONZE_SWORD, 1);
        inventory.addItem(ItemID::BRONZE_SHIELD, 1);
        inventory.addItem(ItemID::COPPER_PICKAXE, 1);
        inventory.addItem(ItemID::FISHING_ROD, 1);
        gold = 50;  // Starting gold to buy food/gear
        maxHP = skills.level(Skill::Hitpoints);
        currentHP = maxHP;
    }

    // Recalculate max HP from skill level
    void recalcMaxHP() {
        maxHP = skills.level(Skill::Hitpoints);
        if (currentHP > maxHP) currentHP = maxHP;
    }
};

// Forward declarations for send helpers
static bool sendPacket(int fd, ServerOpcode opcode, const uint8_t* payload, uint16_t len);
static bool sendSystemMessage(int fd, const std::string& msg);

// Send inventory sync (28 slots × 4 bytes = 112 bytes)
static bool sendInventorySync(int fd, const Inventory& inv) {
    uint8_t buf[112];
    inv.serialize(buf);
    return sendPacket(fd, ServerOpcode::InventorySync, buf, 112);
}

// Send skill update for one skill (1 byte skill_id + 1 byte level + 4 bytes XP = 6 bytes)
static bool sendSkillUpdate(int fd, Skill skill, uint8_t level, uint32_t xp) {
    uint8_t buf[6];
    buf[0] = static_cast<uint8_t>(skill);
    buf[1] = level;
    buf[2] = static_cast<uint8_t>(xp & 0xFF);
    buf[3] = static_cast<uint8_t>((xp >> 8) & 0xFF);
    buf[4] = static_cast<uint8_t>((xp >> 16) & 0xFF);
    buf[5] = static_cast<uint8_t>((xp >> 24) & 0xFF);
    return sendPacket(fd, ServerOpcode::SkillUpdate, buf, 6);
}

// Send all skills sync (23 × 5 bytes = 115 bytes)
static bool sendSkillsSync(int fd, const SkillSet& skills) {
    uint8_t buf[115];
    skills.serialize(buf);
    return sendPacket(fd, ServerOpcode::SkillUpdate, buf, 115);
}

// Send equipment sync (10 slots × 4 bytes = 40 bytes)
static bool sendEquipmentSync(int fd, const Equipment& equip) {
    uint8_t buf[40];
    equip.serialize(buf);
    return sendPacket(fd, ServerOpcode::EquipmentSync, buf, 40);
}

// Send gold update (4 bytes uint32)
static bool sendGoldUpdate(int fd, uint32_t gold) {
    uint8_t buf[4];
    buf[0] = static_cast<uint8_t>(gold & 0xFF);
    buf[1] = static_cast<uint8_t>((gold >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>((gold >> 16) & 0xFF);
    buf[3] = static_cast<uint8_t>((gold >> 24) & 0xFF);
    return sendPacket(fd, ServerOpcode::GoldUpdate, buf, 4);
}

// Send health update (2 bytes: currentHP + maxHP)
static bool sendHealthUpdate(int fd, uint8_t currentHP, uint8_t maxHP) {
    uint8_t buf[2] = { currentHP, maxHP };
    return sendPacket(fd, ServerOpcode::HealthUpdate, buf, 2);
}

// Send shop stock to player
// Format: uint8 count + per item: uint16 itemId + uint16 buyPrice + uint16 sellPrice + uint8 qty
static bool sendShopOpen(int fd) {
    uint8_t buf[256];
    int off = 0;
    buf[off++] = static_cast<uint8_t>(GENERAL_STORE_STOCK.size());
    for (const auto& item : GENERAL_STORE_STOCK) {
        buf[off++] = static_cast<uint8_t>(item.itemId & 0xFF);
        buf[off++] = static_cast<uint8_t>((item.itemId >> 8) & 0xFF);
        buf[off++] = static_cast<uint8_t>(item.buyPrice & 0xFF);
        buf[off++] = static_cast<uint8_t>((item.buyPrice >> 8) & 0xFF);
        buf[off++] = static_cast<uint8_t>(item.sellPrice & 0xFF);
        buf[off++] = static_cast<uint8_t>((item.sellPrice >> 8) & 0xFF);
        buf[off++] = static_cast<uint8_t>(item.quantity & 0xFF); // -1 for infinite shown as 0xFF
    }
    return sendPacket(fd, ServerOpcode::ShopOpen, buf, off);
}

// Send animation instruction (1 byte anim_id + 2 bytes target_x + 2 bytes target_y = 5 bytes)
static bool sendAnimation(int fd, uint8_t animId, int targetX, int targetY) {
    uint8_t buf[5];
    buf[0] = animId;
    buf[1] = static_cast<uint8_t>(targetX & 0xFF);
    buf[2] = static_cast<uint8_t>((targetX >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(targetY & 0xFF);
    buf[4] = static_cast<uint8_t>((targetY >> 8) & 0xFF);
    return sendPacket(fd, ServerOpcode::Animation, buf, 5);
}

// Send NPC update to all players
// Format: uint8 type, uint32 npcId, uint16 x, uint16 y, uint16 hp, uint16 maxHp, uint8 alive = 12 bytes
static bool sendNpcUpdate(int fd, const NPC& npc) {
    uint8_t buf[12];
    buf[0] = static_cast<uint8_t>(npc.type);
    buf[1] = static_cast<uint8_t>(npc.id & 0xFF);
    buf[2] = static_cast<uint8_t>((npc.id >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>((npc.id >> 16) & 0xFF);
    buf[4] = static_cast<uint8_t>((npc.id >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>(npc.position.x & 0xFF);
    buf[6] = static_cast<uint8_t>((npc.position.x >> 8) & 0xFF);
    buf[7] = static_cast<uint8_t>(npc.position.y & 0xFF);
    buf[8] = static_cast<uint8_t>((npc.position.y >> 8) & 0xFF);
    buf[9] = static_cast<uint8_t>(npc.hitpoints & 0xFF);
    buf[10] = static_cast<uint8_t>((npc.hitpoints >> 8) & 0xFF);
    buf[11] = static_cast<uint8_t>(npc.def().hitpoints & 0xFF);
    // Send max HP as a separate byte for simplicity (only need uint8 for now)
    uint8_t buf2[13];
    std::memcpy(buf2, buf, 12);
    buf2[12] = static_cast<uint8_t>(npc.alive ? 1 : 0);
    return sendPacket(fd, ServerOpcode::NPCUpdate, buf2, 13);
}

// Broadcast NPC update to all players
static void broadcastNpcUpdate(const std::unordered_map<int, ConnectedPlayer>& players, const NPC& npc) {
    for (auto& [fd, player] : players) {
        sendNpcUpdate(fd, npc);
    }
}

// Send ground item add
// Format: uint32 itemId, uint16 qty, uint8 x, uint8 y = 8 bytes
static bool sendGroundItemAdd(int fd, uint16_t itemId, uint16_t qty, int x, int y) {
    uint8_t buf[5];
    buf[0] = static_cast<uint8_t>(itemId & 0xFF);
    buf[1] = static_cast<uint8_t>((itemId >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>(qty & 0xFF);
    buf[3] = static_cast<uint8_t>((qty >> 8) & 0xFF);
    buf[4] = static_cast<uint8_t>((x + y) & 0xFF); // placeholder — x,y packed
    // Simplified: just send the item info
    uint8_t buf2[8];
    buf2[0] = static_cast<uint8_t>(itemId & 0xFF);
    buf2[1] = static_cast<uint8_t>((itemId >> 8) & 0xFF);
    buf2[2] = static_cast<uint8_t>(qty & 0xFF);
    buf2[3] = static_cast<uint8_t>((qty >> 8) & 0xFF);
    buf2[4] = static_cast<uint8_t>(x);
    buf2[5] = static_cast<uint8_t>(y);
    buf2[6] = 0; // placeholder
    buf2[7] = 0;
    return sendPacket(fd, ServerOpcode::GroundItemAdd, buf2, 8);
}

// Send ground item remove
static bool sendGroundItemRemove(int fd, int x, int y) {
    uint8_t buf[2] = {static_cast<uint8_t>(x), static_cast<uint8_t>(y)};
    return sendPacket(fd, ServerOpcode::GroundItemRemove, buf, 2);
}

// Broadcast to all players
static void broadcastPacket(const std::unordered_map<int, ConnectedPlayer>& players,
                            ServerOpcode opcode, const uint8_t* payload, uint16_t len) {
    for (auto& [fd, player] : players) {
        sendPacket(fd, opcode, payload, len);
    }
}

static std::atomic<bool> g_shutdown{false};

static void signalHandler(int /*signum*/) {
    std::cout << "\n[Server] Shutting down..." << std::endl;
    g_shutdown.store(true);
}

static std::vector<uint8_t> readAvailable(int fd) {
    std::vector<uint8_t> buf(4096);
    ssize_t n = ::recv(fd, buf.data(), buf.size(), 0);
    if (n <= 0) return {};
    buf.resize(n);
    return buf;
}

static bool sendBytes(int fd, const std::vector<uint8_t>& data) {
    ssize_t n = ::send(fd, data.data(), data.size(), 0);
    return n == static_cast<ssize_t>(data.size());
}

static bool sendPacket(int fd, ServerOpcode opcode, const uint8_t* payload = nullptr, uint16_t len = 0) {
    uint16_t total = 1 + len;
    std::vector<uint8_t> packet(1 + 1 + total);
    packet[0] = static_cast<uint8_t>(total);
    packet[1] = static_cast<uint8_t>(opcode);
    if (payload && len > 0) {
        std::memcpy(&packet[2], payload, len);
    }
    return sendBytes(fd, packet);
}

static bool sendSystemMessage(int fd, const std::string& msg) {
    ByteBuffer buf;
    buf.writeString(msg.c_str());
    return sendPacket(fd, ServerOpcode::SystemMessage, buf.data(),
                     static_cast<uint16_t>(buf.size()));
}

static bool sendPositionUpdate(int fd, const Position& pos) {
    uint8_t payload[2];
    payload[0] = static_cast<uint8_t>(pos.x);
    payload[1] = static_cast<uint8_t>(pos.y);
    return sendPacket(fd, ServerOpcode::PlayerUpdate, payload, 2);
}

// Send world object update (add/remove tree)
static bool sendWorldObjectUpdate(int fd, uint8_t action, const WorldObject& obj) {
    uint8_t buf[9];
    buf[0] = action;
    buf[1] = static_cast<uint8_t>(obj.id & 0xFF);
    buf[2] = static_cast<uint8_t>((obj.id >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>((obj.id >> 16) & 0xFF);
    buf[4] = static_cast<uint8_t>((obj.id >> 24) & 0xFF);
    // Pack objectType (high nibble) + subtype (low nibble) for the client
    buf[5] = static_cast<uint8_t>((static_cast<uint8_t>(obj.objectType) << 4) | (obj.subtype & 0x0F));
    buf[6] = static_cast<uint8_t>(obj.position.x);
    buf[7] = static_cast<uint8_t>(obj.position.y);
    buf[8] = 0;
    return sendPacket(fd, ServerOpcode::GroundItemAdd, buf, 9);
}

// Check if player has an axe (returns best bonus)
static int getAxeBonus(const Inventory& inv) {
    for (int i = 0; i < inv.size(); ++i) {
        auto& slot = inv.slot(i);
        if (slot.quantity == 0) continue;
        for (const auto& axe : AXE_DEFS) {
            if (slot.id == axe.itemId) return axe.woodcuttingBonus;
        }
    }
    return 0;
}

// Check if player has a pickaxe (returns best bonus)
static int getPickaxeBonus(const Inventory& inv) {
    for (int i = 0; i < inv.size(); ++i) {
        auto& slot = inv.slot(i);
        if (slot.quantity == 0) continue;
        for (const auto& pick : PICKAXE_DEFS) {
            if (slot.id == pick.itemId) return pick.miningBonus;
        }
    }
    return -1;  // No pickaxe
}

// Calculate player max hit (Phase 4: uses equipment bonuses)
static int calcPlayerMaxHit(const ConnectedPlayer& player) {
    uint8_t strLevel = player.skills.level(Skill::Strength);
    int weaponBonus = player.equipment.getStrengthBonus();
    // OSRS-style max hit formula: floor((strLevel + weaponBonus) * 0.85) + 1
    return std::max(1, static_cast<int>((strLevel + weaponBonus) * 0.85) + 1);
}

// Calculate player defence (Phase 4: uses equipment bonuses)
static int calcPlayerDefence(const ConnectedPlayer& player) {
    uint8_t defLevel = player.skills.level(Skill::Defence);
    return defLevel + player.equipment.getDefenceBonus();
}

// Calculate player attack speed from equipped weapon
static int calcPlayerAttackSpeed(const ConnectedPlayer& player) {
    return player.equipment.getAttackSpeed();
}

// Resolve woodcutting action on tick
static void resolveWoodcutting(ConnectedPlayer& player, WorldObjectManager& objects,
                               uint64_t tick) {
    if (!player.chopping || player.dead) return;

    WorldObject* obj = objects.mutableObjectById(player.choppingObjectId);
    if (!obj || obj->objectType != ObjectType::Tree || obj->depleted ||
        obj->id != player.choppingObjectId) {
        player.chopping = false;
        sendSystemMessage(player.fd, "Tree is gone.");
        return;
    }

    if (player.position.tileDistanceTo(obj->position) > 1) {
        player.chopping = false;
        sendSystemMessage(player.fd, "You moved too far.");
        return;
    }

    player.chopTimer--;
    if (player.chopTimer > 0) return;

    const auto& treeDef = TREE_DEFS[obj->subtype];

    bool leveled = player.skills.addXP(Skill::Woodcutting, treeDef.xpPerChop);
    auto wcState = player.skills.get(Skill::Woodcutting);
    sendSkillUpdate(player.fd, Skill::Woodcutting, wcState.level, wcState.xp);

    if (leveled) {
        sendSystemMessage(player.fd, "Congratulations! Your Woodcutting level is now " +
                          std::to_string(wcState.level) + "!");
    }

    bool added = player.inventory.addItem(treeDef.logItem, 1);
    sendInventorySync(player.fd, player.inventory);
    if (!added) {
        sendSystemMessage(player.fd, "Inventory full!");
    }

    bool depleted = objects.chop(obj->id, player.fd);

    if (depleted) {
        sendSystemMessage(player.fd, std::string("The ") + treeDef.name + " has been cut down.");
        player.chopping = false;
        return;
    }

    auto& def = TREE_DEFS[obj->subtype];
    int baseTicks = def.chopTicksMin + (std::rand() % (def.chopTicksMax - def.chopTicksMin + 1));
    int bonus = getAxeBonus(player.inventory);
    player.chopTimer = std::max(1, baseTicks - bonus * 2);
}

// Resolve mining action on tick
static void resolveMining(ConnectedPlayer& player, WorldObjectManager& objects,
                          uint64_t tick) {
    if (!player.mining || player.dead) return;

    WorldObject* obj = objects.mutableObjectById(player.miningObjectId);
    if (!obj || obj->objectType != ObjectType::Rock || obj->depleted ||
        obj->id != player.miningObjectId) {
        player.mining = false;
        sendSystemMessage(player.fd, "The rock is gone.");
        return;
    }

    if (player.position.tileDistanceTo(obj->position) > 1) {
        player.mining = false;
        sendSystemMessage(player.fd, "You moved too far.");
        return;
    }

    player.mineTimer--;
    if (player.mineTimer > 0) return;

    const auto& rockDef = ROCK_DEFS[obj->subtype];

    bool leveled = player.skills.addXP(Skill::Mining, rockDef.xpPerMine);
    auto mineState = player.skills.get(Skill::Mining);
    sendSkillUpdate(player.fd, Skill::Mining, mineState.level, mineState.xp);

    if (leveled) {
        sendSystemMessage(player.fd, "Congratulations! Your Mining level is now " +
                          std::to_string(mineState.level) + "!");
    }

    bool added = player.inventory.addItem(rockDef.oreItem, 1);
    sendInventorySync(player.fd, player.inventory);
    if (added) {
        sendSystemMessage(player.fd, std::string("You mine some ") +
                          getItemDef(rockDef.oreItem).name + ".");
    } else {
        sendSystemMessage(player.fd, "Inventory full!");
    }

    bool depleted = objects.mine(obj->id, player.fd);

    if (depleted) {
        // Schedule respawn
        obj->respawnTick = tick + static_cast<uint64_t>(rockDef.respawnTicks);
        sendSystemMessage(player.fd, std::string("The ") + rockDef.name + " has been depleted.");
        player.mining = false;
        return;
    }

    int baseTicks = rockDef.mineTicksMin +
        (std::rand() % (rockDef.mineTicksMax - rockDef.mineTicksMin + 1));
    int bonus = getPickaxeBonus(player.inventory);
    player.mineTimer = std::max(1, baseTicks - bonus * 2);
}

// Resolve fishing action on tick
static void resolveFishing(ConnectedPlayer& player, WorldObjectManager& objects,
                           uint64_t tick) {
    if (!player.fishing || player.dead) return;

    WorldObject* obj = objects.mutableObjectById(player.fishingObjectId);
    if (!obj || obj->objectType != ObjectType::FishingSpot || obj->depleted ||
        obj->id != player.fishingObjectId) {
        player.fishing = false;
        sendSystemMessage(player.fd, "The fishing spot is gone.");
        return;
    }

    if (player.position.tileDistanceTo(obj->position) > 1) {
        player.fishing = false;
        sendSystemMessage(player.fd, "You moved too far.");
        return;
    }

    player.fishTimer--;
    if (player.fishTimer > 0) return;

    const auto& spotDef = FISHING_SPOT_DEFS[obj->subtype];

    // Determine which fish we catch (80% common, 20% uncommon if defined)
    uint16_t fishItem = spotDef.fishItem[0];
    if (spotDef.fishItem[1] != 0 && (std::rand() % 100) < 20) {
        fishItem = spotDef.fishItem[1];
    }

    bool leveled = player.skills.addXP(Skill::Fishing, spotDef.xpPerCatch);
    auto fishState = player.skills.get(Skill::Fishing);
    sendSkillUpdate(player.fd, Skill::Fishing, fishState.level, fishState.xp);

    if (leveled) {
        sendSystemMessage(player.fd, "Congratulations! Your Fishing level is now " +
                          std::to_string(fishState.level) + "!");
    }

    bool added = player.inventory.addItem(fishItem, 1);
    sendInventorySync(player.fd, player.inventory);
    if (added) {
        sendSystemMessage(player.fd, std::string("You catch a ") +
                          getItemDef(fishItem).name + ".");
    } else {
        sendSystemMessage(player.fd, "Inventory full!");
    }

    // Spot depletes after each catch (hp is 1)
    bool depleted = objects.fish(obj->id, player.fd);

    if (depleted) {
        obj->respawnTick = tick + static_cast<uint64_t>(spotDef.respawnTicks);
        sendSystemMessage(player.fd, "The fishing spot moves away.");
        player.fishing = false;
        return;
    }

    int baseTicks = spotDef.fishTicksMin +
        (std::rand() % (spotDef.fishTicksMax - spotDef.fishTicksMin + 1));
    player.fishTimer = std::max(1, baseTicks);
}

// Resolve combat on tick — player attacks NPC
static void resolvePlayerCombat(ConnectedPlayer& player, NPCManager& npcs,
                                  GroundItemManager& groundItems,
                                  const std::unordered_map<int, ConnectedPlayer>& players) {
    if (!player.inCombat || player.dead) return;

    NPC* target = npcs.mutableNpcById(player.attackingNpcId);
    if (!target || !target->alive) {
        player.inCombat = false;
        player.attackingNpcId = 0;
        sendSystemMessage(player.fd, "Your target is gone.");
        return;
    }

    // Check distance
    if (player.position.tileDistanceTo(target->position) > 1) {
        // Cancel combat if too far
        player.inCombat = false;
        player.attackingNpcId = 0;
        target->attackingPlayerFd = 0;
        sendSystemMessage(player.fd, "You are too far away to attack.");
        return;
    }

    player.combatAttackTimer--;
    if (player.combatAttackTimer > 0) return;

    // Player attacks NPC
    int maxHit = calcPlayerMaxHit(player);
    int damage = 1 + (std::rand() % maxHit);  // Min 1 damage

    target->hitpoints = static_cast<uint16_t>(
        std::max(0, static_cast<int>(target->hitpoints) - damage));

    // Grant attack XP
    player.skills.addXP(Skill::Attack, static_cast<uint16_t>(target->def().attackXP));
    sendSkillUpdate(player.fd, Skill::Attack,
                    player.skills.level(Skill::Attack),
                    player.skills.xp(Skill::Attack));

    // Grant strength XP
    player.skills.addXP(Skill::Strength, static_cast<uint16_t>(target->def().attackXP));
    sendSkillUpdate(player.fd, Skill::Strength,
                    player.skills.level(Skill::Strength),
                    player.skills.xp(Skill::Strength));

    // Send hit animation to player
    sendAnimation(player.fd, 2, target->position.x, target->position.y);  // anim_id=2 = player attack

    // Tell player the damage dealt
    std::string hitMsg = "You hit the " + std::string(target->def().name) +
                         " for " + std::to_string(damage) + " damage.";
    sendSystemMessage(player.fd, hitMsg);

    // Broadcast NPC HP update
    broadcastNpcUpdate(players, *target);

    // Reset attack timer (weapon speed from equipment)
    player.combatAttackTimer = calcPlayerAttackSpeed(player);

    // Check if NPC died
    if (target->hitpoints <= 0) {
        // NPC dies
        std::cout << "[Combat] Player fd=" << player.fd
                  << " killed " << target->def().name
                  << " (npc_id=" << target->id << ")" << std::endl;

        // Grant death XP
        player.skills.addXP(Skill::Attack, static_cast<uint16_t>(target->def().deathXP / 3));
        player.skills.addXP(Skill::Strength, static_cast<uint16_t>(target->def().deathXP / 3));
        player.skills.addXP(Skill::Defence, static_cast<uint16_t>(target->def().deathXP / 3));

        sendSkillUpdate(player.fd, Skill::Attack,
                        player.skills.level(Skill::Attack),
                        player.skills.xp(Skill::Attack));
        sendSkillUpdate(player.fd, Skill::Strength,
                        player.skills.level(Skill::Strength),
                        player.skills.xp(Skill::Strength));
        sendSkillUpdate(player.fd, Skill::Defence,
                        player.skills.level(Skill::Defence),
                        player.skills.xp(Skill::Defence));

        // Drop loot
        for (int i = 0; i < 4; ++i) {
            if (target->def().lootItems[i] != 0 && target->def().lootQty[i] > 0) {
                // Phase 4: coins go directly to gold instead of ground
                if (target->def().lootItems[i] == ItemID::COINS) {
                    int qty = target->def().lootQty[i];
                    if (qty == 5) qty = 1 + (std::rand() % 5); // Chicken random drop
                    player.gold += qty;
                    sendGoldUpdate(player.fd, player.gold);
                    sendSystemMessage(player.fd, "You receive " + std::to_string(qty) + " gold.");
                } else {
                    groundItems.addItem(target->def().lootItems[i], target->def().lootQty[i],
                                       target->position.x, target->position.y,
                                       player.fd, 600);
                    sendGroundItemAdd(player.fd, target->def().lootItems[i],
                                      target->def().lootQty[i],
                                      target->position.x, target->position.y);
                }
            }
        }

        sendSystemMessage(player.fd, "The " + std::string(target->def().name) + " is dead!");

        npcs.killNpc(target->id);
        broadcastNpcUpdate(players, *target);

        player.inCombat = false;
        player.attackingNpcId = 0;
    }
}

// Resolve NPC attacks on players (Phase 4: proper HP damage + death)
static void resolveNpcCombat(NPCManager& npcs,
                              std::unordered_map<int, ConnectedPlayer>& players) {
    for (auto& npc : npcs.mutableAll()) {
        if (!npc.alive || npc.attackingPlayerFd == 0) continue;

        npc.ticksSinceLastAttack++;
        if (npc.ticksSinceLastAttack < npc.def().attackSpeed) continue;
        npc.ticksSinceLastAttack = 0;

        // Find the attacking player
        auto it = players.find(npc.attackingPlayerFd);
        if (it == players.end()) {
            npc.attackingPlayerFd = 0;
            continue;
        }
        auto& player = it->second;
        if (player.dead) {
            npc.attackingPlayerFd = 0;
            continue;
        }

        // Check distance
        if (player.position.tileDistanceTo(npc.position) > npc.def().attackRange) {
            npc.attackingPlayerFd = 0;
            continue;
        }

        // NPC attacks player
        int maxHit = npc.def().maxHit;
        int damage = (maxHit > 0) ? (1 + (std::rand() % maxHit)) : 0;

        // Apply defence reduction (simplified: reduce damage by defence% capped)
        int defence = calcPlayerDefence(player);
        if (defence > 0 && damage > 0) {
            int reduce = std::min(damage - 1, defence / 10);
            damage -= reduce;
            if (damage < 0) damage = 0;
        }

        if (damage > 0) {
            player.currentHP = static_cast<uint8_t>(
                std::max(0, static_cast<int>(player.currentHP) - damage));

            sendHealthUpdate(player.fd, player.currentHP, player.maxHP);

            std::string msg = "The " + std::string(npc.def().name) +
                             " hits you for " + std::to_string(damage) + "!";
            sendSystemMessage(player.fd, msg);

            // Check if player died
            if (player.currentHP <= 0) {
                player.dead = true;
                player.deathTimer = 0;
                player.inCombat = false;
                player.chopping = false;
                player.mining = false;
                player.fishing = false;
                npc.attackingPlayerFd = 0;

                sendSystemMessage(player.fd, "Oh dear, you are dead!");

                // Drop most inventory items on the ground (keep 3 most valuable)
                // For Phase 4: drop all coins + equipped items
                // TODO: keep-item logic in later phase
            }
        } else {
            std::string msg = "The " + std::string(npc.def().name) +
                             " attacks but misses!";
            sendSystemMessage(player.fd, msg);
        }
    }
}

// Process packet
static void processPacket(int fd, uint8_t opcode, const uint8_t* payload, uint16_t len,
                         WorldMap& world, ConnectedPlayer& player, uint64_t tick,
                         std::unordered_map<int, ConnectedPlayer>& players,
                         WorldObjectManager& objects,
                         NPCManager& npcs, GroundItemManager& groundItems) {
    auto clientOp = static_cast<ClientOpcode>(opcode);

    switch (clientOp) {
    case ClientOpcode::WalkTile: {
        if (len < 2) return;
        uint8_t tx = payload[0];
        uint8_t ty = payload[1];

        // Cancel actions on move
        if (player.chopping) player.chopping = false;
        if (player.mining) player.mining = false;
        if (player.fishing) player.fishing = false;
        if (player.inCombat) {
            player.inCombat = false;
            // Release NPC
            NPC* target = npcs.mutableNpcById(player.attackingNpcId);
            if (target) target->attackingPlayerFd = 0;
            player.attackingNpcId = 0;
        }

        if (tx >= world.width() || ty >= world.height()) return;
        if (!world.isWalkable(tx, ty)) return;

        player.target = {static_cast<int>(tx), static_cast<int>(ty), 0};
        player.walking = true;
        break;
    }

    case ClientOpcode::ActionObject: {
        if (len < 3) return;
        uint8_t actionType = payload[0];
        uint8_t ox = payload[1];
        uint8_t oy = payload[2];

        const WorldObject* obj = objects.objectAt(ox, oy);
        if (!obj) {
            sendSystemMessage(fd, "Nothing interesting there.");
            return;
        }

        if (player.position.tileDistanceTo(obj->position) > 2) {
            sendSystemMessage(fd, "Too far away.");
            return;
        }

        if (obj->depleted) {
            if (obj->objectType == ObjectType::Tree)
                sendSystemMessage(fd, "The tree is still growing.");
            else if (obj->objectType == ObjectType::Rock)
                sendSystemMessage(fd, "This rock has been depleted.");
            else
                sendSystemMessage(fd, "The fishing spot has moved away.");
            return;
        }

        // -----------------------------------------------------------
        // Tree -> Woodcutting
        // -----------------------------------------------------------
        if (obj->objectType == ObjectType::Tree) {
            auto& treeDef = TREE_DEFS[obj->subtype];
            uint8_t wcLevel = player.skills.level(Skill::Woodcutting);
            if (wcLevel < treeDef.woodcuttingLevel) {
                sendSystemMessage(fd, "You need Woodcutting level " +
                    std::to_string(treeDef.woodcuttingLevel) + " to chop this.");
                return;
            }

            if (!player.inventory.hasItem(ItemID::BRONZE_AXE) &&
                !player.inventory.hasItem(ItemID::IRON_AXE) &&
                !player.inventory.hasItem(ItemID::STEEL_AXE) &&
                !player.inventory.hasItem(ItemID::MITHRIL_AXE) &&
                !player.inventory.hasItem(ItemID::ADAMANT_AXE) &&
                !player.inventory.hasItem(ItemID::RUNE_AXE) &&
                !player.inventory.hasItem(ItemID::DRAGON_AXE)) {
                sendSystemMessage(fd, "You need an axe to chop trees.");
                return;
            }

            // Cancel other actions
            player.mining = false;
            player.fishing = false;

            player.chopping = true;
            player.choppingObjectId = obj->id;

            int baseTicks = treeDef.chopTicksMin +
                (std::rand() % (treeDef.chopTicksMax - treeDef.chopTicksMin + 1));
            int bonus = getAxeBonus(player.inventory);
            player.chopTimer = std::max(1, baseTicks - bonus * 2);

            sendAnimation(fd, 1, ox, oy);
            break;
        }

        // -----------------------------------------------------------
        // Rock -> Mining
        // -----------------------------------------------------------
        if (obj->objectType == ObjectType::Rock) {
            auto& rockDef = ROCK_DEFS[obj->subtype];
            uint8_t mineLevel = player.skills.level(Skill::Mining);
            if (mineLevel < rockDef.miningLevel) {
                sendSystemMessage(fd, "You need Mining level " +
                    std::to_string(rockDef.miningLevel) + " to mine this rock.");
                return;
            }

            if (!player.inventory.hasItem(ItemID::COPPER_PICKAXE) &&
                !player.inventory.hasItem(ItemID::IRON_PICKAXE) &&
                !player.inventory.hasItem(ItemID::STEEL_PICKAXE)) {
                sendSystemMessage(fd, "You need a pickaxe to mine rocks.");
                return;
            }

            // Cancel other actions
            player.chopping = false;
            player.fishing = false;

            player.mining = true;
            player.miningObjectId = obj->id;

            int baseTicks = rockDef.mineTicksMin +
                (std::rand() % (rockDef.mineTicksMax - rockDef.mineTicksMin + 1));
            int bonus = getPickaxeBonus(player.inventory);
            player.mineTimer = std::max(1, baseTicks - bonus * 2);

            sendAnimation(fd, 3, ox, oy);  // anim_id=3 = mining
            sendSystemMessage(fd, std::string("You swing your pickaxe at the ") +
                              rockDef.name + ".");
            break;
        }

        // -----------------------------------------------------------
        // FishingSpot -> Fishing
        // -----------------------------------------------------------
        if (obj->objectType == ObjectType::FishingSpot) {
            auto& spotDef = FISHING_SPOT_DEFS[obj->subtype];
            uint8_t fishLevel = player.skills.level(Skill::Fishing);
            if (fishLevel < spotDef.fishingLevel) {
                sendSystemMessage(fd, "You need Fishing level " +
                    std::to_string(spotDef.fishingLevel) + " to fish here.");
                return;
            }

            if (!player.inventory.hasItem(ItemID::FISHING_ROD)) {
                sendSystemMessage(fd, "You need a fishing rod to fish.");
                return;
            }

            // Cancel other actions
            player.chopping = false;
            player.mining = false;

            player.fishing = true;
            player.fishingObjectId = obj->id;

            int baseTicks = spotDef.fishTicksMin +
                (std::rand() % (spotDef.fishTicksMax - spotDef.fishTicksMin + 1));
            player.fishTimer = std::max(1, baseTicks);

            sendAnimation(fd, 4, ox, oy);  // anim_id=4 = fishing
            sendSystemMessage(fd, std::string("You cast your line at the ") +
                              spotDef.name + ".");
            break;
        }

        sendSystemMessage(fd, "Nothing interesting there.");
        break;
    }

    case ClientOpcode::ActionNPC: {
        if (len < 3) return;
        uint8_t actionType = payload[0];
        uint8_t nx = payload[1];
        uint8_t ny = payload[2];

        // Find NPC at or near that position
        NPC* target = nullptr;
        for (auto& npc : npcs.mutableAll()) {
            if (npc.alive &&
                npc.position.x == nx && npc.position.y == ny) {
                target = &npc;
                break;
            }
        }

        if (!target) {
            // Search adjacent tiles too
            for (auto& npc : npcs.mutableAll()) {
                if (npc.alive &&
                    abs(npc.position.x - nx) <= 1 && abs(npc.position.y - ny) <= 1) {
                    target = &npc;
                    break;
                }
            }
        }

        if (!target) {
            sendSystemMessage(fd, "Nothing interesting there.");
            return;
        }

        if (player.position.tileDistanceTo(target->position) > 2) {
            sendSystemMessage(fd, "Too far away to attack.");
            return;
        }

        // Phase 4: Shopkeeper opens shop instead of combat
        if (target->type == NPCType::SHOPKEEPER) {
            player.shopOpen = true;
            sendShopOpen(fd);
            sendSystemMessage(fd, "You trade with the Shopkeeper.");
            return;
        }

        if (player.dead) {
            sendSystemMessage(fd, "You are dead!");
            return;
        }

        // Cancel other actions
        if (player.chopping) player.chopping = false;
        if (player.mining) player.mining = false;
        if (player.fishing) player.fishing = false;

        // Start combat
        player.inCombat = true;
        player.attackingNpcId = target->id;
        player.combatAttackTimer = 0;  // Attack immediately on first tick
        target->attackingPlayerFd = fd;
        target->ticksSinceLastAttack = 0;

        sendAnimation(fd, 2, target->position.x, target->position.y);  // Attack anim
        sendSystemMessage(fd, "You attack the " + std::string(target->def().name) + ".");
        break;
    }

    case ClientOpcode::ActionGroundItem: {
        if (len < 2) return;
        uint8_t ix = payload[0];
        uint8_t iy = payload[1];

        if (player.dead) return;

        uint16_t itemId = 0, qty = 0;
        if (groundItems.pickItemAt(ix, iy, fd, itemId, qty)) {
            bool added = player.inventory.addItem(itemId, qty);
            sendInventorySync(fd, player.inventory);
            if (added) {
                auto def = getItemDef(itemId);
                sendSystemMessage(fd, "You pick up " + std::string(def.name) + ".");
            } else {
                sendSystemMessage(fd, "Inventory full!");
                // Drop back on ground
                groundItems.addItem(itemId, qty, ix, iy, fd, 600);
            }
        } else {
            sendSystemMessage(fd, "Nothing to pick up here.");
        }
        break;
    }

    case ClientOpcode::InventoryAction: {
        if (len < 2) return;
        uint8_t invAction = payload[0];
        uint8_t slotIdx = payload[1];

        if (slotIdx >= INVENTORY_SIZE) return;

        auto& slot = player.inventory.slot(slotIdx);
        switch (static_cast<InvAction>(invAction)) {
        case InvAction::Equip: {
            if (slot.id == 0 || slot.quantity == 0) break;
            // Try to equip the item
            auto def = getItemDef(slot.id);
            if (def.equipSlot == static_cast<uint16_t>(EquipSlot::NONE)) {
                sendSystemMessage(fd, "You can't equip that.");
                break;
            }
            uint8_t atkLvl = player.skills.level(Skill::Attack);
            uint8_t defLvl = player.skills.level(Skill::Defence);
            ItemStack swapped = player.equipment.equip(slot.id, atkLvl, defLvl);
            if (swapped.id == 0 && swapped.quantity == 0) {
                // Check if it actually equipped (could fail due to level req)
                bool found = false;
                for (int i = 0; i < Equipment::SLOT_COUNT; ++i) {
                    if (player.equipment.slot(static_cast<EquipSlot>(i)).id == slot.id) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    sendSystemMessage(fd, "You don't have the required level to equip that.");
                    break;
                }
                // Successfully equipped (empty slot)
                player.inventory.removeItem(slot.id, 1);
            } else if (swapped.id != 0) {
                // Swapped — put old item back in inventory
                player.inventory.removeItem(slot.id, 1);
                player.inventory.addItem(swapped.id, swapped.quantity);
                if (!player.inventory.hasItem(swapped.id)) {
                    // Inventory full — undo the equip
                    EquipSlot eqSlotType = static_cast<EquipSlot>(getItemDef(slot.id).equipSlot);
                    player.equipment.unequip(eqSlotType);
                    // Put old item back in equip slot
                    player.equipment.equip(swapped.id, 255, 255);
                    // Put new item back in inventory
                    player.inventory.addItem(slot.id, 1);
                    sendSystemMessage(fd, "Not enough inventory space.");
                    break;
                }
            }
            sendInventorySync(fd, player.inventory);
            sendEquipmentSync(fd, player.equipment);
            auto itemName = getItemDef(slot.id).name;
            sendSystemMessage(fd, std::string("You equip the ") + itemName + ".");
            break;
        }
        case InvAction::Unequip: {
            if (slot.id == 0 || slot.quantity == 0) break;
            // Find which equipment slot this item is in (if any)
            // For unequip from inventory view, we unequip from the matching equip slot
            auto def = getItemDef(slot.id);
            EquipSlot eqSlot = static_cast<EquipSlot>(def.equipSlot);
            if (eqSlot == EquipSlot::NONE) break;
            if (player.equipment.slot(eqSlot).id != slot.id) break;
            ItemStack unequipped = player.equipment.unequip(eqSlot);
            if (unequipped.id != 0) {
                bool added = player.inventory.addItem(unequipped.id, 1);
                if (added) {
                    sendInventorySync(fd, player.inventory);
                    sendEquipmentSync(fd, player.equipment);
                    sendSystemMessage(fd, std::string("You remove the ") + def.name + ".");
                } else {
                    // Inventory full — re-equip
                    player.equipment.equip(unequipped.id, 255, 255);
                    sendSystemMessage(fd, "Not enough inventory space.");
                }
            }
            break;
        }
        case InvAction::Drop:
            if (slot.id != 0 && slot.quantity > 0) {
                auto def = getItemDef(slot.id);
                groundItems.addItem(slot.id, 1,
                                   player.position.x, player.position.y,
                                   fd, 600);
                player.inventory.removeItem(slot.id, 1);
                sendInventorySync(fd, player.inventory);
                sendSystemMessage(fd, "You drop the " + std::string(def.name) + ".");
            }
            break;
        case InvAction::Use: {
            // Phase 4: Eat food
            if (slot.id == 0 || slot.quantity == 0) break;
            if (isFood(slot.id)) {
                if (player.currentHP >= player.maxHP) {
                    sendSystemMessage(fd, "You are already at full health.");
                    break;
                }
                uint8_t heal = getFoodHeal(slot.id);
                uint8_t oldHP = player.currentHP;
                player.currentHP = std::min(player.maxHP,
                    static_cast<uint8_t>(player.currentHP + heal));
                uint8_t actualHeal = player.currentHP - oldHP;
                player.inventory.removeItem(slot.id, 1);
                sendInventorySync(fd, player.inventory);
                sendHealthUpdate(fd, player.currentHP, player.maxHP);
                auto def = getItemDef(slot.id);
                sendSystemMessage(fd, "You eat the " + std::string(def.name) +
                    " and heal " + std::to_string(actualHeal) + " HP.");
            } else {
                sendSystemMessage(fd, "You can't use that.");
            }
            break;
        }
        default:
            break;
        }
        break;
    }

    case ClientOpcode::ShopAction: {
        if (len < 3) return;
        auto shopAction = static_cast<ShopActionType>(payload[0]);
        uint16_t itemId = payload[1] | (static_cast<uint16_t>(payload[2]) << 8);

        switch (shopAction) {
        case ShopActionType::Buy: {
            // Find item in shop stock
            const ShopItem* shopItem = nullptr;
            for (const auto& si : GENERAL_STORE_STOCK) {
                if (si.itemId == itemId) { shopItem = &si; break; }
            }
            if (!shopItem) {
                sendSystemMessage(fd, "That item is not in stock.");
                break;
            }
            if (player.gold < shopItem->buyPrice) {
                sendSystemMessage(fd, "You don't have enough gold.");
                break;
            }
            bool added = player.inventory.addItem(shopItem->itemId, 1);
            if (!added) {
                sendSystemMessage(fd, "Inventory full!");
                break;
            }
            player.gold -= shopItem->buyPrice;
            auto def = getItemDef(shopItem->itemId);
            sendSystemMessage(fd, "You buy " + std::string(def.name) +
                " for " + std::to_string(shopItem->buyPrice) + " gold.");
            sendInventorySync(fd, player.inventory);
            sendGoldUpdate(fd, player.gold);
            break;
        }
        case ShopActionType::Sell: {
            // Check player has the item
            auto invSlot = player.inventory.findItem(itemId);
            if (invSlot < 0) {
                sendSystemMessage(fd, "You don't have that item.");
                break;
            }
            // Find sell price from shop stock
            uint16_t sellPrice = 0;
            for (const auto& si : GENERAL_STORE_STOCK) {
                if (si.itemId == itemId) { sellPrice = si.sellPrice; break; }
            }
            if (sellPrice == 0) {
                // Use item value / 2 as fallback
                auto def = getItemDef(itemId);
                sellPrice = std::max(static_cast<uint16_t>(1), static_cast<uint16_t>(def.value / 2));
            }
            player.inventory.removeItem(itemId, 1);
            player.gold += sellPrice;
            auto def = getItemDef(itemId);
            sendSystemMessage(fd, "You sell " + std::string(def.name) +
                " for " + std::to_string(sellPrice) + " gold.");
            sendInventorySync(fd, player.inventory);
            sendGoldUpdate(fd, player.gold);
            break;
        }
        case ShopActionType::Close:
            player.shopOpen = false;
            sendSystemMessage(fd, "You close the shop.");
            break;
        }
        break;
    }

    case ClientOpcode::KeepAlive: {
        sendPacket(fd, ServerOpcode::KeepAliveResponse);
        break;
    }

    default:
        break;
    }
}

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  --port <num>    TCP port (default: " << DEFAULT_PORT << ")\n"
              << "  --db <path>     Database file (default: ./save.db)\n"
              << "  --help          Show this help\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
    std::string db_path = "./save.db";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }

    std::cout << "╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║      Erynfall Server v0.4.0          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // --- Database ---
    auto db = createDatabase();
    if (!db->open(db_path)) {
        std::cerr << "[Server] Failed to open database: " << db_path << std::endl;
        return 1;
    }

    // --- World Map ---
    WorldMap world(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT);
    world.fill(TileType::Grass);

    for (int x = 10; x < 14; ++x) {
        for (int y = 20; y < 25; ++y) {
            world.setTile(x, y, TileType::Water);
        }
    }

    std::cout << "[World] Map initialized: " << DEFAULT_MAP_WIDTH << "x" << DEFAULT_MAP_HEIGHT
              << " (" << world.size() << " tiles)" << std::endl;

    // --- World Objects (Trees) ---
    WorldObjectManager objects;
    std::vector<std::pair<int,int>> treePositions = {
        {2, 2}, {4, 1}, {1, 4}, {5, 3}, {3, 6}, {6, 5}, {2, 8}, {7, 2},
        {5, 7}, {1, 9}, {8, 4}, {4, 10}, {3, 3}, {7, 7}, {9, 1},
        {3, 12}, {7, 13}, {12, 10}, {17, 8}, {22, 12}, {25, 11},
        {5, 17}, {20, 5}, {26, 7}, {15, 4}, {18, 17},
        {8, 8}, {14, 6}, {20, 3}, {27, 9},
        {2, 16}, {25, 5},
    };

    std::vector<TreeType> treeTypes;
    for (size_t i = 0; i < treePositions.size(); ++i) {
        if (i >= 21 && i < 25) treeTypes.push_back(TreeType::Oak);
        else if (i >= 25) treeTypes.push_back(TreeType::Willow);
        else treeTypes.push_back(TreeType::Normal);
    }

    objects.populateForest(treePositions, treeTypes);
    std::cout << "[World] Placed " << objects.count() << " trees" << std::endl;

    // --- World Objects (Rocks - Mining area in the south) ---
    std::vector<std::pair<int,int>> rockPositions = {
        {3, 18}, {4, 18}, {5, 18}, {6, 18}, {7, 18},
        {3, 19}, {4, 19}, {6, 19}, {7, 19},
        {3, 20}, {5, 20}, {7, 20},
        {4, 21}, {6, 21},
        {5, 22}, {3, 22}, {7, 22},
    };
    std::vector<RockType> rockTypes;
    for (size_t i = 0; i < rockPositions.size(); ++i) {
        if (i < 6)
            rockTypes.push_back(RockType::Copper);
        else if (i < 11)
            rockTypes.push_back(RockType::Iron);
        else if (i < 14)
            rockTypes.push_back(RockType::Gold);
        else
            rockTypes.push_back(RockType::Mithril);
    }
    objects.populateRocks(rockPositions, rockTypes);
    std::cout << "[World] Placed " << rockPositions.size() << " mining rocks" << std::endl;

    // --- World Objects (Fishing spots - Fishing area in the south-east) ---
    std::vector<std::pair<int,int>> fishingPositions = {
        {22, 20}, {23, 20}, {24, 21}, {25, 21}, {22, 22}, {25, 22},
    };
    std::vector<FishingSpotType> fishingTypes;
    for (size_t i = 0; i < fishingPositions.size(); ++i) {
        if (i < 2)
            fishingTypes.push_back(FishingSpotType::ShrimpPool);
        else if (i < 4)
            fishingTypes.push_back(FishingSpotType::SardinePool);
        else
            fishingTypes.push_back(FishingSpotType::TroutPool);
    }
    objects.populateFishingSpots(fishingPositions, fishingTypes);
    std::cout << "[World] Placed " << fishingPositions.size() << " fishing spots" << std::endl;

    // --- NPCs ---
    NPCManager npcs;
    npcs.initialize();

    // --- Ground Items ---
    GroundItemManager groundItems;

    // --- Players & buffers ---
    std::unordered_map<int, ConnectedPlayer> players;
    std::unordered_map<int, std::vector<uint8_t>> recv_buffers;

    // --- TCP Server ---
    TcpServer tcp;
    if (!tcp.bind(port)) {
        std::cerr << "[Server] Failed to bind port " << port << std::endl;
        return 1;
    }
    if (!tcp.listen()) {
        std::cerr << "[Server] Failed to listen on port " << port << std::endl;
        return 1;
    }

    std::cout << "[Server] Listening on port " << tcp.boundPort() << std::endl;
    std::cout << "[Server] Tick rate: " << TICK_RATE_MS << "ms ("
              << (1000.0 / TICK_RATE_MS) << " ticks/sec)" << std::endl;
    std::cout << "[Server] Protocol version: " << PROTOCOL_VERSION << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to shut down." << std::endl;
    std::cout << std::endl;

    // --- Game Loop ---
    GameLoop gameLoop(TICK_RATE_MS);
    std::atomic<uint64_t> tick_count{0};

    gameLoop.setOnTick([&world, &tcp, &db, &players, &recv_buffers, &tick_count,
                       &objects, &npcs, &groundItems](uint64_t tick) {
        uint64_t current_tick = tick_count.fetch_add(1);

        // Tick world systems
        objects.tick(current_tick);
        npcs.tickRespawns();
        npcs.tickWandering();
        groundItems.tick();

        // Broadcast NPC updates for wandering/respawning NPCs
        for (const auto& npc : npcs.all()) {
            if (npc.alive && npc.isWandering) {
                broadcastNpcUpdate(players, npc);
            }
        }

        // Accept new connections
        tcp.acceptPending([&](int client_fd, sockaddr_in addr) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
            std::cout << "[Network] Connection from " << ip
                      << " (fd=" << client_fd << ")" << std::endl;

            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

            ConnectedPlayer p;
            p.fd = client_fd;
            p.position = {DEFAULT_MAP_WIDTH / 2, DEFAULT_MAP_HEIGHT / 2, 0};
            p.target = p.position;
            p.walking = false;
            p.last_move_tick = 0;
            p.dead = false;
            p.deathTimer = 0;
            p.init();

            players[client_fd] = p;
            recv_buffers[client_fd] = {};

            // Full sync
            sendSystemMessage(client_fd, "Welcome to Erynfall!");
            sendPositionUpdate(client_fd, p.position);
            sendInventorySync(client_fd, p.inventory);
            sendEquipmentSync(client_fd, p.equipment);
            sendSkillsSync(client_fd, p.skills);
            sendGoldUpdate(client_fd, p.gold);
            sendHealthUpdate(client_fd, p.currentHP, p.maxHP);

            // Send tree locations
            for (const auto& obj : objects.objects()) {
                sendWorldObjectUpdate(client_fd, 1, obj);
            }

            // Send NPC locations
            for (const auto& npc : npcs.all()) {
                if (npc.alive) {
                    sendNpcUpdate(client_fd, npc);
                }
            }
        });

        // Read from all players
        for (auto it = players.begin(); it != players.end(); ) {
            int fd = it->first;
            auto& player = it->second;
            auto& buf = recv_buffers[fd];

            // Handle dead players
            if (player.dead) {
                player.deathTimer++;
                if (player.deathTimer >= 8) {  // ~5 seconds
                    // Respawn
                    player.dead = false;
                    player.deathTimer = 0;
                    player.position = {DEFAULT_MAP_WIDTH / 2, DEFAULT_MAP_HEIGHT / 2, 0};
                    player.target = player.position;
                    player.inCombat = false;
                    player.attackingNpcId = 0;
                    player.chopping = false;
                    player.mining = false;
                    player.fishing = false;

                    // Restore HP to full
                    player.recalcMaxHP();
                    player.currentHP = player.maxHP;

                    sendPositionUpdate(fd, player.position);
                    sendSkillsSync(fd, player.skills);
                    sendEquipmentSync(fd, player.equipment);
                    sendGoldUpdate(fd, player.gold);
                    sendHealthUpdate(fd, player.currentHP, player.maxHP);
                    sendSystemMessage(fd, "You respawn at home.");
                }
                ++it;
                continue;
            }

            auto data = readAvailable(fd);
            if (data.empty()) {
                int err = errno;
                if (err != EAGAIN && err != EWOULDBLOCK) {
                    std::cout << "[Network] Player fd=" << fd << " disconnected" << std::endl;
                    ::close(fd);
                    recv_buffers.erase(fd);
                    it = players.erase(it);
                    continue;
                }
                ++it;
                continue;
            }

            buf.insert(buf.end(), data.begin(), data.end());

            while (buf.size() >= 2) {
                uint16_t pkt_len = buf[0];
                if (buf.size() < 1 + pkt_len) break;

                uint8_t opcode = buf[1];
                const uint8_t* payload = buf.size() > 2 ? &buf[2] : nullptr;
                uint16_t payload_len = pkt_len - 1;

                processPacket(fd, opcode, payload, payload_len,
                             world, player, current_tick, players,
                             objects, npcs, groundItems);

                buf.erase(buf.begin(), buf.begin() + 1 + pkt_len);
            }
            ++it;
        }

        // Process movement
        for (auto& [fd, player] : players) {
            if (player.dead) continue;
            if (!player.walking || player.target == player.position) continue;

            auto diff = player.target - player.position;
            int step_x = 0, step_y = 0;
            if (std::abs(diff.x) >= std::abs(diff.y)) {
                step_x = (diff.x > 0) ? 1 : -1;
            } else {
                step_y = (diff.y > 0) ? 1 : -1;
            }
            int nx = player.position.x + step_x;
            int ny = player.position.y + step_y;
            if (nx >= 0 && nx < world.width() && ny >= 0 && ny < world.height()
                && world.isWalkable(nx, ny)) {
                player.position = {nx, ny, 0};
                player.last_move_tick = current_tick;
                sendPositionUpdate(fd, player.position);
                if (player.position == player.target) {
                    player.walking = false;
                }
            }
        }

        // Resolve actions
        for (auto& [fd, player] : players) {
            if (player.dead) continue;
            resolveWoodcutting(player, objects, current_tick);
            resolveMining(player, objects, current_tick);
            resolveFishing(player, objects, current_tick);
            resolvePlayerCombat(player, npcs, groundItems, players);
        }

        // Resolve NPC attacks
        resolveNpcCombat(npcs, players);

        // Periodic save
        if (current_tick > 0 && current_tick % SAVE_INTERVAL_TICKS == 0) {
            // TODO: Phase 7
        }
    });

    std::thread loopThread([&gameLoop]() {
        gameLoop.run();
    });

    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    gameLoop.stop();
    loopThread.join();

    for (auto& [fd, player] : players) {
        sendSystemMessage(fd, "Server shutting down.");
        ::close(fd);
    }

    tcp.close();
    db->close();

    std::cout << "[Server] Shutdown complete." << std::endl;
    return 0;
}
