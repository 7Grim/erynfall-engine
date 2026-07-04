#include "core/game_loop.h"
#include "net/tcp_server.h"
#include "world/world_map.h"
#include "db/database.h"

#include "constants/game_constants.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

using namespace erynfall;

// Global shutdown flag for signal handler
static std::atomic<bool> g_shutdown{false};

static void signalHandler(int /*signum*/) {
    std::cout << "\n[Server] Shutting down..." << std::endl;
    g_shutdown.store(true);
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

    // Parse args
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
    std::cout << "║      Erynfall Server v0.1.0          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Register signal handlers for clean shutdown
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
    std::cout << "[World] Map initialized: " << DEFAULT_MAP_WIDTH << "x" << DEFAULT_MAP_HEIGHT
              << " (" << world.size() << " tiles)" << std::endl;

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

    gameLoop.setOnTick([&world, &tcp, &db](uint64_t tick) {
        // Accept new connections
        tcp.acceptPending([](int client_fd, sockaddr_in addr) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
            std::cout << "[Network] Connection from " << ip
                      << " (fd=" << client_fd << ")" << std::endl;

            // TODO: Phase 7 — read login packet, authenticate, add to world
            // For now, send a server message and close
            const char* msg = "Erynfall Server v0.1.0 — Login not implemented yet.\n";
            ::send(client_fd, msg, std::strlen(msg), 0);
            ::close(client_fd);
        });

        // Periodic save (every SAVE_INTERVAL_TICKS)
        if (tick > 0 && tick % SAVE_INTERVAL_TICKS == 0) {
            // TODO: Phase 2+ — save connected players
        }
    });

    // Run in a separate thread so signal handler can stop it
    std::thread loopThread([&gameLoop]() {
        gameLoop.run();
    });

    // Wait for shutdown signal
    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    gameLoop.stop();
    loopThread.join();

    tcp.close();
    db->close();

    std::cout << "[Server] Shutdown complete." << std::endl;
    return 0;
}
