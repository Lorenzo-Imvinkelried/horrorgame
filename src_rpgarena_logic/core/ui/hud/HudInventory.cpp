#include "Hud.h"
#include "Config.h"
#include "core/items/ItemManager.h"
#include "entities/player/Player.h"
#include "utils/Random.h"
#include "utils/TinyJson.h"
#include <iostream>

void Hud::addTestItemToInventory(ItemManager &itemMgr) {
  // [TEST] Crear item nivel 20 (Random Quality)
  auto sword = itemMgr.createRandomWeapon(20);

  if (sword) {
    std::cout << "[Hud] Received test item " << sword->name
              << " Damage: " << sword->stats.physicalDamage << "\n";
    int slot = findEmptyInventorySlot();
    if (slot != -1) {
      mInventoryPanel.setItem(slot, sword);
    } else {
      std::cerr << "[Hud] No hay espacio para item de prueba\n";
    }
  }
}

// [DEBUG] Clear Inventory
void Hud::clearInventory() {
  for (int i = 0; i < cfg::UI::Inventory::TOTAL_SLOTS; ++i) {
    mInventoryPanel.setItem(i, nullptr);
  }
}

int Hud::findEmptyInventorySlot() {
  for (int i = 0; i < 100; ++i) {
    auto item = mInventoryPanel.getItem(i);
    if (!item) {
      if (i < cfg::UI::Inventory::TOTAL_SLOTS)
        return i;
      else
        break; // Fin del inventario
    }
  }
  return -1;
}

bool Hud::addItemToInventory(std::shared_ptr<Item> item) {
  if (!item)
    return false;

  // Si es consumible, intentar stackear
  if (item->type == ItemType::Potion) {
    for (int i = 0; i < cfg::UI::Inventory::TOTAL_SLOTS; ++i) {
      auto invItem = mInventoryPanel.getItem(i);
      if (invItem && invItem->id == item->id && invItem->stackCount < 99) {
        int spaceLeft = 99 - invItem->stackCount;
        if (item->stackCount <= spaceLeft) {
          invItem->stackCount += item->stackCount;
          return true;
        } else {
          invItem->stackCount = 99;
          item->stackCount -= spaceLeft;
        }
      }
    }
  }

  // Si aun queda item por colocar, buscar slot vacio
  if (item->stackCount > 0) {
    int slot = findEmptyInventorySlot();
    if (slot != -1) {
      mInventoryPanel.setItem(slot, item);
      return true;
    }
  }

  return false;
}

