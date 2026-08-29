#include "WeaponsFactory.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include "utils/Random.h"
#include "utils/TinyJson.h"

#include <vector>

struct WeaponsConfig {
    bool loaded = false;
    std::vector<std::string> baseNames;
    std::map<std::string, std::string> statSuffixes;
    std::map<std::string, std::string> specialSuffixes;
    std::map<int, std::string> custom32x32Names;
};

static WeaponsConfig s_config;

void WeaponsFactory::ensureConfigLoaded() {
    if (s_config.loaded) return;
    
    json::Value root = json::parseFile("assets/data/weapons.json");
    if (root.type == json::Type::Object) {
        auto obj = root.asObject();
        
        // 1. Base Names
        if (obj.count("base_names") && obj.at("base_names").type == json::Type::Array) {
            auto arr = obj.at("base_names").asArray();
            s_config.baseNames.clear();
            for (const auto& v : arr) {
                s_config.baseNames.push_back(v.asString());
            }
        }
        
        // 2. Stat Suffixes
        if (obj.count("stat_suffixes") && obj.at("stat_suffixes").type == json::Type::Object) {
            auto sobj = obj.at("stat_suffixes").asObject();
            for (const auto& pair : sobj) {
                s_config.statSuffixes[pair.first] = pair.second.asString();
            }
        }
        
        // 3. Special Suffixes
        if (obj.count("special_suffixes") && obj.at("special_suffixes").type == json::Type::Object) {
            auto spobj = obj.at("special_suffixes").asObject();
            for (const auto& pair : spobj) {
                s_config.specialSuffixes[pair.first] = pair.second.asString();
            }
        }
        
        // 4. Custom 32x32 Names
        if (obj.count("custom_32x32_names") && obj.at("custom_32x32_names").type == json::Type::Object) {
            auto cobj = obj.at("custom_32x32_names").asObject();
            for (const auto& pair : cobj) {
                try {
                    int idx = std::stoi(pair.first);
                    s_config.custom32x32Names[idx] = pair.second.asString();
                } catch(...) {}
            }
        }
    }
    
    // Fallback if base names are empty
    if (s_config.baseNames.empty()) {
        s_config.baseNames = {"Espada", "Hacha", "Mazo", "Daga", "Lanza", "Báculo", "Estoque", "Cimitarra"};
    }
    
    // Fallback if stat suffixes are empty
    if (s_config.statSuffixes.empty()) {
        s_config.statSuffixes["strength"] = " de str";
        s_config.statSuffixes["agility"] = " de agi";
        s_config.statSuffixes["intelligence"] = " de int";
        s_config.statSuffixes["vitality"] = " de vit";
    }
    
    // Fallback if special suffixes are empty
    if (s_config.specialSuffixes.empty()) {
        s_config.specialSuffixes["lifesteal"] = " vampírico";
        s_config.specialSuffixes["stun"] = " aturdidor";
        s_config.specialSuffixes["bleed"] = " desgarrador";
        s_config.specialSuffixes["slow"] = " gélido";
        s_config.specialSuffixes["aoe"] = " de hendidura";
        s_config.specialSuffixes["attack_speed"] = " de rapidez";
        s_config.specialSuffixes["crit_chance"] = " crítico";
        s_config.specialSuffixes["crit_damage"] = " devastador";
        s_config.specialSuffixes["move_speed"] = " veloz";
        s_config.specialSuffixes["cooldown_reduction"] = " de concentración";
    }
    
    s_config.loaded = true;
}

const std::vector<std::string>& WeaponsFactory::getBaseNames() {
    ensureConfigLoaded();
    return s_config.baseNames;
}

std::string WeaponsFactory::getCustom32x32Name(int index) {
    ensureConfigLoaded();
    if (s_config.custom32x32Names.count(index)) {
        return s_config.custom32x32Names.at(index);
    }
    return (index == 1) ? "Gran Espada" : "Martillo de Guerra";
}

