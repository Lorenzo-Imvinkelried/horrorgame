#include "CultivoSystem.h"
#include "../engine/ResourceManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

CultivoSystem& CultivoSystem::getInstance() {
    static CultivoSystem instance;
    return instance;
}

void CultivoSystem::setCultivatedItem(std::shared_ptr<Item> item) {
    mCultivatedItem = item;
}

int CultivoSystem::getExpForNextLevel(int level) {
    if (level < 1) level = 1;
    return 100 + (level * level * 50);
}

bool CultivoSystem::addExp(int amount) {
    if (!mCultivatedItem || !mCultivatedItem->cultivoLocked || amount <= 0) return false;

    mCultivatedItem->cultivoExp += amount;

    int expReq = getExpForNextLevel(mCultivatedItem->cultivoLevel + 1);
    bool leveledUp = false;

    while (mCultivatedItem->cultivoExp >= expReq) {
        mCultivatedItem->cultivoExp -= expReq;
        mCultivatedItem->cultivoLevel++;
        leveledUp = true;
        recalculateCultivoBonusStats(*mCultivatedItem);
        expReq = getExpForNextLevel(mCultivatedItem->cultivoLevel + 1);
    }

    if (leveledUp) {
        std::cout << "[CULTIVO] Item " << mCultivatedItem->name 
                  << " subió al Nivel de Cultivo " << mCultivatedItem->cultivoLevel << "!\n";
    }

    return leveledUp;
}

void CultivoSystem::recalculateCultivoBonusStats(Item& item) {
    item.cultivoBonusStats = ItemStats{}; // Reset

    if (item.cultivoLevel <= 0 || item.cultivoSelectedStats.empty()) return;

    size_t numStats = item.cultivoSelectedStats.size();
    float growthPerLevel = 6.0f / static_cast<float>(numStats); // Growth points divided among selected stats
    float totalGrowth = growthPerLevel * item.cultivoLevel;

    for (const auto& statId : item.cultivoSelectedStats) {
        if (statId == "STR" || statId == "strength") {
            item.cultivoBonusStats.strength = std::max(1, static_cast<int>(std::round(totalGrowth * 1.5f)));
        } else if (statId == "AGI" || statId == "agility") {
            item.cultivoBonusStats.agility = std::max(1, static_cast<int>(std::round(totalGrowth * 1.5f)));
        } else if (statId == "INT" || statId == "intelligence") {
            item.cultivoBonusStats.intelligence = std::max(1, static_cast<int>(std::round(totalGrowth * 1.5f)));
        } else if (statId == "VIT" || statId == "vitality") {
            item.cultivoBonusStats.vitality = std::max(1, static_cast<int>(std::round(totalGrowth * 1.5f)));
        } else if (statId == "physicalDamage" || statId == "attack") {
            item.cultivoBonusStats.physicalDamage = std::max(1, static_cast<int>(std::round(totalGrowth * 2.0f)));
        } else if (statId == "defense") {
            item.cultivoBonusStats.defense = std::max(1, static_cast<int>(std::round(totalGrowth * 1.8f)));
        } else if (statId == "hpBonus" || statId == "hp") {
            item.cultivoBonusStats.hpBonus = std::max(5, static_cast<int>(std::round(totalGrowth * 15.0f)));
        } else if (statId == "mpBonus" || statId == "mp") {
            item.cultivoBonusStats.mpBonus = std::max(5, static_cast<int>(std::round(totalGrowth * 12.0f)));
        } else if (statId == "critChance") {
            item.cultivoBonusStats.critChance = totalGrowth * 0.4f;
        } else if (statId == "critDamage") {
            item.cultivoBonusStats.critDamage = totalGrowth * 1.2f;
        } else if (statId == "lifestealPercent") {
            item.cultivoBonusStats.lifestealPercent = totalGrowth * 0.3f;
        } else if (statId == "cooldownReductionPercent") {
            item.cultivoBonusStats.cooldownReductionPercent = totalGrowth * 0.3f;
        } else if (statId == "attackPercent") {
            item.cultivoBonusStats.attackPercent = totalGrowth * 0.5f;
        } else if (statId == "defensePercent") {
            item.cultivoBonusStats.defensePercent = totalGrowth * 0.5f;
        } else if (statId == "hpPercent") {
            item.cultivoBonusStats.hpPercent = totalGrowth * 0.6f;
        } else if (statId == "physicalDamageBonus") {
            item.cultivoBonusStats.physicalDamageBonus = totalGrowth * 0.8f;
        } else if (statId == "armorPenetration") {
            item.cultivoBonusStats.armorPenetration = totalGrowth * 0.4f;
        }
    }
}

