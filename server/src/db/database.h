#pragma once

#include <functional>
#include <memory>
#include <string>

struct sqlite3;

namespace erynfall {

// Interface for player persistence.
// SQLite implementation for dev; swap for PostgreSQL later.
class Database {
public:
    Database() = default;
    virtual ~Database() = default;

    // Initialize (open file, create tables if needed)
    virtual bool open(const std::string& db_path) = 0;

    // Close the database
    virtual void close() = 0;

    // Check if a player exists
    virtual bool playerExists(const std::string& username) = 0;

    // Create a new player (returns player ID, 0 on failure)
    virtual int createPlayer(const std::string& username,
                             const std::string& password_hash,
                             int start_x, int start_y, int start_z) = 0;

    // Save player state (call periodically)
    virtual bool savePlayer(int player_id,
                            int pos_x, int pos_y, int pos_z,
                            const std::string& skills_json,
                            const std::string& inventory_json,
                            const std::string& equipment_json,
                            const std::string& bank_json) = 0;
};

// Factory — creates SQLite implementation
std::unique_ptr<Database> createDatabase();

} // namespace erynfall