void WeaponsFactory::scaleWeapon(Item& item, int level, ItemQuality quality) {
    if (item.type != ItemType::Weapon) return; 

    item.level = level;
    item.quality = quality;

    // 1. Calcular Multiplicadores
    // Fórmula: Multiplier = pow(1.07, level - 1)
    double levelMultiplier = std::pow(1.07, level - 1); 

    // Quality Multiplier
    float qualityMult = getQualityMultiplier(quality);

    // Total Multiplier
    double totalMult = levelMultiplier * (double)qualityMult;
    
    // [VARIANCE] Add +/- 5% randomness
    float variance = Random::Float(0.95f, 1.05f); 
    
    totalMult *= variance;

    // Helper para aplicar multiplicador con seguridad
    auto applyMult = [&](int& val) {
        if (val <= 0) return; 
        long long newVal = static_cast<long long>(val * totalMult);
        if (newVal > 2000000000) newVal = 2000000000; 
        val = static_cast<int>(newVal);
    };

    // 2. Escalar Daño
    applyMult(item.stats.physicalDamage);

    // 3. Escalar Stats (STR, DEX, INT, VIT)
    // Base Stats = (Level * 0.75) * QualityMult
    float statBase = std::max(1.f, (float)level * 0.75f);
    statBase *= qualityMult;
    // Variance applies to base as well
    statBase *= variance;

    auto scaleStat = [&](int& statVal) {
        if (statVal > 0) {
             applyMult(statVal);
        } else {
             // [MODIFIED] Randomize distribution (50% - 150%)
             float randomFactor = Random::Float(0.5f, 1.5f);
             statVal = static_cast<int>(statBase * randomFactor);
        }
    };

    scaleStat(item.stats.strength);
    scaleStat(item.stats.agility);
    scaleStat(item.stats.intelligence);
    scaleStat(item.stats.vitality);

    // 4. Roll Special Stats based on Quality
    int specialStatsToRoll = 0;
    switch(quality) {
        case ItemQuality::Uncommon:
            specialStatsToRoll = 1;
            break;
        case ItemQuality::Rare:
            specialStatsToRoll = 1;
            break;
        case ItemQuality::Epic:
            specialStatsToRoll = Random::Roll(50.0f) ? 2 : 1;
            break;
        case ItemQuality::Legendary:
            specialStatsToRoll = 2;
            break;
        case ItemQuality::Mythic:
            specialStatsToRoll = Random::Roll(50.0f) ? 3 : 2;
            break;
        case ItemQuality::Godly:
            specialStatsToRoll = 3;
            break;
        default:
            specialStatsToRoll = 0;
            break;
    }

    std::vector<int> availableCategories = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    // Shuffle categories manually to pick unique ones
    for (size_t i = 0; i < availableCategories.size(); ++i) {
        size_t j = Random::Int(0, static_cast<int>(availableCategories.size() - 1));
        std::swap(availableCategories[i], availableCategories[j]);
    }

    std::string specialSuffix = "";
    int specialRolledCount = 0;

    ensureConfigLoaded();

    for (int i = 0; i < specialStatsToRoll && i < static_cast<int>(availableCategories.size()); ++i) {
        int category = availableCategories[i];
        if (category == 0) { // Lifesteal
            item.stats.lifestealPercent = std::clamp(Random::Float(1.5f, 3.5f) * qualityMult * variance, 2.0f, 40.0f);
            specialSuffix += s_config.specialSuffixes["lifesteal"];
            specialRolledCount++;
        }
        else if (category == 1) { // Stun
            item.stats.stunChance = std::clamp(Random::Float(3.0f, 6.0f) * qualityMult * variance, 2.0f, 35.0f);
            item.stats.stunDuration = Random::Float(0.5f, 1.0f) * (1.0f + 0.15f * qualityMult);
            specialSuffix += s_config.specialSuffixes["stun"];
            specialRolledCount++;
        }
        else if (category == 2) { // Bleed
            if (Random::Roll(50.0f)) {
                // Flat bleed
                item.stats.bleedFlat = static_cast<int>(std::max(1.f, Random::Float(3.f, 8.f) * (float)totalMult));
                item.stats.bleedDurationFlat = Random::Float(3.0f, 5.0f);
            } else {
                // Percent bleed
                item.stats.bleedPercent = std::clamp(Random::Float(2.0f, 5.0f) * qualityMult * variance, 2.0f, 40.0f);
                item.stats.bleedDurationPercent = Random::Float(3.0f, 5.0f);
            }
            specialSuffix += s_config.specialSuffixes["bleed"];
            specialRolledCount++;
        }
        else if (category == 3) { // Slow
            if (Random::Roll(50.0f)) {
                item.stats.slowMovePercent = std::clamp(Random::Float(10.0f, 20.0f) * qualityMult * variance, 10.0f, 60.0f);
                item.stats.slowMoveDuration = Random::Float(2.5f, 4.5f);
            } else {
                item.stats.slowAttackPercent = std::clamp(Random::Float(8.0f, 15.0f) * qualityMult * variance, 8.0f, 50.0f);
                item.stats.slowAttackDuration = Random::Float(2.5f, 4.5f);
            }
            specialSuffix += s_config.specialSuffixes["slow"];
            specialRolledCount++;
        }
        else if (category == 4) { // AoE
            item.stats.aoeRadius = std::clamp(Random::Float(45.0f, 65.0f) * (1.0f + 0.12f * qualityMult), 40.0f, 150.0f);
            item.stats.aoeDamagePercent = std::clamp(Random::Float(15.0f, 30.0f) * qualityMult * variance, 10.0f, 80.0f);
            specialSuffix += s_config.specialSuffixes["aoe"];
            specialRolledCount++;
        }
        else if (category == 5) { // Attack Speed
            item.stats.attackSpeed += std::clamp(Random::Float(0.08f, 0.20f) * qualityMult * variance, 0.05f, 1.0f);
            specialSuffix += s_config.specialSuffixes["attack_speed"];
            specialRolledCount++;
        }
        else if (category == 6) { // Crit Chance
            item.stats.critChance += std::clamp(Random::Float(3.0f, 8.0f) * qualityMult * variance, 2.0f, 50.0f);
            specialSuffix += s_config.specialSuffixes["crit_chance"];
            specialRolledCount++;
        }
        else if (category == 7) { // Crit Damage
            item.stats.critDamage += std::clamp(Random::Float(10.0f, 25.0f) * qualityMult * variance, 5.0f, 150.0f);
            specialSuffix += s_config.specialSuffixes["crit_damage"];
            specialRolledCount++;
        }
        else if (category == 8) { // Move Speed
            item.stats.moveSpeedBonus += std::clamp(Random::Float(10.f, 25.f) * qualityMult * variance, 5.f, 150.f);
            specialSuffix += s_config.specialSuffixes["move_speed"];
            specialRolledCount++;
        }
        else if (category == 9) { // Cooldown Reduction
            item.stats.cooldownReductionPercent = std::clamp(Random::Float(4.0f, 10.0f) * qualityMult * variance, 2.0f, 50.0f);
            specialSuffix += s_config.specialSuffixes["cooldown_reduction"];
            specialRolledCount++;
        }
    }

    // 5. Dynamic naming suffix
    std::string statSuffix = "";
    int maxStat = std::max({item.stats.strength, item.stats.agility, item.stats.intelligence, item.stats.vitality});
    if (maxStat > 0) {
        if (maxStat == item.stats.strength)          statSuffix = s_config.statSuffixes["strength"];
        else if (maxStat == item.stats.agility)    statSuffix = s_config.statSuffixes["agility"];
        else if (maxStat == item.stats.intelligence) statSuffix = s_config.statSuffixes["intelligence"];
        else if (maxStat == item.stats.vitality)     statSuffix = s_config.statSuffixes["vitality"];
    }

    if (!statSuffix.empty()) {
        item.name += statSuffix;
    }

    if (specialRolledCount > 0) {
        item.name += specialSuffix;
    }

    // Value (Gold) scaling
    applyMult(item.stats.value);

    // Initialize base stats for fortification display
    item.basePhysicalDamage = item.stats.physicalDamage;

    // Roll Sockets
    if (item.gripType == GripType::OneHanded) {
        item.maxSockets = Random::Int(0, 3);
    } else {
        item.maxSockets = Random::Int(0, 6);
    }
    item.socketedStones.resize(item.maxSockets, nullptr);
}

