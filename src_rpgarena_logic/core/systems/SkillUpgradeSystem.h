#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include "core/stats/Stats.h"

class Player;

struct ScalingRule {
    bool hasPercent = false;
    double percentIncreasePerLevel = 0.0;

    bool hasLinear = false;
    double linearIncreasePerLevel = 0.0;
};

struct SkillUpgradeConfig {
    uint64_t baseCostBronze = 1000;
    double costMultiplier = 1.3;
    double defaultPercentIncrease = 10.0;

    ScalingRule damageFlatRule;
    ScalingRule stunDurationRule;
    ScalingRule buffDurationRule;

    std::unordered_map<std::string, ScalingRule> effectRules;
};

class SkillUpgradeSystem {
public:
    static SkillUpgradeSystem& getInstance() {
        static SkillUpgradeSystem instance;
        return instance;
    }

    bool loadConfig(const std::string& filepath = "assets/data/skill_upgrades.json");

    int getSkillLevel(int skillId) const;
    void setSkillLevel(int skillId, int level);

    uint64_t getUpgradeCost(int skillId) const;
    double getBonusPercent(int skillId) const;
    double getStatMultiplier(int skillId) const;

    bool canUpgrade(const Player* player, int skillId) const;
    bool upgradeSkill(Player* player, int skillId);

    int getScaledDamageFlat(int skillId, int baseDamage) const;
    float getScaledStunDuration(int skillId, float baseDuration) const;
    float getScaledBuffDuration(int skillId, float baseDuration) const;
    float getScaledEffectValue(int skillId, const std::string& statName, float baseValue) const;
    float getScaledEffectValue(int skillId, Stat stat, float baseValue) const;

    const SkillUpgradeConfig& getConfig(int skillId) const;

    std::string statToString(Stat stat) const;

private:
    SkillUpgradeSystem() = default;
    ~SkillUpgradeSystem() = default;
    SkillUpgradeSystem(const SkillUpgradeSystem&) = delete;
    SkillUpgradeSystem& operator=(const SkillUpgradeSystem&) = delete;

    float getScaledProperty(int skillId, float baseValue, const ScalingRule& rule) const;

    std::unordered_map<int, SkillUpgradeConfig> mConfigs;
    SkillUpgradeConfig mDefaultConfig;
    std::unordered_map<int, int> mSkillLevels;
    bool mLoaded = false;
};
