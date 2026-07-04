#include "core/game_loop.h"
#include "net/tcp_server.h"
#include "world/world_map.h"
#include "db/database.h"
#include "protocol/opcodes.h"
#include "protocol/buffer.h"

#include "constants/game_constants.h"

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
    int fd;
    Position position;
    Position target;
    bool walking;
    uint64_t last_move_tick;
};

static std::atomic<bool> g_shutdown{false};

static void signalHandler(int /*signum*/) {
    std::cout << "\n[Server] Shutting down..." << std::endl;
    g_shutdown.store(true);
}

// Read all available bytes from a non-blocking socket
static std::vector<uint8_t> readAvailable(int fd) {
    std::vector<uint8_t> buf(4096);
    ssize_t n = ::recv(fd, buf.data(), buf.size(), 0);
    if (n <= 0) return {};
    buf.resize(n);
    return buf;
}

// Send raw bytes
static bool sendBytes(int fd, const std::vector<uint8_t>& data) {
    ssize_t n = ::send(fd, data.data(), data.size(), 0);
    return n == static_cast<ssize_t>(data.size());
}

// Send a server opcode packet
static bool sendPacket(int fd, ServerOpcode opcode, const uint8_t* payload = nullptr, uint16_t len = 0) {
    uint16_t total = 2 + len;  // opcode + payload
    std::vector<uint8_t> packet(3 + total);  // length prefix (1) + total
    packet[0] = static_cast<uint8_t>(total);  // length prefix
    packet[1] = static_cast<uint8_t>(opcode); // opcode
    if (payload && len > 0) {
        std::memcpy(&packet[2], payload, len);
    }
    return sendBytes(fd, packet);
}

// Send a system message (text string)
static bool sendSystemMessage(int fd, const std::string& msg) {
    ByteBuffer buf;
    buf.writeString(msg.c_str());
    return sendPacket(fd, ServerOpcode::SystemMessage, buf.data(),
                     static_cast<uint16_t>(buf.size()));
}

// Send player position update
static bool sendPositionUpdate(int fd, const Position& pos) {
    uint8_t payload[2];
    payload[0] = static_cast<uint8_t>(pos.x);
    payload[1] = static_cast<uint8_t>(pos.y);
    return sendPacket(fd, ServerOpcode::PlayerUpdate, payload, 2);
}

