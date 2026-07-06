#include "game/skill_set.h"
#include <cstring>

namespace erynfall {

SkillSet::SkillSet() {
    skills_.fill({1, 0});
    // Hitpoints starts at level 10
    skills_[static_cast<int>(Skill::Hitpoints)] = {10, 1154}; // 10 * 10 = 1154 XP
}

bool SkillSet::addXP(Skill skill, uint32_t amount) {
    int idx = static_cast<int>(skill);
    if (idx < 0 || idx >= NUM_SKILLS) return false;

    uint8_t old_level = level(skill);
    skills_[idx].xp += amount;
    skills_[idx].level = levelFromXP(skills_[idx].xp);
    return skills_[idx].level > old_level;
}

const SkillState& SkillSet::get(Skill skill) const {
    return skills_[static_cast<int>(skill)];
}

SkillState& SkillSet::getMutable(Skill skill) {
    return skills_[static_cast<int>(skill)];
}

uint8_t SkillSet::level(Skill skill) const {
    return skills_[static_cast<int>(skill)].level;
}

uint32_t SkillSet::xp(Skill skill) const {
    return skills_[static_cast<int>(skill)].xp;
}

void SkillSet::serialize(uint8_t* buf) const {
    for (int i = 0; i < NUM_SKILLS; ++i) {
        int offset = i * 5;
        buf[offset + 0] = skills_[i].level;
        buf[offset + 1] = static_cast<uint8_t>(skills_[i].xp & 0xFF);
        buf[offset + 2] = static_cast<uint8_t>((skills_[i].xp >> 8) & 0xFF);
        buf[offset + 3] = static_cast<uint8_t>((skills_[i].xp >> 16) & 0xFF);
        buf[offset + 4] = static_cast<uint8_t>((skills_[i].xp >> 24) & 0xFF);
    }
}

void SkillSet::deserialize(const uint8_t* buf) {
    for (int i = 0; i < NUM_SKILLS; ++i) {
        int offset = i * 5;
        skills_[i].level = buf[offset + 0];
        skills_[i].xp = static_cast<uint32_t>(buf[offset + 1])
                      | (static_cast<uint32_t>(buf[offset + 2]) << 8)
                      | (static_cast<uint32_t>(buf[offset + 3]) << 16)
                      | (static_cast<uint32_t>(buf[offset + 4]) << 24);
    }
}

uint16_t SkillSet::totalLevel() const {
    uint16_t total = 0;
    for (int i = 0; i < NUM_SKILLS; ++i) {
        total += skills_[i].level;
    }
    return total;
}

} // namespace erynfall
