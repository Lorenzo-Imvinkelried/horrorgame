#include "Mob.h"
#include "core/items/Item.h"
#include "core/items/ItemManager.h"
#include "core/engine/ResourceManager.h"
#include "utils/Random.h"
#include <iostream>
#include <vector>

std::shared_ptr<Item> Mob::getWeapon(int slotIndex) const {
    if (slotIndex == 0) return mEquippedWeapon;
    if (slotIndex == 1) return mWeaponSecondary;
    return nullptr;
}

void Mob::equipWeapon(std::shared_ptr<Item> item, ResourceManager& res, int slotIndex) {
    mResourceManager = &res;
    if (item == nullptr || item->type != ItemType::Weapon) return;

    if (item->gripType == GripType::TwoHanded) {
        mEquippedWeapon = nullptr;
        mWeaponSecondary = nullptr;
    } else {
        int otherSlot = 1 - slotIndex;
        auto opposite = (otherSlot == 0) ? mEquippedWeapon : mWeaponSecondary;
        if (opposite && opposite->gripType == GripType::TwoHanded) {
            if (otherSlot == 0) mEquippedWeapon = nullptr;
            else mWeaponSecondary = nullptr;
        }
    }

    if (slotIndex == 0) {
        mEquippedWeapon = item;
    } else if (slotIndex == 1) {
        mWeaponSecondary = item;
    }

    mEquipment[static_cast<int>(EquipmentSlot::MainHand)] = mEquippedWeapon;
    mEquipment[static_cast<int>(EquipmentSlot::OffHand)] = mWeaponSecondary;

    updateWeaponVisuals(res);
    recalculateStats();
    notifyStatsChanged();
}

void Mob::equipItem(std::shared_ptr<Item> item, EquipmentSlot slot, ResourceManager& res) {
    int idx = static_cast<int>(slot);
    if (idx >= 0 && idx < 12) {
        if (slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) {
            if (item && item->gripType == GripType::TwoHanded) {
                mEquipment[static_cast<int>(EquipmentSlot::MainHand)] = nullptr;
                mEquipment[static_cast<int>(EquipmentSlot::OffHand)] = nullptr;
            } else {
                EquipmentSlot otherSlot = (slot == EquipmentSlot::MainHand) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
                auto opposite = mEquipment[static_cast<int>(otherSlot)];
                if (opposite && opposite->gripType == GripType::TwoHanded) {
                    mEquipment[static_cast<int>(otherSlot)] = nullptr;
                }
            }
        }

        mEquipment[idx] = item;

        mEquippedWeapon = mEquipment[static_cast<int>(EquipmentSlot::MainHand)];
        mWeaponSecondary = mEquipment[static_cast<int>(EquipmentSlot::OffHand)];

        if (slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) {
            updateWeaponVisuals(res);
        } else if (slot == EquipmentSlot::Head || slot == EquipmentSlot::Chest || slot == EquipmentSlot::Hands || slot == EquipmentSlot::Feet) {
            updateArmorVisuals(res);
        }
        recalculateStats();
        notifyStatsChanged();
    }
}

