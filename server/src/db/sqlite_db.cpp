#include "db/database.h"
#include <sqlite3.h>
#include <iostream>
#include <memory>

namespace erynfall {

class SqliteDatabase : public Database {
public:
    ~SqliteDatabase() override { close(); }

    bool open(const std::string& db_path) override {
        if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
            std::cerr << "[DB] Failed to open " << db_path << ": "
                      << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        std::cout << "[DB] Opened " << db_path << std::endl;
        return createTables();
    }

    void close() override {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    bool playerExists(const std::string& username) override {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT 1 FROM players WHERE username = ? LIMIT 1";

        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return exists;
    }

    int createPlayer(const std::string& username,
                     const std::string& password_hash,
                     int start_x, int start_y, int start_z) override {
        const char* sql = "INSERT INTO players "
                          "(username, password_hash, position_x, position_y, position_z, "
                          "skills, inventory, equipment, bank) "
                          "VALUES (?, ?, ?, ?, ?, '{}', '[]', '{}', '[]')";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "[DB] createPlayer prepare failed: "
                      << sqlite3_errmsg(db_) << std::endl;
            return 0;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, start_x);
        sqlite3_bind_int(stmt, 4, start_y);
        sqlite3_bind_int(stmt, 5, start_z);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "[DB] createPlayer failed: "
                      << sqlite3_errmsg(db_) << std::endl;
            sqlite3_finalize(stmt);
            return 0;
        }

        int id = static_cast<int>(sqlite3_last_insert_rowid(db_));
        sqlite3_finalize(stmt);
        std::cout << "[DB] Created player '" << username << "' (id=" << id << ")" << std::endl;
        return id;
    }

    bool savePlayer(int player_id,
                    int pos_x, int pos_y, int pos_z,
                    const std::string& skills_json,
                    const std::string& inventory_json,
                    const std::string& equipment_json,
                    const std::string& bank_json) override {
        const char* sql = "UPDATE players SET "
                          "position_x=?, position_y=?, position_z=?, "
                          "skills=?, inventory=?, equipment=?, bank=?, "
                          "last_save=datetime('now') "
                          "WHERE id=?";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, pos_x);
        sqlite3_bind_int(stmt, 2, pos_y);
        sqlite3_bind_int(stmt, 3, pos_z);
        sqlite3_bind_text(stmt, 4, skills_json.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, inventory_json.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, equipment_json.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, bank_json.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, player_id);

        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

private:
    bool createTables() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS players (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                position_x INTEGER DEFAULT 0,
                position_y INTEGER DEFAULT 0,
                position_z INTEGER DEFAULT 0,
                skills TEXT DEFAULT '{}',
                inventory TEXT DEFAULT '[]',
                equipment TEXT DEFAULT '{}',
                bank TEXT DEFAULT '[]',
                last_save TIMESTAMP,
                last_login TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )";

        char* err = nullptr;
        if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::cerr << "[DB] Table creation failed: " << err << std::endl;
            sqlite3_free(err);
            return false;
        }

        std::cout << "[DB] Tables ready" << std::endl;
        return true;
    }

    sqlite3* db_ = nullptr;
};

// Factory
std::unique_ptr<Database> createDatabase() {
    return std::make_unique<SqliteDatabase>();
}

} // namespace erynfall
