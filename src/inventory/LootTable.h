#pragma once

#include "ItemInstance.h"
#include <vector>
#include <string>
#include <random>

struct LootEntry {
    ItemId itemId = INVALID_ITEM_ID;
    int minQuantity = 1;
    int maxQuantity = 1;
    float dropChance = 1.0f; // 0.0f a 1.0f: Probabilidad Bernoulli independiente
    int rollWeight = 100;    // Peso para selección por ruleta ponderada
    bool isGuaranteed = false;
};

/**
 * @brief LootTable: Generador de botín desacoplado basado en datos.
 * Soporta drops independientes (Bernoulli), drops garantizados y ruletas ponderadas con modificador de suerte.
 */
class LootTable {
public:
    LootTable() = default;
    ~LootTable() = default;

    void AddGuaranteed(ItemId id, int minQty = 1, int maxQty = 1);
    void AddGuaranteedByString(const std::string& stringId, int minQty = 1, int maxQty = 1);

    void AddIndependentDrop(ItemId id, float chance, int minQty = 1, int maxQty = 1);
    void AddIndependentDropByString(const std::string& stringId, float chance, int minQty = 1, int maxQty = 1);

    void AddWeightedRoll(ItemId id, int weight, int minQty = 1, int maxQty = 1);
    void AddWeightedRollByString(const std::string& stringId, int weight, int minQty = 1, int maxQty = 1);

    /**
     * @brief Genera una lista de items resultantes.
     * @param luckMultiplier Multiplicador de suerte del jugador (ej. 1.0f base, 1.25f con buffs).
     * @param weightedPulls Cantidad de tiradas en la ruleta ponderada (ej. 1 para cofre o drop raro garantizado).
     */
    std::vector<ItemInstance> GenerateLoot(float luckMultiplier = 1.0f, int weightedPulls = 0) const;

private:
    std::vector<LootEntry> m_guaranteedDrops;
    std::vector<LootEntry> m_independentDrops;
    std::vector<LootEntry> m_weightedPool;
    int m_totalWeight = 0;
};
