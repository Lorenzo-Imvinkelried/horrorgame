#include "SkillUpgradeSystem.h"
#include "utils/TinyJson.h"
#include "entities/player/Player.h"
#include <iostream>
#include <cmath>
#include <algorithm>

static ScalingRule parseScalingRule(const json::Value& val) {
    ScalingRule rule;
    if (val.type == json::Type::Object) {
        const auto& obj = val.asObject();
        if (obj.count("percent_increase_per_level")) {
            rule.hasPercent = true;
            rule.percentIncreasePerLevel = obj.at("percent_increase_per_level").asDouble();
        }
        if (obj.count("linear_increase_per_level")) {
            rule.hasLinear = true;
            rule.linearIncreasePerLevel = obj.at("linear_increase_per_level").asDouble();
        }
    } else if (val.type == json::Type::Number) {
        rule.hasPercent = true;
        rule.percentIncreasePerLevel = val.asDouble();
    }
    return rule;
}

bool SkillUpgradeSystem::loadConfig(const std::string& filepath) {
    if (mLoaded) return true;

    json::Value root = json::parseFile(filepath);
    if (root.type != json::Type::Object) {
        std::cerr << "[SkillUpgradeSystem] WARNING: No se pudo cargar " << filepath << "\n";
        return false;
    }

    auto rootObj = root.asObject();

    // Parse default
    if (rootObj.count("default") && rootObj.at("default").type == json::Type::Object) {
        auto defObj = rootObj.at("default").asObject();
        if (defObj.count("base_cost_bronze")) mDefaultConfig.baseCostBronze = static_cast<uint64_t>(defObj.at("base_cost_bronze").asInt());
        if (defObj.count("cost_multiplier_per_level")) mDefaultConfig.costMultiplier = defObj.at("cost_multiplier_per_level").asDouble();
        if (defObj.count("percent_increase_per_level")) mDefaultConfig.defaultPercentIncrease = defObj.at("percent_increase_per_level").asDouble();
    }

    // Parse per skill upgrades
    if (rootObj.count("upgrades") && rootObj.at("upgrades").type == json::Type::Object) {
        for (const auto& pair : rootObj.at("upgrades").asObject()) {
            try {
                int skillId = std::stoi(pair.first);
                if (pair.second.type == json::Type::Object) {
                    auto obj = pair.second.asObject();
                    SkillUpgradeConfig cfg = mDefaultConfig;
                    if (obj.count("base_cost_bronze")) cfg.baseCostBronze = static_cast<uint64_t>(obj.at("base_cost_bronze").asInt());
                    if (obj.count("cost_multiplier_per_level")) cfg.costMultiplier = obj.at("cost_multiplier_per_level").asDouble();
                    if (obj.count("percent_increase_per_level")) cfg.defaultPercentIncrease = obj.at("percent_increase_per_level").asDouble();

                    if (obj.count("damage_flat")) cfg.damageFlatRule = parseScalingRule(obj.at("damage_flat"));
                    if (obj.count("stun_duration")) cfg.stunDurationRule = parseScalingRule(obj.at("stun_duration"));
                    if (obj.count("buff_duration")) cfg.buffDurationRule = parseScalingRule(obj.at("buff_duration"));

                    if (obj.count("effects") && obj.at("effects").type == json::Type::Object) {
                        for (const auto& effPair : obj.at("effects").asObject()) {
                            std::string statKey = effPair.first;
                            std::transform(statKey.begin(), statKey.end(), statKey.begin(), ::toupper);
                            cfg.effectRules[statKey] = parseScalingRule(effPair.second);
                        }
                    }

                    mConfigs[skillId] = cfg;
                }
            } catch (...) {
                std::cerr << "[SkillUpgradeSystem] Error parsing skill ID: " << pair.first << "\n";
            }
        }
    }

    mLoaded = true;
    std::cout << "[SkillUpgradeSystem] Configuracion de mejoras cargada (" << mConfigs.size() << " habilidades configuradas).\n";
    return true;
}

int SkillUpgradeSystem::getSkillLevel(int skillId) const {
    auto it = mSkillLevels.find(skillId);
    if (it != mSkillLevels.end()) {
        return std::max(1, it->second);
    }
    return 1;
}

void SkillUpgradeSystem::setSkillLevel(int skillId, int level) {
    mSkillLevels[skillId] = std::max(1, level);
}

const SkillUpgradeConfig& SkillUpgradeSystem::getConfig(int skillId) const {
    auto it = mConfigs.find(skillId);
    if (it != mConfigs.end()) {
        return it->second;
    }
    return mDefaultConfig;
}

uint64_t SkillUpgradeSystem::getUpgradeCost(int skillId) const {
    int lvl = getSkillLevel(skillId);
    const auto& cfg = getConfig(skillId);
    double cost = static_cast<double>(cfg.baseCostBronze) * std::pow(cfg.costMultiplier, static_cast<double>(lvl - 1));
    return static_cast<uint64_t>(std::max(1.0, std::floor(cost)));
}

double SkillUpgradeSystem::getBonusPercent(int skillId) const {
    int lvl = getSkillLevel(skillId);
    const auto& cfg = getConfig(skillId);
    return cfg.defaultPercentIncrease * static_cast<double>(lvl - 1);
}

