#include "core/game_loop.h"
#include <thread>

namespace erynfall {

GameLoop::GameLoop(int tick_rate_ms)
    : tick_rate_ms_(tick_rate_ms) {}

void GameLoop::setOnTick(TickCallback callback) {
    on_tick_ = std::move(callback);
}

void GameLoop::run() {
    running_.store(true);
    tick_count_.store(0);

    // Catch up time to maintain consistent tick rate.
    // Each tick targets exactly tick_rate_ms_ milliseconds.
    auto tick_duration = std::chrono::milliseconds(tick_rate_ms_);

    while (running_.load()) {
        auto tick_start = std::chrono::steady_clock::now();

        tick();

        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(tick_end - tick_start);
        auto remaining = tick_duration - elapsed;

        if (remaining.count() > 0) {
            std::this_thread::sleep_for(remaining);
        }
        // If tick took longer than tick_rate_ms_, we start the next one immediately
        // (tick slip — OSRS does this too)
    }
}

void GameLoop::stop() {
    running_.store(false);
}

void GameLoop::tick() {
    uint64_t current = tick_count_.fetch_add(1);
    if (on_tick_) {
        on_tick_(current);
    }
}

} // namespace erynfall
