#include "StoneSystem.h"
#include "../items/WeaponsFactory.h" // For multipliers
#include "utils/Random.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

void StoneSystem::scaleStone(Item& item, int level, ItemQuality quality) {
    if (item.type != ItemType::Stone) return;
    
    item.level = level;
    item.quality = quality;
    
    double levelMultiplier = std::pow(1.07, level - 1);
    float qualityMult = WeaponsFactory::getQualityMultiplier(quality);
    double totalMult = levelMultiplier * (double)qualityMult;
    float variance = Random::Float(0.95f, 1.05f);
    totalMult *= variance;
    
    auto scaleStat = [&](int& val) {
        if (val <= 0) return;
        val = std::max(1, static_cast<int>(val * totalMult));
    };
    
    auto scaleFloatStat = [&](float& val) {
        if (val <= 0.001f) return;
        val = val * totalMult;
    };
    
    scaleStat(item.stats.strength);
    scaleStat(item.stats.agility);
    scaleStat(item.stats.intelligence);
    scaleStat(item.stats.vitality);
    scaleStat(item.stats.hpBonus);
    scaleStat(item.stats.mpBonus);
    scaleStat(item.stats.physicalDamage);
    scaleStat(item.stats.defense);
    
    scaleFloatStat(item.stats.critChance);
    scaleFloatStat(item.stats.critDamage);
    scaleFloatStat(item.stats.lifestealPercent);
    scaleFloatStat(item.stats.armorPenetration);
    scaleFloatStat(item.stats.cooldownReductionPercent);

    scaleFloatStat(item.stats.attackPercent);
    scaleFloatStat(item.stats.defensePercent);
    scaleFloatStat(item.stats.hpPercent);
    scaleFloatStat(item.stats.mpPercent);
    scaleFloatStat(item.stats.strengthPercent);
    scaleFloatStat(item.stats.agilityPercent);
    scaleFloatStat(item.stats.intelligencePercent);
    scaleFloatStat(item.stats.vitalityPercent);
    scaleFloatStat(item.stats.physicalDamageBonus);

    // Roll secondary bonus affixes for higher quality procedural stones
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

    if (specialStatsToRoll > 0) {
        std::vector<int> availableCategories = {0, 1, 2, 3, 4, 5, 6, 7};
        for (size_t i = 0; i < availableCategories.size(); ++i) {
            size_t j = Random::Int(0, static_cast<int>(availableCategories.size() - 1));
            std::swap(availableCategories[i], availableCategories[j]);
        }

        std::string specialSuffix = "";
        int specialRolledCount = 0;

        for (int i = 0; i < specialStatsToRoll && i < (int)availableCategories.size(); ++i) {
            int cat = availableCategories[i];
            if (cat == 0) {
                item.stats.attackPercent += std::clamp(Random::Float(2.0f, 5.0f) * qualityMult * variance, 2.0f, 30.0f);
                specialSuffix += " del Devastador";
                specialRolledCount++;
            } else if (cat == 1) {
                item.stats.defensePercent += std::clamp(Random::Float(2.0f, 5.0f) * qualityMult * variance, 2.0f, 30.0f);
                specialSuffix += " de Resguardo";
                specialRolledCount++;
            } else if (cat == 2) {
                item.stats.physicalDamageBonus += std::clamp(Random::Float(3.0f, 8.0f) * qualityMult * variance, 3.0f, 40.0f);
                specialSuffix += " de Cólera";
                specialRolledCount++;
            } else if (cat == 3) {
                item.stats.critChance += std::clamp(Random::Float(1.5f, 4.0f) * qualityMult * variance, 1.0f, 25.0f);
                specialSuffix += " del Ojo Certero";
                specialRolledCount++;
            } else if (cat == 4) {
                item.stats.critDamage += std::clamp(Random::Float(5.0f, 12.0f) * qualityMult * variance, 5.0f, 60.0f);
                specialSuffix += " de Furia";
                specialRolledCount++;
            } else if (cat == 5) {
                item.stats.hpPercent += std::clamp(Random::Float(3.0f, 7.0f) * qualityMult * variance, 2.0f, 35.0f);
                specialSuffix += " del Titán";
                specialRolledCount++;
            } else if (cat == 6) {
                item.stats.lifestealPercent += std::clamp(Random::Float(1.0f, 3.0f) * qualityMult * variance, 1.0f, 20.0f);
                specialSuffix += " de Sangre";
                specialRolledCount++;
            } else if (cat == 7) {
                item.stats.cooldownReductionPercent += std::clamp(Random::Float(2.0f, 5.0f) * qualityMult * variance, 2.0f, 25.0f);
                specialSuffix += " de Rapidez";
                specialRolledCount++;
            }
        }

        if (specialRolledCount > 0) {
            item.name += specialSuffix;
        }
    }
    
    item.stats.value = std::max(5, static_cast<int>(5 * totalMult));
}
