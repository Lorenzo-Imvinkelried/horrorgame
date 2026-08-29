#include "LootManager.h"
#include "ItemManager.h"
#include "utils/TinyJson.h"
#include "utils/Random.h"
#include <iostream>

LootManager::LootManager(ItemManager& itemMgr) 
    : mItemManager(itemMgr) 
{}

void LootManager::loadLootTables(const std::string& filepath) {
    if (mLoaded) return;

    std::cout << "[LootManager] Loading loot tables from " << filepath << "...\n";
    json::Value root = json::parseFile(filepath);
    if (root.type == json::Type::Null) {
        std::cerr << "[LootManager] ERROR: Failed to load loot tables config. File is missing or invalid JSON.\n";
        return;
    }

    if (root.type == json::Type::Object) {
        auto rootObj = root.asObject();
        for (const auto& pair : rootObj) {
            std::string mobType = pair.first;
            if (pair.second.type == json::Type::Array) {
                auto lootArr = pair.second.asArray();
                for (const auto& val : lootArr) {
                    if (val.type == json::Type::Object) {
                        auto entryObj = val.asObject();
                        LootEntry entry;
                        if (entryObj.count("id")) entry.itemId = entryObj.at("id").asString();
                        if (entryObj.count("chance")) entry.chance = (float)entryObj.at("chance").asDouble();
                        if (entryObj.count("min_quantity")) entry.minQty = entryObj.at("min_quantity").asInt();
                        if (entryObj.count("max_quantity")) entry.maxQty = entryObj.at("max_quantity").asInt();
                        mLootTables[mobType].push_back(entry);
                    }
                }
            }
        }
    }

    mLoaded = true;
    std::cout << "[LootManager] Successfully loaded loot tables for " << mLootTables.size() << " mob types.\n";
}

std::vector<std::shared_ptr<Item>> LootManager::rollLoot(const std::string& mobType, int mobLevel) {
    std::vector<std::shared_ptr<Item>> droppedItems;

    auto it = mLootTables.find(mobType);
    if (it == mLootTables.end()) {
        return droppedItems; // No loot table for this mob type
    }

    const auto& entries = it->second;
    for (const auto& entry : entries) {
        float roll = Random::Float(0.f, 100.f);
        if (roll <= entry.chance) {
            std::shared_ptr<Item> item = nullptr;

            if (entry.itemId == "random_weapon") {
                item = mItemManager.createRandomWeapon(mobLevel);
            } else if (entry.itemId == "random_armor") {
                // Select a random armor slot
                EquipmentSlot slots[] = { EquipmentSlot::Head, EquipmentSlot::Chest, EquipmentSlot::Hands, EquipmentSlot::Feet };
                EquipmentSlot slot = slots[Random::Int(0, 3)];
                item = mItemManager.createRandomArmor(slot, mobLevel);
            } else if (entry.itemId == "random_stone") {
                item = mItemManager.createRandomStone(mobLevel);
            } else if (entry.itemId == "random_ring") {
                item = mItemManager.createRandomRing(mobLevel);
            } else if (entry.itemId == "weapon_32x32_1") {
                item = mItemManager.create32x32Weapon(1, mobLevel);
            } else if (entry.itemId == "weapon_32x32_2") {
                item = mItemManager.create32x32Weapon(2, mobLevel);
            } else {
                item = mItemManager.createItem(entry.itemId);
            }

            if (item) {
                int quantity = 1;
                if (entry.maxQty > entry.minQty) {
                    quantity = Random::Int(entry.minQty, entry.maxQty);
                } else {
                    quantity = entry.minQty;
                }
                item->stackCount = quantity;
                droppedItems.push_back(item);
            }
        }
    }

    return droppedItems;
}