// Process a complete packet from a client
static void processPacket(int fd, uint8_t opcode, const uint8_t* payload, uint16_t len,
                         WorldMap& world, ConnectedPlayer& player, uint64_t tick,
                         std::unordered_map<int, ConnectedPlayer>& players) {
    auto clientOp = static_cast<ClientOpcode>(opcode);

    switch (clientOp) {
    case ClientOpcode::WalkTile: {
        if (len < 2) {
            sendSystemMessage(fd, "ERR: WalkTile packet too short");
            return;
        }
        uint8_t tx = payload[0];
        uint8_t ty = payload[1];

        // Bounds check
        if (tx >= world.width() || ty >= world.height()) {
            sendSystemMessage(fd, "ERR: Out of bounds");
            return;
        }

        // Walkability check
        if (!world.isWalkable(tx, ty)) {
            sendSystemMessage(fd, "ERR: Can't walk there");
            return;
        }

        // Rate limit: 1 move per tick
        if (tick - player.last_move_tick < 1) {
            // Queue the target, process next tick
            player.target = {static_cast<int>(tx), static_cast<int>(ty), 0};
            player.walking = true;
            return;
        }

        player.last_move_tick = tick;
        player.position = {static_cast<int>(tx), static_cast<int>(ty), 0};
        player.walking = false;

        std::cout << "[Movement] Player fd=" << fd
                  << " moved to (" << (int)tx << "," << (int)ty << ")" << std::endl;

        // Echo position back
        sendPositionUpdate(fd, player.position);

        // TODO: Phase 7 — broadcast to nearby players
        break;
    }

    case ClientOpcode::KeepAlive: {
        sendPacket(fd, ServerOpcode::KeepAliveResponse);
        break;
    }

    default:
        std::cout << "[Protocol] Unknown opcode 0x"
                  << std::hex << (int)opcode << std::dec
                  << " from fd=" << fd << std::endl;
        break;
    }
}

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  --port <num>    TCP port (default: " << DEFAULT_PORT << ")\n"
              << "  --data <path>   Data directory (default: ./data/)\n"
              << "  --db <path>     Database file (default: ./save.db)\n"
              << "  --help          Show this help\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
    std::string data_path = "./data/";
    std::string db_path = "./save.db";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--data" && i + 1 < argc) {
            data_path = argv[++i];
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }

    std::cout << "╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║      Erynfall Server v0.2.0          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // --- Database ---
    auto db = createDatabase();
    if (!db->open(db_path)) {
        std::cerr << "[Server] Failed to open database: " << db_path << std::endl;
        return 1;
    }

    // --- World Map ---
    WorldMap world(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT);
    world.fill(TileType::Grass);

    // Add water features matching client
    for (int x = 10; x < 14; ++x) {
        for (int y = 20; y < 25; ++y) {
            world.setTile(x, y, TileType::Water);
        }
    }

    std::cout << "[World] Map initialized: " << DEFAULT_MAP_WIDTH << "x" << DEFAULT_MAP_HEIGHT
              << " (" << world.size() << " tiles)" << std::endl;

    // --- Connected Players ---
    std::unordered_map<int, ConnectedPlayer> players;

    // Per-client receive buffers (for TCP stream reassembly)
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
    std::cout << "[Server] Max players: " << MAX_PLAYERS << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to shut down." << std::endl;
    std::cout << std::endl;

    // --- Game Loop ---
    GameLoop gameLoop(TICK_RATE_MS);
    std::atomic<uint64_t> tick_count{0};

    gameLoop.setOnTick([&world, &tcp, &db, &players, &recv_buffers, &tick_count](uint64_t tick) {
        uint64_t current_tick = tick_count.fetch_add(1);

        // Accept new connections
        tcp.acceptPending([&players, &recv_buffers, &world](int client_fd, sockaddr_in addr) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
            std::cout << "[Network] Connection from " << ip
                      << " (fd=" << client_fd << ")" << std::endl;

            // Set non-blocking
            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

            // Initialize player at center of map
            ConnectedPlayer p;
            p.fd = client_fd;
            p.position = {DEFAULT_MAP_WIDTH / 2, DEFAULT_MAP_HEIGHT / 2, 0};
            p.target = p.position;
            p.walking = false;
            p.last_move_tick = 0;
            players[client_fd] = p;
            recv_buffers[client_fd] = {};

            // Send welcome + initial position
            sendSystemMessage(client_fd, "Welcome to Erynfall!");
            sendPositionUpdate(client_fd, p.position);
        });

        // Read data from all connected players
        for (auto it = players.begin(); it != players.end(); ) {
            int fd = it->first;
            auto& player = it->second;
            auto& buf = recv_buffers[fd];

            auto data = readAvailable(fd);
            if (data.empty()) {
                // Check for disconnect (recv returned -1 with EAGAIN is ok, -1 otherwise = disconnect)
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

            // Append to buffer
            buf.insert(buf.end(), data.begin(), data.end());

            // Process complete packets
            while (buf.size() >= 2) {
                uint16_t pkt_len = buf[0];  // length prefix (opcode + payload)
                if (buf.size() < 1 + pkt_len) break;  // incomplete packet

                uint8_t opcode = buf[1];
                const uint8_t* payload = buf.size() > 2 ? &buf[2] : nullptr;
                uint16_t payload_len = pkt_len - 1;  // opcode byte is part of length

                processPacket(fd, opcode, payload, payload_len,
                             world, player, current_tick, players);

                buf.erase(buf.begin(), buf.begin() + 1 + pkt_len);
            }

            ++it;
        }

        // Process queued movement
        for (auto& [fd, player] : players) {
            if (player.walking && player.target != player.position) {
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
        }

        // Periodic save
        if (tick > 0 && tick % SAVE_INTERVAL_TICKS == 0) {
            // TODO: Phase 2+ — save all connected players
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

    // Close all player connections
    for (auto& [fd, player] : players) {
        sendSystemMessage(fd, "Server shutting down.");
        ::close(fd);
    }

    tcp.close();
    db->close();

    std::cout << "[Server] Shutdown complete." << std::endl;
    return 0;
}
