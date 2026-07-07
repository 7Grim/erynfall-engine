#pragma once

#include <cstdint>

namespace erynfall {

// Client → Server
enum class ClientOpcode : uint8_t {
    LoginRequest     = 0x01,
    WalkTile         = 0x02,
    ActionObject     = 0x03,
    ActionNPC        = 0x04,
    ActionGroundItem = 0x05,
    InventoryAction  = 0x06,
    EquipmentAction  = 0x07,
    ChatMessage      = 0x08,
    KeepAlive        = 0x09,
    ShopAction       = 0x0A,  // Phase 4: buy/sell from shop
};

// Server → Client
enum class ServerOpcode : uint8_t {
    LoginResponse    = 0x81,
    RegionUpdate     = 0x82,
    PlayerUpdate     = 0x83,
    InventorySync    = 0x84,
    EquipmentSync    = 0x85,
    SkillUpdate      = 0x86,
    Animation        = 0x87,
    GroundItemAdd    = 0x88,
    GroundItemRemove = 0x89,
    SystemMessage    = 0x90,
    PlayerAppearance = 0x91,
    PlayerAddRemove  = 0x92,
    NPCUpdate        = 0x93,
    KeepAliveResponse = 0x94,
    LogoutResponse   = 0x95,
    ShopOpen         = 0x96,  // Phase 4: send shop stock to client
    GoldUpdate       = 0x97,  // Phase 4: player gold balance
    HealthUpdate     = 0x98,  // Phase 4: current HP update
};

// Login status codes
enum class LoginStatus : uint8_t {
    Success             = 0,
    InvalidCredentials = 1,
    ServerFull         = 2,
    AlreadyOnline      = 3,
    WrongVersion       = 4,
};

// Action types for interacting with objects/NPCs/items
enum class ActionType : uint8_t {
    Interact1 = 0,  // Left click / primary action
    Interact2 = 1,  // Right click option 1
    Interact3 = 2,  // Right click option 2
    Interact4 = 3,  // Right click option 3
    Examine   = 4,  // Examine
};

// Inventory/equipment action types
enum class InvAction : uint8_t {
    Equip      = 0,
    Unequip    = 1,
    Drop       = 2,
    Use        = 3,
    Swap       = 4,
};

// Shop action types (for ClientOpcode::ShopAction)
enum class ShopActionType : uint8_t {
    Buy        = 0,
    Sell       = 1,
    Close      = 2,
};

} // namespace erynfall
