#pragma once

#include "types/skill.h"
#include "game_data/xp_table.h"
#include <array>
#include <cstdint>

namespace erynfall {

// Player skills — all 23 OSRS skills
class SkillSet {
public:
    SkillSet();

    // Add XP to a skill. Returns true if leveled up.
    bool addXP(Skill skill, uint32_t amount);

    // Get current state
    const SkillState& get(Skill skill) const;

    // Get level (calculated from XP)
    uint8_t level(Skill skill) const;

    // Get XP
    uint32_t xp(Skill skill) const;

    // Serialize all skills to buffer (for network sync)
    // Format: 23 * (uint8 level, uint32 xp) = 115 bytes
    void serialize(uint8_t* buf) const;

    // Deserialize from buffer
    void deserialize(const uint8_t* buf);

    // Total level (sum of all skills)
    uint16_t totalLevel() const;

    static constexpr int NUM_SKILLS = 23;

private:
    std::array<SkillState, NUM_SKILLS> skills_;
};

} // namespace erynfall