std::vector<std::pair<std::string, std::string>> CultivoSystem::getAvailableStats(const Item& item) {
    std::vector<std::pair<std::string, std::string>> result;
    const auto& s = item.stats;

    if (s.physicalDamage > 0) result.push_back({"physicalDamage", "Daño Físico"});
    if (s.defense > 0) result.push_back({"defense", "Defensa"});
    if (s.strength > 0) result.push_back({"STR", "Fuerza (STR)"});
    if (s.agility > 0) result.push_back({"AGI", "Agilidad (AGI)"});
    if (s.intelligence > 0) result.push_back({"INT", "Inteligencia (INT)"});
    if (s.vitality > 0) result.push_back({"VIT", "Vitalidad (VIT)"});
    if (s.hpBonus > 0) result.push_back({"hpBonus", "Puntos de Vida (HP)"});
    if (s.mpBonus > 0) result.push_back({"mpBonus", "Puntos de Maná (MP)"});
    if (s.critChance > 0.001f) result.push_back({"critChance", "Prob. Crítico (%)"});
    if (s.critDamage > 0.001f) result.push_back({"critDamage", "Daño Crítico (%)"});
    if (s.lifestealPercent > 0.001f) result.push_back({"lifestealPercent", "Robo de Vida (%)"});
    if (s.armorPenetration > 0.001f) result.push_back({"armorPenetration", "Penetración de Armadura (%)"});
    if (s.cooldownReductionPercent > 0.001f) result.push_back({"cooldownReductionPercent", "Reducción Enfriamiento (%)"});
    if (s.attackPercent > 0.001f) result.push_back({"attackPercent", "% Ataque Físico"});
    if (s.defensePercent > 0.001f) result.push_back({"defensePercent", "% Defensa"});
    if (s.hpPercent > 0.001f) result.push_back({"hpPercent", "% Vida Máxima"});
    if (s.physicalDamageBonus > 0.001f) result.push_back({"physicalDamageBonus", "% Bonus Daño Físico"});

    // If item has no non-zero base stats, fallback to core defaults based on item type
    if (result.empty()) {
        if (item.type == ItemType::Weapon) {
            result.push_back({"physicalDamage", "Daño Físico"});
            result.push_back({"STR", "Fuerza (STR)"});
        } else if (item.type == ItemType::Armor) {
            result.push_back({"defense", "Defensa"});
            result.push_back({"VIT", "Vitalidad (VIT)"});
        } else {
            result.push_back({"hpBonus", "Puntos de Vida (HP)"});
            result.push_back({"STR", "Fuerza (STR)"});
        }
    }

    return result;
}

void CultivoSystem::update(float dt) {
    if (!mCultivatedItem || !mCultivatedItem->cultivoLocked) {
        mAnimTimer = 0.0f;
        mAnimationFrame = 0;
        return;
    }

    mAnimTimer += dt;
    const float frameDuration = 1.0f / 8.0f; // 8 FPS
    while (mAnimTimer >= frameDuration) {
        mAnimTimer -= frameDuration;
        mAnimationFrame = (mAnimationFrame + 1) % 4;
    }
}

sf::IntRect CultivoSystem::getAnimationFrameRect() const {
    // 0,0 -> frame 1 (index 0)
    // 0,20 -> frame 2 (index 1)
    // 0,40 -> frame 3 (index 2)
    // 0,60 -> frame 4 (index 3)
    return sf::IntRect({0, mAnimationFrame * 20}, {20, 20});
}

void CultivoSystem::drawIndicator(sf::RenderTarget& target, ResourceManager& res, sf::Vector2f slotPos, float zoom) {
    if (!mCultivatedItem || !mCultivatedItem->cultivoLocked) return;

    try {
        sf::Texture& tex = res.getTexture("assets/ui/cultivo-panel/slot_charpanel_cultivo_indicator.png");
        sf::Sprite indicatorSprite(tex);
        indicatorSprite.setTextureRect(getAnimationFrameRect());
        indicatorSprite.setScale({ zoom, zoom });
        indicatorSprite.setPosition(slotPos);
        target.draw(indicatorSprite);
    } catch (const std::exception& e) {
        std::cerr << "[CultivoSystem] Error drawing indicator: " << e.what() << "\n";
    } catch (...) {}
}
