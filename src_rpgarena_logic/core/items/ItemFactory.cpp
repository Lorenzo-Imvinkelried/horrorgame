#include "ItemFactory.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include "utils/Random.h"

float ItemFactory::getLevelMultiplier(int level) {
    return std::pow(1.07f, static_cast<float>(level - 1));
}

float ItemFactory::getQualityMultiplier(ItemQuality quality) {
    switch (quality) {
        case ItemQuality::Uncommon:  return 1.5f;
        case ItemQuality::Rare:      return 2.2f;
        case ItemQuality::Epic:      return 3.2f;
        case ItemQuality::Legendary: return 4.5f;
        case ItemQuality::Mythic:    return 6.0f;
        case ItemQuality::Godly:     return 8.0f;
        default:                     return 1.0f;
    }
}

ItemQuality ItemFactory::rollQuality(int level) {
    float roll = Random::Float(0.0f, 100.0f);
    
    // Chances shift based on level
    float godlyChance = 0.05f + (level * 0.01f);
    float mythicChance = 0.2f + (level * 0.02f);
    float legendaryChance = 1.0f + (level * 0.05f);
    float epicChance = 4.0f + (level * 0.1f);
    float rareChance = 12.0f + (level * 0.2f);
    float uncommonChance = 30.0f;
    
    if (roll < godlyChance) return ItemQuality::Godly;
    if (roll < godlyChance + mythicChance) return ItemQuality::Mythic;
    if (roll < godlyChance + mythicChance + legendaryChance) return ItemQuality::Legendary;
    if (roll < godlyChance + mythicChance + legendaryChance + epicChance) return ItemQuality::Epic;
    if (roll < godlyChance + mythicChance + legendaryChance + epicChance + rareChance) return ItemQuality::Rare;
    if (roll < godlyChance + mythicChance + legendaryChance + epicChance + rareChance + uncommonChance) return ItemQuality::Uncommon;
    
    return ItemQuality::Common;
}

void ItemFactory::scaleArmor(Item& item, int level, ItemQuality quality) {
    item.level = level;
    item.quality = quality;

    float levelMult = getLevelMultiplier(level);
    float qualityMult = getQualityMultiplier(quality);
    float totalMult = levelMult * qualityMult;
    float variance = Random::Float(0.95f, 1.05f);
    totalMult *= variance;

    auto applyMult = [&](int& val) {
        if (val <= 0) return;
        long long newVal = static_cast<long long>(val * totalMult);
        if (newVal > 2000000000) newVal = 2000000000;
        val = static_cast<int>(newVal);
    };

    // 1. Scale core defense & attributes
    applyMult(item.stats.defense);
    applyMult(item.stats.vitality);
    applyMult(item.stats.strength);
    applyMult(item.stats.agility);
    applyMult(item.stats.intelligence);
    applyMult(item.stats.hpBonus);
    applyMult(item.stats.mpBonus);

    // 2. Roll special defensive suffixes
    int specialStatsToRoll = 0;
    switch(quality) {
        case ItemQuality::Uncommon:  specialStatsToRoll = 1; break;
        case ItemQuality::Rare:      specialStatsToRoll = 1; break;
        case ItemQuality::Epic:      specialStatsToRoll = Random::Roll(50.0f) ? 2 : 1; break;
        case ItemQuality::Legendary: specialStatsToRoll = 2; break;
        case ItemQuality::Mythic:    specialStatsToRoll = Random::Roll(50.0f) ? 3 : 2; break;
        case ItemQuality::Godly:     specialStatsToRoll = 3; break;
        default:                     specialStatsToRoll = 0; break;
    }

    std::vector<int> availableCategories = {0, 1, 2, 3, 4, 5, 6, 7};
    for (size_t i = 0; i < availableCategories.size(); ++i) {
        size_t j = Random::Int(0, static_cast<int>(availableCategories.size() - 1));
        std::swap(availableCategories[i], availableCategories[j]);
    }

    std::string specialSuffix = "";
    int specialRolledCount = 0;

    for (int i = 0; i < specialStatsToRoll && i < (int)availableCategories.size(); ++i) {
        int category = availableCategories[i];
        if (category == 0) { // Damage Reduction
            item.stats.damageReduction = std::clamp(Random::Float(1.0f, 3.0f) * qualityMult * variance, 1.0f, 25.0f);
            specialSuffix += " de Resguardo";
            specialRolledCount++;
        }
        else if (category == 1) { // Tenacity
            item.stats.tenacity = std::clamp(Random::Float(2.0f, 5.0f) * qualityMult * variance, 2.0f, 40.0f);
            specialSuffix += " de Firmeza";
            specialRolledCount++;
        }
        else if (category == 2) { // Crit Avoidance
            item.stats.critAvoidance = std::clamp(Random::Float(2.0f, 6.0f) * qualityMult * variance, 2.0f, 30.0f);
            specialSuffix += " del Defensor";
            specialRolledCount++;
        }
        else if (category == 3) { // Evasion
            item.stats.evasion += static_cast<int>(Random::Float(10.0f, 30.0f) * totalMult);
            specialSuffix += " de Evasión";
            specialRolledCount++;
        }
        else if (category == 4) { // Thorns
            item.stats.thornsPercent = std::clamp(Random::Float(3.0f, 8.0f) * qualityMult * variance, 2.0f, 50.0f);
            specialSuffix += " de Espinas";
            specialRolledCount++;
        }
        else if (category == 5) { // HP Regen
            item.stats.hpRegenPercent = std::clamp(Random::Float(0.3f, 1.0f) * qualityMult * variance, 0.2f, 10.0f);
            specialSuffix += " de Regeneración";
            specialRolledCount++;
        }
        else if (category == 6) { // MP Regen
            item.stats.mpRegenPercent = std::clamp(Random::Float(0.3f, 1.0f) * qualityMult * variance, 0.2f, 10.0f);
            specialSuffix += " de Claridad";
            specialRolledCount++;
        }
        else if (category == 7) { // Block
            item.stats.blockChance = std::clamp(Random::Float(2.0f, 6.0f) * qualityMult * variance, 1.0f, 35.0f);
            item.stats.blockValuePercent = std::clamp(10.0f + Random::Float(5.0f, 10.0f) * qualityMult, 10.0f, 75.0f);
            specialSuffix += " de Bloqueo";
            specialRolledCount++;
        }
    }

    if (specialRolledCount > 0) {
        item.name += specialSuffix;
    }

    applyMult(item.stats.value);
    item.baseDefense = item.stats.defense;

    // Roll Sockets for Armor
    item.maxSockets = Random::Int(0, 3);
    item.socketedStones.resize(item.maxSockets, nullptr);
}

