cmake_minimum_required(VERSION 3.20)
project(erynfall-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Shared library (protocol, constants, types)
add_library(erynfall-shared STATIC
    ${CMAKE_SOURCE_DIR}/../shared/protocol/buffer.cpp
)
target_include_directories(erynfall-shared PUBLIC ${CMAKE_SOURCE_DIR}/../shared)

# Server executable
file(GLOB_RECURSE SERVER_SOURCES src/*.cpp)
add_executable(erynfall-server ${SERVER_SOURCES})

target_link_libraries(erynfall-server
    PRIVATE
        erynfall-shared
        SQLite::SQLite3
        Threads::Threads
)

target_include_directories(erynfall-server PRIVATE src)
