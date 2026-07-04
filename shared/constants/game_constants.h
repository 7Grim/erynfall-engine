#pragma once

namespace erynfall {

// Game tick rate — identical to OSRS (600ms = ~1.67 ticks/second)
constexpr int TICK_RATE_MS = 600;

// Tile dimensions
constexpr int TILE_SIZE = 1;          // 1 unit per tile
constexpr int REGION_SIZE = 8;         // 8x8 tiles per region
constexpr int VIEWPORT_REGIONS = 2;   // 2x2 region viewport per player

// Server limits (dev)
constexpr int MAX_PLAYERS = 16;       // Dev limit — prod will be higher
constexpr int MAX_NPCS = 256;
constexpr int MAX_GROUND_ITEMS = 1024;

// Network
constexpr int DEFAULT_PORT = 43594;   // Classic RS port
constexpr int KEEPALIVE_INTERVAL_TICKS = 30;
constexpr int KEEPALIVE_TIMEOUT_TICKS = 60;

// Persistence
constexpr int SAVE_INTERVAL_TICKS = 100;  // Save every 100 ticks (~60s)

// Inventory
constexpr int INVENTORY_SIZE = 28;
constexpr int EQUIPMENT_SLOTS = 10;

// Protocol
constexpr int PROTOCOL_VERSION = 1;

// Map
constexpr int DEFAULT_MAP_WIDTH = 30;
constexpr int DEFAULT_MAP_HEIGHT = 30;

} // namespace erynfall
