#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

struct GoldSplit {
    uint32_t gold = 0;   // Máx 99999
    uint8_t silver = 0;  // 0 - 99
    uint8_t bronze = 0;  // 0 - 99
};

struct MobGoldDropConfig {
    uint64_t minBronze = 0;
    uint64_t maxBronze = 0;
};

class GoldSystem {
public:
    GoldSystem() = default;
    ~GoldSystem() = default;

    void loadGoldDrops(const std::string& filepath);

    // Dado un total en bronce, lo desglosa en Oro, Plata y Bronce (con clamp visual de oro a 99999)
    static GoldSplit splitCoins(uint64_t totalBronze);

    // Calcula las monedas dropeadas al morir un mob
    uint64_t rollGold(const std::string& mobType, const std::string& mapName, int level = 1);

private:
    std::unordered_map<std::string, std::unordered_map<std::string, MobGoldDropConfig>> mMapMobDropConfigs; // mapName -> (mobType -> MobGoldDropConfig)
    std::unordered_map<std::string, MobGoldDropConfig> mMobDropConfigs;
    std::unordered_map<std::string, float> mMapMultipliers;
    bool mLoaded = false;
};
