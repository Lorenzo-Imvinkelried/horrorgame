#include "GoldSystem.h"
#include "utils/TinyJson.h"
#include "utils/Random.h"
#include <iostream>
#include <algorithm>

void GoldSystem::loadGoldDrops(const std::string& filepath) {
    if (mLoaded) return;

    json::Value root = json::parseFile(filepath);
    if (root.type != json::Type::Object) {
        std::cerr << "[GoldSystem] WARNING: No se pudo cargar " << filepath << "\n";
        return;
    }

    auto rootObj = root.asObject();

    // Parse Mobs globales (fallback)
    if (rootObj.count("mobs") && rootObj.at("mobs").type == json::Type::Object) {
        for (const auto& pair : rootObj.at("mobs").asObject()) {
            std::string mobType = pair.first;
            if (pair.second.type == json::Type::Object) {
                auto mobObj = pair.second.asObject();
                MobGoldDropConfig cfg;
                if (mobObj.count("min_bronze")) cfg.minBronze = static_cast<uint64_t>(mobObj.at("min_bronze").asInt());
                if (mobObj.count("max_bronze")) cfg.maxBronze = static_cast<uint64_t>(mobObj.at("max_bronze").asInt());
                mMobDropConfigs[mobType] = cfg;
            }
        }
    }

    // Parse Maps (mobs específicos por mapa y/o multiplicador de mapa)
    if (rootObj.count("maps") && rootObj.at("maps").type == json::Type::Object) {
        for (const auto& mapPair : rootObj.at("maps").asObject()) {
            std::string mapName = mapPair.first;
            if (mapPair.second.type == json::Type::Object) {
                auto mapObj = mapPair.second.asObject();
                for (const auto& mobPair : mapObj) {
                    std::string keyName = mobPair.first;
                    if (keyName == "multiplier") {
                        if (mobPair.second.type == json::Type::Number) {
                            mMapMultipliers[mapName] = static_cast<float>(mobPair.second.asDouble());
                        }
                    } else if (mobPair.second.type == json::Type::Object) {
                        auto mobObj = mobPair.second.asObject();
                        MobGoldDropConfig cfg;
                        if (mobObj.count("min_bronze")) cfg.minBronze = static_cast<uint64_t>(mobObj.at("min_bronze").asInt());
                        if (mobObj.count("max_bronze")) cfg.maxBronze = static_cast<uint64_t>(mobObj.at("max_bronze").asInt());
                        mMapMobDropConfigs[mapName][keyName] = cfg;
                    }
                }
            }
        }
    }

    mLoaded = true;
    std::cout << "[GoldSystem] Configuracion de oro cargada (" << mMapMobDropConfigs.size() << " mapas con mobs, " << mMobDropConfigs.size() << " mobs globales).\n";
}

GoldSplit GoldSystem::splitCoins(uint64_t totalBronze) {
    GoldSplit split;
    split.bronze = static_cast<uint8_t>(totalBronze % 100ULL);
    uint64_t silverTotal = totalBronze / 100ULL;
    split.silver = static_cast<uint8_t>(silverTotal % 100ULL);
    uint64_t goldTotal = silverTotal / 100ULL;
    split.gold = static_cast<uint32_t>(std::min(99999ULL, goldTotal));
    return split;
}

uint64_t GoldSystem::rollGold(const std::string& mobType, const std::string& mapName, int level) {
    uint64_t baseBronze = 0;
    bool foundConfig = false;

    // 1. Intentar buscar en mapa específico -> mob especifico
    auto mapIt = mMapMobDropConfigs.find(mapName);
    if (mapIt != mMapMobDropConfigs.end()) {
        auto mobIt = mapIt->second.find(mobType);
        if (mobIt != mapIt->second.end()) {
            const auto& cfg = mobIt->second;
            if (cfg.maxBronze > cfg.minBronze) {
                baseBronze = static_cast<uint64_t>(Random::Int(static_cast<int>(cfg.minBronze), static_cast<int>(cfg.maxBronze)));
            } else {
                baseBronze = cfg.minBronze;
            }
            foundConfig = true;
        }
    }

    // 2. Si no se encontró en el mapa, buscar en mobs globales (fallback)
    if (!foundConfig) {
        auto mobIt = mMobDropConfigs.find(mobType);
        if (mobIt != mMobDropConfigs.end()) {
            const auto& cfg = mobIt->second;
            if (cfg.maxBronze > cfg.minBronze) {
                baseBronze = static_cast<uint64_t>(Random::Int(static_cast<int>(cfg.minBronze), static_cast<int>(cfg.maxBronze)));
            } else {
                baseBronze = cfg.minBronze;
            }
            foundConfig = true;
        }
    }

    // 3. Fallback genérico basado en nivel si no hay ninguna config
    if (!foundConfig) {
        baseBronze = static_cast<uint64_t>(Random::Int(5, 25) * level);
    }

    // Aplicar multiplicador del mapa si existe
    float mapMult = 1.0f;
    auto multIt = mMapMultipliers.find(mapName);
    if (multIt != mMapMultipliers.end()) {
        mapMult = multIt->second;
    }

    double finalAmount = static_cast<double>(baseBronze) * mapMult;
    return static_cast<uint64_t>(std::max(0.0, finalAmount));
}
