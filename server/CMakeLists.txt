cmake_minimum_required(VERSION 3.20)
project(erynfall-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Shared library (protocol, constants, types, game data)
add_library(erynfall-shared STATIC
    ${CMAKE_SOURCE_DIR}/../shared/protocol/buffer.cpp
)
target_include_directories(erynfall-shared PUBLIC ${CMAKE_SOURCE_DIR}/../shared)

# Server executable
add_executable(erynfall-server
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/core/game_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/net/tcp_server.cpp
    ${CMAKE_SOURCE_DIR}/src/world/world_map.cpp
    ${CMAKE_SOURCE_DIR}/src/world/world_objects.cpp
    ${CMAKE_SOURCE_DIR}/src/world/npc_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/world/ground_items.cpp
    ${CMAKE_SOURCE_DIR}/src/db/sqlite_db.cpp
    ${CMAKE_SOURCE_DIR}/src/game/inventory.cpp
    ${CMAKE_SOURCE_DIR}/src/game/skill_set.cpp
    ${CMAKE_SOURCE_DIR}/src/game/equipment.cpp
)

target_include_directories(erynfall-server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../shared
)

target_link_libraries(erynfall-server
    PRIVATE erynfall-shared SQLite::SQLite3 Threads::Threads
)