void Hud::addAllItemsToInventory(ItemManager &itemMgr, Player *player, int targetLevel) {
  try {
    json::Value root = json::parseFile("assets/data/initial_inventory.json");
    if (root.type != json::Type::Object) {
      std::cerr << "[Hud] ERROR: assets/data/initial_inventory.json is not an "
                   "object.\n";
      return;
    }
    auto rootObj = root.asObject();

    // 0. Process initial_coins
    if (player) {
      uint64_t totalBronze = 0;
      if (rootObj.count("initial_coins") && rootObj.at("initial_coins").type == json::Type::Object) {
        auto coinsObj = rootObj.at("initial_coins").asObject();
        uint64_t g = coinsObj.count("gold") ? static_cast<uint64_t>(coinsObj.at("gold").asInt()) : 0;
        uint64_t s = coinsObj.count("silver") ? static_cast<uint64_t>(coinsObj.at("silver").asInt()) : 0;
        uint64_t b = coinsObj.count("bronze") ? static_cast<uint64_t>(coinsObj.at("bronze").asInt()) : 0;
        totalBronze = g * 10000ULL + s * 100ULL + b;
      } else {
        uint64_t g = rootObj.count("initial_gold") ? static_cast<uint64_t>(rootObj.at("initial_gold").asInt()) : (rootObj.count("gold") ? static_cast<uint64_t>(rootObj.at("gold").asInt()) : 0);
        uint64_t s = rootObj.count("initial_silver") ? static_cast<uint64_t>(rootObj.at("initial_silver").asInt()) : (rootObj.count("silver") ? static_cast<uint64_t>(rootObj.at("silver").asInt()) : 0);
        uint64_t b = rootObj.count("initial_bronze") ? static_cast<uint64_t>(rootObj.at("initial_bronze").asInt()) : (rootObj.count("bronze") ? static_cast<uint64_t>(rootObj.at("bronze").asInt()) : 0);
        totalBronze = g * 10000ULL + s * 100ULL + b;
      }
      if (totalBronze > 0) {
        player->setBronzeCoins(totalBronze);
      }
    }

    // 1. Process starting_items
    if (rootObj.count("starting_items") &&
        rootObj.at("starting_items").type == json::Type::Array) {
      auto itemsArr = rootObj.at("starting_items").asArray();
      for (const auto &itemVal : itemsArr) {
        if (itemVal.type == json::Type::Object) {
          auto itemObj = itemVal.asObject();
          std::string id =
              itemObj.count("id") ? itemObj.at("id").asString() : "";
          std::string type =
              itemObj.count("type") ? itemObj.at("type").asString() : "";

          std::shared_ptr<Item> item = nullptr;
          if (id.rfind("weapon_32x32_", 0) == 0) {
            try {
              int indexVal = std::stoi(id.substr(13));
              int lvl32 =
                  (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
              item = itemMgr.create32x32Weapon(indexVal, lvl32);
            } catch (...) {
              std::cerr << "[Hud] ERROR parsing 32x32 index from id: " << id
                        << "\n";
            }
          } else if (type == "weapon_32x32_index") {
            int indexVal = itemObj.count("val") ? itemObj.at("val").asInt() : 1;
            int lvl32 = (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
            item = itemMgr.create32x32Weapon(indexVal, lvl32);
          } else if (!id.empty()) {
            item = itemMgr.createItem(id);
          }

          if (item) {
            if (itemObj.count("stackCount")) {
              item->stackCount = itemObj.at("stackCount").asInt();
            }
            if (itemObj.count("slot")) {
              int slot = itemObj.at("slot").asInt();
              if (slot >= 0 && slot < cfg::UI::Inventory::TOTAL_SLOTS) {
                mInventoryPanel.setItem(slot, item);
              }
            } else {
              addItemToInventory(item);
            }
          }
        }
      }
    }

    // 2. Process random_generation
    if (rootObj.count("random_generation") &&
        rootObj.at("random_generation").type == json::Type::Object) {
      auto genObj = rootObj.at("random_generation").asObject();

      // Random armors
      if (genObj.count("armors") &&
          genObj.at("armors").type == json::Type::Array) {
        auto armorsArr = genObj.at("armors").asArray();
        for (const auto &armorVal : armorsArr) {
          std::string armorSlotName = armorVal.asString();
          EquipmentSlot slot = EquipmentSlot::None;
          if (armorSlotName == "Head")
            slot = EquipmentSlot::Head;
          else if (armorSlotName == "Cape")
            slot = EquipmentSlot::Cape;
          else if (armorSlotName == "Chest")
            slot = EquipmentSlot::Chest;
          else if (armorSlotName == "Hands")
            slot = EquipmentSlot::Hands;
          else if (armorSlotName == "Legs")
            slot = EquipmentSlot::Legs;
          else if (armorSlotName == "Feet")
            slot = EquipmentSlot::Feet;

          if (slot != EquipmentSlot::None) {
            int level = (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
            auto armorItem = itemMgr.createRandomArmor(slot, level);
            if (armorItem) {
              int islot = findEmptyInventorySlot();
              if (islot != -1) {
                mInventoryPanel.setItem(islot, armorItem);
              }
            }
          }
        }
      }

      // Random rings
      if (genObj.count("rings_count")) {
        int ringsCount = genObj.at("rings_count").asInt();
        for (int i = 0; i < ringsCount; ++i) {
          int level = (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
          auto ringItem = itemMgr.createRandomRing(level);
          if (ringItem) {
            int islot = findEmptyInventorySlot();
            if (islot != -1) {
              mInventoryPanel.setItem(islot, ringItem);
            }
          }
        }
      }

      // Random weapons
      if (genObj.count("weapons_count")) {
        int weaponsCount = genObj.at("weapons_count").asInt();
        for (int i = 0; i < weaponsCount; ++i) {
          int level = (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
          auto item = itemMgr.createRandomWeapon(level);
          if (item) {
            int slot = findEmptyInventorySlot();
            if (slot != -1) {
              mInventoryPanel.setItem(slot, item);
            } else {
              break;
            }
          }
        }
      }

      // Random stones (gems)
      if (genObj.count("stones_count")) {
        int stonesCount = genObj.at("stones_count").asInt();
        for (int i = 0; i < stonesCount; ++i) {
          int level = (targetLevel == -1) ? Random::Int(1, 100) : targetLevel;
          auto stoneItem = itemMgr.createRandomStone(level);
          if (stoneItem) {
            int slot = findEmptyInventorySlot();
            if (slot != -1) {
              mInventoryPanel.setItem(slot, stoneItem);
            } else {
              break;
            }
          }
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[Hud] ERROR parsing initial_inventory.json: " << e.what()
              << "\n";
  } catch (...) {
    std::cerr
        << "[Hud] ERROR parsing initial_inventory.json: unknown exception.\n";
  }
}