void Mob::unequipItem(EquipmentSlot slot) {
    int idx = static_cast<int>(slot);
    if (idx >= 0 && idx < 12) {
        mEquipment[idx] = nullptr;
        if (slot == EquipmentSlot::MainHand) {
            mEquippedWeapon = nullptr;
            if (mResourceManager) updateWeaponVisuals(*mResourceManager);
        } else if (slot == EquipmentSlot::OffHand) {
            mWeaponSecondary = nullptr;
            if (mResourceManager) updateWeaponVisuals(*mResourceManager);
        } else if (slot == EquipmentSlot::Head || slot == EquipmentSlot::Chest || slot == EquipmentSlot::Hands || slot == EquipmentSlot::Feet) {
            if (mResourceManager) {
                updateArmorVisuals(*mResourceManager);
            } else {
                mSkin.setArmorVisuals(slot, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
            }
        }
        recalculateStats();
        notifyStatsChanged();
    }
}

std::shared_ptr<Item> Mob::getEquippedItem(EquipmentSlot slot) const {
    int idx = static_cast<int>(slot);
    if (idx >= 0 && idx < 12) {
        return mEquipment[idx];
    }
    return nullptr;
}

void Mob::unequipWeapon(int slotIndex) {
    if (slotIndex == 0) {
        mEquippedWeapon = nullptr;
    } else if (slotIndex == 1) {
        mWeaponSecondary = nullptr;
    }

    mEquipment[static_cast<int>(EquipmentSlot::MainHand)] = mEquippedWeapon;
    mEquipment[static_cast<int>(EquipmentSlot::OffHand)] = mWeaponSecondary;

    if (mResourceManager) {
        updateWeaponVisuals(*mResourceManager);
    } else {
        if (slotIndex == 0) {
            mSkin.setWeaponVisuals(nullptr, nullptr, sf::IntRect({0,0}, {0,0}), sf::IntRect({0,0}, {0,0}), ItemQuality::Common, {0.f,0.f});
        } else if (slotIndex == 1) {
            mSkin.setSecondaryWeaponVisuals(nullptr, nullptr, sf::IntRect({0,0}, {0,0}), sf::IntRect({0,0}, {0,0}), ItemQuality::Common, {0.f,0.f});
        }
    }
    recalculateStats();
    notifyStatsChanged();
}

void Mob::updateWeaponVisuals(ResourceManager& res) {
    std::shared_ptr<Item> twoHandedWeapon = nullptr;
    if (mEquippedWeapon && mEquippedWeapon->gripType == GripType::TwoHanded) {
        twoHandedWeapon = mEquippedWeapon;
    } else if (mWeaponSecondary && mWeaponSecondary->gripType == GripType::TwoHanded) {
        twoHandedWeapon = mWeaponSecondary;
    }

    if (twoHandedWeapon) {
        mHasTwoHandedWeapon = true;
        try {
            std::string texPath = twoHandedWeapon->texturePath.empty() ? "assets/items/weapons/weapons-base.png" : twoHandedWeapon->texturePath;
            const sf::Texture& baseTex = res.getTexture(texPath);
            const sf::Texture& layoutTex = res.getTexture("assets/items/weapons/weapons_layout.png");
            
            sf::IntRect overlayRect({0,0}, {0,0});
            if (twoHandedWeapon->textureRect.size.x == 16 && twoHandedWeapon->textureRect.size.y == 16) {
                if (twoHandedWeapon->overlayGridCoords.x >= 0 && twoHandedWeapon->overlayGridCoords.y >= 0) {
                     overlayRect = sf::IntRect({twoHandedWeapon->overlayGridCoords.x * 16, twoHandedWeapon->overlayGridCoords.y * 16}, {16, 16});
                } else {
                     overlayRect = twoHandedWeapon->textureRect;
                }
            }

            mSkin.setWeaponVisuals(&baseTex, &layoutTex, twoHandedWeapon->textureRect, overlayRect, twoHandedWeapon->quality, twoHandedWeapon->offset, true, twoHandedWeapon->fortificationLevel);
        } catch(...) {
            std::cout << "[Mob] Error loading two-handed visuals.\n";
        }
        mSkin.setSecondaryWeaponVisuals(nullptr, nullptr, sf::IntRect({0,0}, {0,0}), sf::IntRect({0,0}, {0,0}), ItemQuality::Common, {0.f,0.f});
    } else {
        mHasTwoHandedWeapon = false;
        if (mEquippedWeapon) {
            try {
                std::string texPath = mEquippedWeapon->texturePath.empty() ? "assets/items/weapons/weapons-base.png" : mEquippedWeapon->texturePath;
                const sf::Texture& baseTex = res.getTexture(texPath);
                const sf::Texture& layoutTex = res.getTexture("assets/items/weapons/weapons_layout.png");
                
                sf::IntRect overlayRect({0,0}, {0,0});
                if (mEquippedWeapon->textureRect.size.x == 16 && mEquippedWeapon->textureRect.size.y == 16) {
                    if (mEquippedWeapon->overlayGridCoords.x >= 0 && mEquippedWeapon->overlayGridCoords.y >= 0) {
                         overlayRect = sf::IntRect({mEquippedWeapon->overlayGridCoords.x * 16, mEquippedWeapon->overlayGridCoords.y * 16}, {16, 16});
                    } else {
                         overlayRect = mEquippedWeapon->textureRect;
                    }
                }

                mSkin.setWeaponVisuals(&baseTex, &layoutTex, mEquippedWeapon->textureRect, overlayRect, mEquippedWeapon->quality, mEquippedWeapon->offset, false, mEquippedWeapon->fortificationLevel);
            } catch(...) {
                std::cout << "[Mob] Error loading main hand visuals.\n";
            }
        } else {
            mSkin.setWeaponVisuals(nullptr, nullptr, sf::IntRect({0,0}, {0,0}), sf::IntRect({0,0}, {0,0}), ItemQuality::Common, {0.f,0.f});
        }

        if (mWeaponSecondary) {
            try {
                std::string texPath = mWeaponSecondary->texturePath.empty() ? "assets/items/weapons/weapons-base.png" : mWeaponSecondary->texturePath;
                const sf::Texture& baseTex = res.getTexture(texPath);
                const sf::Texture& layoutTex = res.getTexture("assets/items/weapons/weapons_layout.png");
                
                sf::IntRect overlayRect({0,0}, {0,0});
                if (mWeaponSecondary->textureRect.size.x == 16 && mWeaponSecondary->textureRect.size.y == 16) {
                    if (mWeaponSecondary->overlayGridCoords.x >= 0 && mWeaponSecondary->overlayGridCoords.y >= 0) {
                         overlayRect = sf::IntRect({mWeaponSecondary->overlayGridCoords.x * 16, mWeaponSecondary->overlayGridCoords.y * 16}, {16, 16});
                    } else {
                         overlayRect = mWeaponSecondary->textureRect;
                    }
                }

                mSkin.setSecondaryWeaponVisuals(&baseTex, &layoutTex, mWeaponSecondary->textureRect, overlayRect, mWeaponSecondary->quality, mWeaponSecondary->offset, mWeaponSecondary->fortificationLevel);
            } catch(...) {
                std::cout << "[Mob] Error loading secondary hand visuals.\n";
            }
        } else {
            mSkin.setSecondaryWeaponVisuals(nullptr, nullptr, sf::IntRect({0,0}, {0,0}), sf::IntRect({0,0}, {0,0}), ItemQuality::Common, {0.f,0.f});
        }
    }
}

void Mob::updateArmorVisuals(ResourceManager& res) {
    std::vector<EquipmentSlot> armorSlots = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Hands,
        EquipmentSlot::Feet
    };
    for (auto slot : armorSlots) {
        auto item = mEquipment[static_cast<int>(slot)];
        if (item) {
            try {
                std::string texPath = item->texturePath.empty()
                                          ? "assets/items/weapons/armor_32x32.png"
                                          : item->texturePath;
                const sf::Texture& baseTex = res.getTexture(texPath);
                mSkin.setArmorVisuals(slot, &baseTex, item->textureRect, item->offset, item->scale, item->fortificationLevel);
            } catch (...) {
                std::cout << "[Mob] Error loading armor visuals for slot " << static_cast<int>(slot) << "\n";
            }
        } else {
            mSkin.setArmorVisuals(slot, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f, 0);
        }
    }
}

