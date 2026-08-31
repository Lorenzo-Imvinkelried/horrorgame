#include "LootTable.h"
#include "ItemRegistry.h"
#include <algorithm>
#include <cstdlib>

void LootTable::AddGuaranteed(ItemId id, int minQty, int maxQty) {
    if (!id.IsValid()) return;
    LootEntry entry;
    entry.itemId = id;
    entry.minQuantity = std::max(1, minQty);
    entry.maxQuantity = std::max(entry.minQuantity, maxQty);
    entry.isGuaranteed = true;
    m_guaranteedDrops.push_back(entry);
}

void LootTable::AddGuaranteedByString(const std::string& stringId, int minQty, int maxQty) {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    AddGuaranteed(id, minQty, maxQty);
}

void LootTable::AddIndependentDrop(ItemId id, float chance, int minQty, int maxQty) {
    if (!id.IsValid() || chance <= 0.0f) return;
    LootEntry entry;
    entry.itemId = id;
    entry.dropChance = std::clamp(chance, 0.0f, 1.0f);
    entry.minQuantity = std::max(1, minQty);
    entry.maxQuantity = std::max(entry.minQuantity, maxQty);
    m_independentDrops.push_back(entry);
}

void LootTable::AddIndependentDropByString(const std::string& stringId, float chance, int minQty, int maxQty) {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    AddIndependentDrop(id, chance, minQty, maxQty);
}

void LootTable::AddWeightedRoll(ItemId id, int weight, int minQty, int maxQty) {
    if (!id.IsValid() || weight <= 0) return;
    LootEntry entry;
    entry.itemId = id;
    entry.rollWeight = weight;
    entry.minQuantity = std::max(1, minQty);
    entry.maxQuantity = std::max(entry.minQuantity, maxQty);
    m_weightedPool.push_back(entry);
    m_totalWeight += weight;
}

void LootTable::AddWeightedRollByString(const std::string& stringId, int weight, int minQty, int maxQty) {
    ItemId id = ItemRegistry::Get().FindId(stringId);
    AddWeightedRoll(id, weight, minQty, maxQty);
}

std::vector<ItemInstance> LootTable::GenerateLoot(float luckMultiplier, int weightedPulls) const {
    std::vector<ItemInstance> results;

    auto randomRange = [](int minVal, int maxVal) -> uint16_t {
        if (minVal >= maxVal) return static_cast<uint16_t>(minVal);
        return static_cast<uint16_t>(minVal + rand() % (maxVal - minVal + 1));
    };

    // 1. Drops Garantizados (100% de certeza, ej. Carne, Huesos)
    for (const auto& entry : m_guaranteedDrops) {
        uint16_t qty = randomRange(entry.minQuantity, entry.maxQuantity);
        results.push_back(ItemInstance(entry.itemId, qty));
    }

    // 2. Drops Independientes (Ensayos de Bernoulli con modificador de Suerte)
    for (const auto& entry : m_independentDrops) {
        float effectiveChance = std::clamp(entry.dropChance * luckMultiplier, 0.0f, 1.0f);
        float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        if (roll <= effectiveChance) {
            uint16_t qty = randomRange(entry.minQuantity, entry.maxQuantity);
            results.push_back(ItemInstance(entry.itemId, qty));
        }
    }

    // 3. Ruleta Ponderada (Categorical Sampling para cofres o drops raros garantizados)
    if (weightedPulls > 0 && !m_weightedPool.empty() && m_totalWeight > 0) {
        for (int p = 0; p < weightedPulls; ++p) {
            int roll = rand() % m_totalWeight;
            int currentAcc = 0;
            for (const auto& entry : m_weightedPool) {
                currentAcc += entry.rollWeight;
                if (roll < currentAcc) {
                    uint16_t qty = randomRange(entry.minQuantity, entry.maxQuantity);
                    results.push_back(ItemInstance(entry.itemId, qty));
                    break;
                }
            }
        }
    }

    return results;
}
