#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "Item.h"

struct LootEntry {
    std::string itemId;
    float chance = 0.0f;
    int minQty = 1;
    int maxQty = 1;
};

class LootManager {
public:
    LootManager(class ItemManager& itemMgr);
    ~LootManager() = default;

    // Loads loot tables from assets/data/loot_tables.json
    void loadLootTables(const std::string& filepath);

    // Rolls loot for a mob type at a certain level and returns a list of items
    std::vector<std::shared_ptr<Item>> rollLoot(const std::string& mobType, int mobLevel);

private:
    class ItemManager& mItemManager;
    std::map<std::string, std::vector<LootEntry>> mLootTables;
    bool mLoaded = false;
};