double SkillUpgradeSystem::getStatMultiplier(int skillId) const {
    return 1.0 + (getBonusPercent(skillId) / 100.0);
}

bool SkillUpgradeSystem::canUpgrade(const Player* player, int skillId) const {
    if (!player) return false;
    uint64_t cost = getUpgradeCost(skillId);
    return player->getBronzeCoins() >= cost;
}

bool SkillUpgradeSystem::upgradeSkill(Player* player, int skillId) {
    if (!player) return false;
    uint64_t cost = getUpgradeCost(skillId);
    if (player->getBronzeCoins() < cost) return false;

    if (player->removeBronzeCoins(cost)) {
        int currentLvl = getSkillLevel(skillId);
        setSkillLevel(skillId, currentLvl + 1);
        std::cout << "[SkillUpgradeSystem] Skill " << skillId << " subida a nivel " << (currentLvl + 1) << " por " << cost << " bronce.\n";
        return true;
    }
    return false;
}

float SkillUpgradeSystem::getScaledProperty(int skillId, float baseValue, const ScalingRule& rule) const {
    if (baseValue == 0.0f) return 0.0f;
    int level = getSkillLevel(skillId);
    if (level <= 1) return baseValue;

    int extraLevels = level - 1;

    if (rule.hasLinear) {
        return baseValue + static_cast<float>(rule.linearIncreasePerLevel * extraLevels);
    } else if (rule.hasPercent) {
        double mult = 1.0 + (rule.percentIncreasePerLevel * extraLevels / 100.0);
        return static_cast<float>(baseValue * mult);
    } else {
        double defaultPercent = getConfig(skillId).defaultPercentIncrease;
        double mult = 1.0 + (defaultPercent * extraLevels / 100.0);
        return static_cast<float>(baseValue * mult);
    }
}

int SkillUpgradeSystem::getScaledDamageFlat(int skillId, int baseDamage) const {
    if (baseDamage == 0) return 0;
    const auto& cfg = getConfig(skillId);
    float scaled = getScaledProperty(skillId, static_cast<float>(baseDamage), cfg.damageFlatRule);
    return static_cast<int>(std::round(scaled));
}

float SkillUpgradeSystem::getScaledStunDuration(int skillId, float baseDuration) const {
    if (baseDuration <= 0.0f) return 0.0f;
    const auto& cfg = getConfig(skillId);
    float scaled = getScaledProperty(skillId, baseDuration, cfg.stunDurationRule);
    return std::round(scaled * 100.0f) / 100.0f;
}

float SkillUpgradeSystem::getScaledBuffDuration(int skillId, float baseDuration) const {
    if (baseDuration <= 0.0f) return 0.0f;
    const auto& cfg = getConfig(skillId);
    float scaled = getScaledProperty(skillId, baseDuration, cfg.buffDurationRule);
    return std::round(scaled * 100.0f) / 100.0f;
}

std::string SkillUpgradeSystem::statToString(Stat stat) const {
    switch (stat) {
    case Stat::STR: return "STR";
    case Stat::DEX: return "AGI";
    case Stat::INT: return "INT";
    case Stat::VIT: return "VIT";
    case Stat::ATTACK: return "ATTACK";
    case Stat::DEFENSE: return "DEFENSE";
    case Stat::MAX_HP: return "MAX_HP";
    case Stat::MAX_MP: return "MAX_MP";
    case Stat::ATK_SPEED: return "ATK_SPEED";
    case Stat::MOVE_SPEED: return "MOVE_SPEED";
    case Stat::PHYSICAL_DAMAGE_BONUS: return "PHYSICAL_DAMAGE_BONUS";
    case Stat::CRIT_CHANCE: return "CRIT_CHANCE";
    case Stat::CRIT_DMG: return "CRIT_DMG";
    case Stat::LIFESTEAL: return "LIFESTEAL";
    case Stat::COOLDOWN_REDUCTION: return "COOLDOWN_REDUCTION";
    case Stat::TENACITY: return "TENACITY";
    case Stat::ACCURACY: return "ACCURACY";
    case Stat::EVASION: return "EVASION";
    default: return "UNKNOWN";
    }
}

float SkillUpgradeSystem::getScaledEffectValue(int skillId, const std::string& statName, float baseValue) const {
    if (baseValue == 0.0f) return 0.0f;
    const auto& cfg = getConfig(skillId);
    std::string uKey = statName;
    std::transform(uKey.begin(), uKey.end(), uKey.begin(), ::toupper);

    auto it = cfg.effectRules.find(uKey);
    ScalingRule rule;
    if (it != cfg.effectRules.end()) {
        rule = it->second;
    }
    float scaled = getScaledProperty(skillId, baseValue, rule);
    if (std::abs(baseValue) >= 1.0f) {
        return static_cast<float>(std::round(scaled));
    } else {
        return static_cast<float>(std::round(scaled * 100.0) / 100.0);
    }
}

float SkillUpgradeSystem::getScaledEffectValue(int skillId, Stat stat, float baseValue) const {
    return getScaledEffectValue(skillId, statToString(stat), baseValue);
}