void ItemFactory::scaleRing(Item& item, int level, ItemQuality quality) {
    item.level = level;
    item.quality = quality;

    float levelMult = getLevelMultiplier(level);
    float qualityMult = getQualityMultiplier(quality);
    float totalMult = levelMult * qualityMult;
    float variance = Random::Float(0.95f, 1.05f);
    totalMult *= variance;

    auto applyMult = [&](int& val) {
        if (val <= 0) return;
        long long newVal = static_cast<long long>(val * totalMult * 1.25); // Jewelry attributes scale slightly better
        if (newVal > 2000000000) newVal = 2000000000;
        val = static_cast<int>(newVal);
    };

    // 1. Scale attributes
    applyMult(item.stats.strength);
    applyMult(item.stats.agility);
    applyMult(item.stats.intelligence);
    applyMult(item.stats.vitality);
    applyMult(item.stats.hpBonus);
    applyMult(item.stats.mpBonus);

    // 2. Roll special accessory suffixes
    int specialStatsToRoll = 0;
    switch(quality) {
        case ItemQuality::Uncommon:  specialStatsToRoll = 1; break;
        case ItemQuality::Rare:      specialStatsToRoll = 1; break;
        case ItemQuality::Epic:      specialStatsToRoll = Random::Roll(50.0f) ? 2 : 1; break;
        case ItemQuality::Legendary: specialStatsToRoll = 2; break;
        case ItemQuality::Mythic:    specialStatsToRoll = Random::Roll(50.0f) ? 3 : 2; break;
        case ItemQuality::Godly:     specialStatsToRoll = 3; break;
        default:                     specialStatsToRoll = 0; break;
    }

    std::vector<int> availableCategories = {0, 1, 2, 3, 4, 5};
    for (size_t i = 0; i < availableCategories.size(); ++i) {
        size_t j = Random::Int(0, static_cast<int>(availableCategories.size() - 1));
        std::swap(availableCategories[i], availableCategories[j]);
    }

    std::string specialSuffix = "";
    int specialRolledCount = 0;

    for (int i = 0; i < specialStatsToRoll && i < (int)availableCategories.size(); ++i) {
        int category = availableCategories[i];
        if (category == 0) { // Crit Chance
            item.stats.critChance = std::clamp(Random::Float(1.5f, 4.0f) * qualityMult * variance, 1.0f, 30.0f);
            specialSuffix += " del Ojo Certero";
            specialRolledCount++;
        }
        else if (category == 1) { // Crit Damage
            item.stats.critDamage = std::clamp(Random::Float(6.0f, 15.0f) * qualityMult * variance, 5.0f, 100.0f);
            specialSuffix += " de Cólera";
            specialRolledCount++;
        }
        else if (category == 2) { // Cooldown Reduction
            item.stats.cooldownReductionPercent = std::clamp(Random::Float(2.0f, 6.0f) * qualityMult * variance, 2.0f, 30.0f);
            specialSuffix += " de Rapidez Mágica";
            specialRolledCount++;
        }
        else if (category == 3) { // XP Bonus
            item.stats.xpBonusPercent = std::clamp(Random::Float(3.0f, 8.0f) * qualityMult * variance, 2.0f, 40.0f);
            specialSuffix += " del Sabio";
            specialRolledCount++;
        }
        else if (category == 4) { // Mana Steal
            item.stats.manaStealPercent = std::clamp(Random::Float(1.0f, 3.0f) * qualityMult * variance, 0.5f, 20.0f);
            specialSuffix += " de Drenaje";
            specialRolledCount++;
        }
        else if (category == 5) { // Move Speed
            item.stats.moveSpeedBonus = std::clamp(Random::Float(8.0f, 20.0f) * qualityMult * variance, 5.0f, 100.0f);
            specialSuffix += " de Impulso";
            specialRolledCount++;
        }
    }

    if (specialRolledCount > 0) {
        item.name += specialSuffix;
    }

    applyMult(item.stats.value);
}
