#include "core/game_loop.h"
#include "net/tcp_server.h"
#include "world/world_map.h"
#include "world/world_objects.h"
#include "db/database.h"
#include "game/inventory.h"
#include "game/skill_set.h"
#include "protocol/opcodes.h"
#include "protocol/buffer.h"

#include "constants/game_constants.h"
#include "game_data/item_defs.h"
#include "game_data/xp_table.h"

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
    uint32_t choppingObjectId = 0;    // Which tree
    int chopsRemaining = 0;          // Ticks until success
    int chopTimer = 0;                // Countdown

    // Give bronze axe on join
    void init() {
        inventory.addItem(ItemID::BRONZE_AXE, 1);
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
static bool sendSkillUpdate(int fd, Skill skill) {
    // We need skill state, so this is called from context that has it
    return false; // Overloaded below
}

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
    uint16_t total = 1 + len;  // opcode + payload
    std::vector<uint8_t> packet(1 + 1 + total);  // length(1) + opcode(1) + payload
    packet[0] = static_cast<uint8_t>(total);       // length prefix
    packet[1] = static_cast<uint8_t>(opcode);    // opcode
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

// Send world object update (add/remove)
// Format: uint8 action (0=remove, 1=add), uint32 objId, uint8 type, uint8 x, uint8 y = 7 bytes
static bool sendWorldObjectUpdate(int fd, uint8_t action, const WorldObject& obj) {
    uint8_t buf[7];
    buf[0] = action;  // 0 = removed, 1 = added
    buf[1] = static_cast<uint8_t>(obj.id & 0xFF);
    buf[2] = static_cast<uint8_t>((obj.id >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>((obj.id >> 16) & 0xFF);
    buf[4] = static_cast<uint8_t>((obj.id >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>(obj.treeType);
    buf[6] = 0; // placeholder
    return sendPacket(fd, ServerOpcode::GroundItemAdd, buf, 7);  // Reusing for now
}

// Check if player has an axe equipped (returns best axe bonus)
static int getAxeBonus(const Inventory& inv) {
    // Check if player has any axe in inventory
    for (int i = 0; i < inv.size(); ++i) {
        auto& slot = inv.slot(i);
        if (slot.quantity == 0) continue;
        for (const auto& axe : AXE_DEFS) {
            if (slot.id == axe.itemId) {
                return axe.woodcuttingBonus;
            }
        }
    }
    return 0;  // No axe
}

// Resolve woodcutting action on tick
static void resolveWoodcutting(ConnectedPlayer& player, WorldObjectManager& objects,
                               uint64_t tick) {
    if (!player.chopping) return;

    WorldObject* obj = objects.mutableObjectAt(
        player.position.x, player.position.y);
    if (!obj || obj->depleted || obj->id != player.choppingObjectId) {
        // Tree gone or different tree
        player.chopping = false;
        sendSystemMessage(player.fd, "Tree is gone.");
        return;
    }

    // Check distance
    if (player.position.tileDistanceTo(obj->position) > 1) {
        player.chopping = false;
        sendSystemMessage(player.fd, "You moved too far.");
        return;
    }

    player.chopTimer--;
    if (player.chopTimer > 0) return;  // Still chopping

    // Chop complete — success!
    const auto& treeDef = TREE_DEFS[static_cast<int>(obj->treeType)];

    // Grant XP
    bool leveled = player.skills.addXP(Skill::Woodcutting, treeDef.xpPerChop);
    auto wcState = player.skills.get(Skill::Woodcutting);
    sendSkillUpdate(player.fd, Skill::Woodcutting, wcState.level, wcState.xp);

    if (leveled) {
        std::string msg = "Congratulations! Your Woodcutting level is now " +
                          std::to_string(wcState.level) + "!";
        sendSystemMessage(player.fd, msg);
    }

    // Add logs to inventory
    bool added = player.inventory.addItem(treeDef.logItem, 1);
    sendInventorySync(player.fd, player.inventory);
    if (!added) {
        sendSystemMessage(player.fd, "Inventory full!");
    }

    // Chop the tree
    bool depleted = objects.chop(obj->id, player.fd);

    std::cout << "[Woodcutting] Player fd=" << player.fd
              << " chopped " << treeDef.name
              << " (xp=" << treeDef.xpPerChop << ")"
              << (depleted ? " [DEPLETED]" : "") << std::endl;

    if (depleted) {
        sendSystemMessage(player.fd, std::string("The ") + treeDef.name + " has been cut down.");
        player.chopping = false;
        return;
    }

    // Set up next chop
    auto& def = TREE_DEFS[static_cast<int>(obj->treeType)];
    int baseTicks = def.chopTicksMin + (std::rand() % (def.chopTicksMax - def.chopTicksMin + 1));
    int bonus = getAxeBonus(player.inventory);
    player.chopTimer = std::max(1, baseTicks - bonus * 2);
}

// Process packet
static void processPacket(int fd, uint8_t opcode, const uint8_t* payload, uint16_t len,
                         WorldMap& world, ConnectedPlayer& player, uint64_t tick,
                         std::unordered_map<int, ConnectedPlayer>& players,
                         WorldObjectManager& objects) {
    auto clientOp = static_cast<ClientOpcode>(opcode);

    switch (clientOp) {
    case ClientOpcode::WalkTile: {
        if (len < 2) return;
        uint8_t tx = payload[0];
        uint8_t ty = payload[1];

        // Cancel chopping on move
        if (player.chopping) {
            player.chopping = false;
        }

        if (tx >= world.width() || ty >= world.height()) return;
        if (!world.isWalkable(tx, ty)) return;

        player.target = {static_cast<int>(tx), static_cast<int>(ty), 0};
        player.walking = true;
        break;
    }

    case ClientOpcode::ActionObject: {
        // ActionObject: uint8 type, uint8 x, uint8 y
        if (len < 3) return;
        uint8_t actionType = payload[0];
        uint8_t ox = payload[1];
        uint8_t oy = payload[2];

        // Find tree at position
        const WorldObject* obj = objects.objectAt(ox, oy);
        if (!obj) {
            sendSystemMessage(fd, "Nothing interesting there.");
            return;
        }

        // Check distance (must be adjacent)
        if (player.position.tileDistanceTo(obj->position) > 2) {
            sendSystemMessage(fd, "Too far away.");
            return;
        }

        if (obj->depleted) {
            sendSystemMessage(fd, "The tree is still growing.");
            return;
        }

        // Check woodcutting level
        auto& treeDef = TREE_DEFS[static_cast<int>(obj->treeType)];
        uint8_t wcLevel = player.skills.level(Skill::Woodcutting);
        if (wcLevel < treeDef.woodcuttingLevel) {
            sendSystemMessage(fd, "You need Woodcutting level " +
                std::to_string(treeDef.woodcuttingLevel) + " to chop this.");
            return;
        }

        // Check axe
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

        // Start chopping
        player.chopping = true;
        player.choppingObjectId = obj->id;

        int baseTicks = treeDef.chopTicksMin +
            (std::rand() % (treeDef.chopTicksMax - treeDef.chopTicksMin + 1));
        int bonus = getAxeBonus(player.inventory);
        player.chopTimer = std::max(1, baseTicks - bonus * 2);

        // Tell client to play chopping animation
        sendAnimation(fd, 1, ox, oy);  // anim_id=1 = chop

        std::cout << "[Woodcutting] Player fd=" << fd
                  << " starts chopping " << treeDef.name
                  << " at (" << (int)ox << "," << (int)oy << ")"
                  << " (timer=" << player.chopTimer << " ticks)" << std::endl;
        break;
    }

    case ClientOpcode::InventoryAction: {
        if (len < 2) return;
        uint8_t invAction = payload[0];
        uint8_t slotIdx = payload[1];

        if (slotIdx >= INVENTORY_SIZE) return;

        auto& slot = player.inventory.slot(slotIdx);
        switch (static_cast<InvAction>(invAction)) {
        case InvAction::Drop:
            if (slot.id != 0 && slot.quantity > 0) {
                auto def = getItemDef(slot.id);
                std::string msg = "You drop the " + std::string(def.name) + ".";
                player.inventory.removeItem(slot.id, 1);
                sendInventorySync(fd, player.inventory);
                sendSystemMessage(fd, msg);
            }
            break;
        default:
            break;  // Other inventory actions not yet implemented
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
    std::cout << "║      Erynfall Server v0.3.0          ║" << std::endl;
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

    // Water pond (10-13, 20-24)
    for (int x = 10; x < 14; ++x) {
        for (int y = 20; y < 25; ++y) {
            world.setTile(x, y, TileType::Water);
        }
    }

    std::cout << "[World] Map initialized: " << DEFAULT_MAP_WIDTH << "x" << DEFAULT_MAP_HEIGHT
              << " (" << world.size() << " tiles)" << std::endl;

    // --- World Objects (Trees) ---
    WorldObjectManager objects;

    // Forest area: scatter normal trees in top-left quadrant
    std::vector<std::pair<int,int>> treePositions = {
        // Dense forest (top-left)
        {2, 2}, {4, 1}, {1, 4}, {5, 3}, {3, 6}, {6, 5}, {2, 8}, {7, 2},
        {5, 7}, {1, 9}, {8, 4}, {4, 10}, {3, 3}, {7, 7}, {9, 1},
        // Sparse trees near path
        {3, 12}, {7, 13}, {12, 10}, {17, 8}, {22, 12}, {25, 11},
        // Scattered elsewhere
        {5, 17}, {20, 5}, {26, 7}, {15, 4}, {18, 17},
        // Oak trees (require level 15)
        {8, 8}, {14, 6}, {20, 3}, {27, 9},
        // Willow trees (require level 30)
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
                       &objects](uint64_t tick) {
        uint64_t current_tick = tick_count.fetch_add(1);

        // Tick world objects (respawns)
        objects.tick(current_tick);

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
            p.init();  // Give bronze axe

            players[client_fd] = p;
            recv_buffers[client_fd] = {};

            // Send full sync: welcome, position, inventory, skills
            sendSystemMessage(client_fd, "Welcome to Erynfall!");
            sendPositionUpdate(client_fd, p.position);
            sendInventorySync(client_fd, p.inventory);
            sendSkillsSync(client_fd, p.skills);

            // Send tree locations (initial object sync)
            for (const auto& obj : objects.objects()) {
                sendWorldObjectUpdate(client_fd, 1, obj);
            }
        });

        // Read from all players
        for (auto it = players.begin(); it != players.end(); ) {
            int fd = it->first;
            auto& player = it->second;
            auto& buf = recv_buffers[fd];

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
                             world, player, current_tick, players, objects);

                buf.erase(buf.begin(), buf.begin() + 1 + pkt_len);
            }
            ++it;
        }

        // Process movement
        for (auto& [fd, player] : players) {
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

        // Resolve woodcutting actions
        for (auto& [fd, player] : players) {
            resolveWoodcutting(player, objects, current_tick);
        }

        // Periodic save
        if (current_tick > 0 && current_tick % SAVE_INTERVAL_TICKS == 0) {
            // TODO: Phase 7 — persist to DB
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