bool Mob::hasWeaponEquipped() const {
    return getWeapon(0) != nullptr || getWeapon(1) != nullptr;
}

void Mob::setupEquipment() {
    unequipWeapon(0);
    unequipWeapon(1);
    for (int i = 0; i < 12; ++i) {
        mEquipment[i] = nullptr;
    }

    if (mResourceManager) {
        updateArmorVisuals(*mResourceManager);
    } else {
        mSkin.setArmorVisuals(EquipmentSlot::Head, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
        mSkin.setArmorVisuals(EquipmentSlot::Chest, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
        mSkin.setArmorVisuals(EquipmentSlot::Hands, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
        mSkin.setArmorVisuals(EquipmentSlot::Feet, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
    }

    if (!mResourceManager) return;

    if (!mBlueprint.equipmentPool.empty()) {
        for (const auto& pair : mBlueprint.equipmentPool) {
            EquipmentSlot slot = pair.first;
            const auto& options = pair.second;

            float roll = Random::Float(0.f, 100.f);
            float cumulativeChance = 0.f;

            for (const auto& option : options) {
                cumulativeChance += option.chance;
                if (roll <= cumulativeChance) {
                    std::shared_ptr<Item> item = nullptr;
                    if (option.itemId == "random_weapon") {
                        item = mItemManager.createRandomWeapon(mLevel);
                    } else if (option.itemId == "random_armor") {
                        item = mItemManager.createRandomArmor(slot, mLevel);
                    } else if (option.itemId == "random_stone") {
                        item = mItemManager.createRandomStone(mLevel);
                    } else if (option.itemId == "random_ring") {
                        item = mItemManager.createRandomRing(mLevel);
                    } else if (option.itemId == "weapon_32x32_1") {
                        item = mItemManager.create32x32Weapon(1, mLevel);
                    } else if (option.itemId == "weapon_32x32_2") {
                        item = mItemManager.create32x32Weapon(2, mLevel);
                    } else {
                        item = mItemManager.createItem(option.itemId);
                    }

                    if (item) {
                        equipItem(item, slot, *mResourceManager);
                    }
                    break;
                }
            }
        }
    } else {
        setupRandomWeapons();
    }
}

void Mob::setupRandomWeapons() {
    unequipWeapon(0);
    unequipWeapon(1);

    if (mBlueprintName == "mob_grande_1") {
        return;
    }

    if (!mResourceManager) return;

    if (mBlueprintName == "goblin") {
        if (Random::Int(1, 100) <= 50) {
            int index = Random::Int(1, 2);
            auto weapon = mItemManager.create32x32Weapon(index, mLevel);
            if (weapon) {
                equipWeapon(weapon, *mResourceManager, 0);
            }
            return;
        }
    }

    int roll = Random::Int(1, 100);
    if (roll <= 40) {
    } else if (roll <= 80) {
        auto weapon = mItemManager.createRandomWeapon(mLevel);
        if (weapon) {
            equipWeapon(weapon, *mResourceManager, 0);
        }
    } else {
        auto weaponMain = mItemManager.createRandomWeapon(mLevel);
        if (weaponMain) {
            equipWeapon(weaponMain, *mResourceManager, 0);
        }
        auto weaponSec = mItemManager.createRandomWeapon(mLevel);
        if (weaponSec) {
            equipWeapon(weaponSec, *mResourceManager, 1);
        }
    }

    setupRandomArmor();
}

void Mob::setupRandomArmor() {
    if (!mResourceManager) return;
    if (mBlueprintName != "goblin") return;

    int count = Random::Int(1, 4);

    std::vector<EquipmentSlot> slots = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Hands,
        EquipmentSlot::Feet
    };

    for (size_t i = 0; i < slots.size(); ++i) {
        size_t j = Random::Int(0, static_cast<int>(slots.size() - 1));
        std::swap(slots[i], slots[j]);
    }

    for (int i = 0; i < count; ++i) {
        EquipmentSlot slot = slots[i];
        auto armor = mItemManager.createRandomArmor(slot, mLevel);
        if (armor) {
            equipItem(armor, slot, *mResourceManager);
        }
    }
}

void Mob::emitWeaponGibs(GoreSystem& gore, float floorY, sf::Vector2f sourcePos, float forceMultiplier) {
}

Animation::WeaponSpawnInfo Mob::getWeaponSpawnInfo(int slotIndex) const {
    return mSkin.getWeaponSpawnInfo(slotIndex);
}
