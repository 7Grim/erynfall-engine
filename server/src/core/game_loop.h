#pragma once

#include <chrono>
#include <functional>
#include <atomic>

namespace erynfall {

class GameLoop {
public:
    using TickCallback = std::function<void(uint64_t tick_number)>;

    explicit GameLoop(int tick_rate_ms = 600);

    // Set the callback invoked every tick
    void setOnTick(TickCallback callback);

    // Run the game loop (blocks until stop() is called)
    void run();

    // Stop the loop (signal from another thread or signal handler)
    void stop();

    // Current tick number
    uint64_t currentTick() const { return tick_count_.load(); }

    // Whether the loop is running
    bool running() const { return running_.load(); }

private:
    void tick();

    int tick_rate_ms_;
    std::atomic<uint64_t> tick_count_{0};
    std::atomic<bool> running_{false};
    TickCallback on_tick_;
};

} // namespace erynfall