float WeaponsFactory::getQualityMultiplier(ItemQuality quality) {
    switch(quality) {
        case ItemQuality::Uncommon:  return 1.5f;
        case ItemQuality::Rare:      return 2.2f;
        case ItemQuality::Epic:      return 3.0f;
        case ItemQuality::Legendary: return 4.0f;
        case ItemQuality::Mythic:    return 4.5f;
        case ItemQuality::Godly:     return 5.0f;
        default:                     return 1.0f; // Common
    }
}

ItemQuality WeaponsFactory::rollQuality(int level) {
    RarityChances chances;

    // 1. Calcular las chances actuales basadas en el level
    float t = (float)level / 100.0f; // Normalized 0..1
    float t2 = t * t;

    // Godly: Requires level 75+. Starts at 0.001% at level 75, scales to 0.01% at level 100.
    if (level >= 75) {
        chances.godly = 0.001f + (float)(level - 75) * 0.00036f;
    } else {
        chances.godly = 0.0f;
    }

    // Mythic: Requires level 50+. Starts at 0.01% at level 50, scales up to 0.10% at level 100.
    if (level >= 50) {
        chances.mythic = 0.01f + t2 * 0.09f;
    } else {
        chances.mythic = 0.0f;
    }

    // Legendary: Requires level 25+. Scales quadratically. Target 0.50% at level 100.
    if (level >= 25) {
        chances.legendary = t2 * 0.5f;
    } else {
        chances.legendary = 0.0f;
    }

    // Epic: Requires level 10+. Scales power-1.5. Target 2.00% at level 100.
    if (level >= 10) {
        chances.epic = std::pow(t, 1.5f) * 2.0f;
    } else {
        chances.epic = 0.0f;
    }

    // Rare: Requires level 4+. Scales linearly. Target 8.00% at level 100.
    if (level >= 4) {
        chances.rare = (float)level * 0.08f;
    } else {
        chances.rare = 0.0f;
    }

    // Uncommon: No gate. Scales linearly. Target 25.00% at level 100.
    chances.uncommon = 5.0f + (float)level * 0.20f;

    // 2. Chequeos secuenciales (Highest -> Lowest)
    if (Random::Roll(chances.godly))     return ItemQuality::Godly;
    if (Random::Roll(chances.mythic))    return ItemQuality::Mythic;
    if (Random::Roll(chances.legendary)) return ItemQuality::Legendary;
    if (Random::Roll(chances.epic))      return ItemQuality::Epic;
    if (Random::Roll(chances.rare))      return ItemQuality::Rare;
    if (Random::Roll(chances.uncommon))  return ItemQuality::Uncommon;

    // 3. Fallback
    return ItemQuality::Common;
}

