#include "Player.h"
#include "Config.h"
#include "core/engine/ResourceManager.h"
#include "core/engine/animation/Animation.h"
#include "core/systems/CultivoSystem.h"
#include "core/systems/gore/GoreSystem.h"
#include "core/systems/ShieldSystem.h"
#include "core/systems/SoundSystem.h"
#include <cmath>
#include <iostream>

static sf::Clock weaponSoundCooldownClock;

void Player::equipItem(std::shared_ptr<Item> item, EquipmentSlot slot, ResourceManager &res) {
  mResourceManager = &res;
  int idx = static_cast<int>(slot);
  if (idx < 0 || idx >= static_cast<int>(EquipmentSlot::Count))
    return;

  if (weaponSoundCooldownClock.getElapsedTime().asMilliseconds() > 100) {
    if (auto *ss = SoundSystem::getInstance()) {
      ss->playSound("assets/sounds/player/equip_weapon.mp3");
    }
    weaponSoundCooldownClock.restart();
  }

  auto cultivated = CultivoSystem::getInstance().getCultivatedItem();

  if (slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) {
    if (item && item->gripType == GripType::TwoHanded) {
      auto mh = mEquipment[static_cast<int>(EquipmentSlot::MainHand)];
      auto oh = mEquipment[static_cast<int>(EquipmentSlot::OffHand)];
      if (cultivated && cultivated != item && (cultivated == mh || cultivated == oh)) {
        CultivoSystem::getInstance().setCultivatedItem(nullptr);
        cultivated = nullptr;
      }
      mEquipment[static_cast<int>(EquipmentSlot::MainHand)] = nullptr;
      mEquipment[static_cast<int>(EquipmentSlot::OffHand)] = nullptr;
    } else {
      EquipmentSlot otherSlot = (slot == EquipmentSlot::MainHand) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
      auto opposite = mEquipment[static_cast<int>(otherSlot)];
      if (opposite && opposite->gripType == GripType::TwoHanded) {
        if (cultivated && cultivated != item && cultivated == opposite) {
          CultivoSystem::getInstance().setCultivatedItem(nullptr);
          cultivated = nullptr;
        }
        mEquipment[static_cast<int>(otherSlot)] = nullptr;
      }
    }
  }

  if (mEquipment[idx] && mEquipment[idx] != item && cultivated && cultivated == mEquipment[idx]) {
    CultivoSystem::getInstance().setCultivatedItem(nullptr);
  }

  mEquipment[idx] = item;

  if (slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) {
    updateWeaponVisuals(res);
  } else if (slot == EquipmentSlot::Head || slot == EquipmentSlot::Chest || slot == EquipmentSlot::Hands || slot == EquipmentSlot::Feet) {
    updateArmorVisuals(res);
  }
  recalculateStats();

  if (item) {
    std::cout << "[Player] Equipado en slot " << idx << ": " << item->name << "\n";
  }
}

void Player::unequipItem(EquipmentSlot slot) {
  int idx = static_cast<int>(slot);
  if (idx < 0 || idx >= static_cast<int>(EquipmentSlot::Count))
    return;

  if (weaponSoundCooldownClock.getElapsedTime().asMilliseconds() > 100) {
    if (auto *ss = SoundSystem::getInstance()) {
      ss->playSound("assets/sounds/player/equip_weapon.mp3");
    }
    weaponSoundCooldownClock.restart();
  }

  if (mEquipment[idx]) {
    if (CultivoSystem::getInstance().getCultivatedItem() == mEquipment[idx]) {
      CultivoSystem::getInstance().setCultivatedItem(nullptr);
    }
  }

  mEquipment[idx] = nullptr;

  if (slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) {
    if (!hasShieldEquipped() && isGuardActive()) {
      if (auto* ss = ShieldSystem::getInstance()) {
        ss->setGuardActive(this, false);
      } else {
        setGuardActive(false);
      }
    }
    if (mResourceManager) {
      updateWeaponVisuals(*mResourceManager);
    } else {
      if (slot == EquipmentSlot::MainHand) {
        mSkin.setWeaponVisuals(nullptr, nullptr, sf::IntRect({0, 0}, {0, 0}),
                               sf::IntRect({0, 0}, {0, 0}), ItemQuality::Common,
                               1.f, {0.f, 0.f}, false);
      } else if (slot == EquipmentSlot::OffHand) {
        mSkin.setSecondaryWeaponVisuals(
            nullptr, nullptr, sf::IntRect({0, 0}, {0, 0}),
            sf::IntRect({0, 0}, {0, 0}), ItemQuality::Common, 1.f, {0.f, 0.f});
      }
    }
  } else if (slot == EquipmentSlot::Head || slot == EquipmentSlot::Chest || slot == EquipmentSlot::Hands || slot == EquipmentSlot::Feet) {
    if (mResourceManager) {
      updateArmorVisuals(*mResourceManager);
    } else {
      mSkin.setArmorVisuals(slot, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f);
    }
  }
  recalculateStats();
}

void Player::equipWeapon(std::shared_ptr<Item> item, ResourceManager &res, int slotIndex) {
  EquipmentSlot slot = (slotIndex == 1) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
  equipItem(item, slot, res);
}

void Player::unequipWeapon(int slotIndex) {
  EquipmentSlot slot = (slotIndex == 1) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
  unequipItem(slot);
}

void Player::updateWeaponVisuals(ResourceManager &res) {
  auto mainHandWeapon = mEquipment[static_cast<int>(EquipmentSlot::MainHand)];
  auto offHandWeapon = mEquipment[static_cast<int>(EquipmentSlot::OffHand)];
  std::shared_ptr<Item> twoHandedWeapon = nullptr;
  if (mainHandWeapon && mainHandWeapon->gripType == GripType::TwoHanded) {
    twoHandedWeapon = mainHandWeapon;
  } else if (offHandWeapon &&
             offHandWeapon->gripType == GripType::TwoHanded) {
    twoHandedWeapon = offHandWeapon;
  }

  if (mSkin.getAnimation()) {
    float prevWeight = mSkin.getAnimation()->getEquippedWeightFactor();
    float weightFactor = 1.0f; // Base weight
    if (twoHandedWeapon) {
      weightFactor += 2.0f;
    } else if (mainHandWeapon && offHandWeapon) {
      weightFactor += 1.4f;
    } else if (mainHandWeapon || offHandWeapon) {
      weightFactor += 0.8f;
    }

    for (int i = 0; i < static_cast<int>(EquipmentSlot::Count); ++i) {
      auto armor = mEquipment[i];
      if (armor && armor->type == ItemType::Armor) {
        weightFactor += 0.4f;
      }
    }
    mSkin.getAnimation()->setEquippedWeightFactor(weightFactor);
    float deltaWeight = std::abs(weightFactor - prevWeight);
    if (deltaWeight > 0.1f) {
      mSkin.getAnimation()->applyEquipWeightImpact(deltaWeight);
    }
  }

  if (twoHandedWeapon) {
    try {
      std::string texPath = twoHandedWeapon->texturePath.empty()
                                ? "assets/items/weapons/weapons-base.png"
                                : twoHandedWeapon->texturePath;
      const sf::Texture &baseTex = res.getTexture(texPath);
      const sf::Texture &layoutTex =
          res.getTexture("assets/items/weapons/weapons_layout.png");

      sf::IntRect overlayRect({0, 0}, {0, 0});
      if (twoHandedWeapon->textureRect.size.x == 16 &&
          twoHandedWeapon->textureRect.size.y == 16) {
        if (twoHandedWeapon->overlayGridCoords.x >= 0 &&
            twoHandedWeapon->overlayGridCoords.y >= 0) {
          overlayRect = sf::IntRect({twoHandedWeapon->overlayGridCoords.x * 16,
                                     twoHandedWeapon->overlayGridCoords.y * 16},
                                    {16, 16});
        } else {
          overlayRect = twoHandedWeapon->textureRect;
        }
      }

      mSkin.setWeaponVisuals(&baseTex, &layoutTex, twoHandedWeapon->textureRect,
                             overlayRect, twoHandedWeapon->quality,
                             twoHandedWeapon->scale, twoHandedWeapon->offset,
                             true, twoHandedWeapon->fortificationLevel);
    } catch (...) {
      std::cout << "[Player] Error loading two-handed visuals.\n";
    }
    mSkin.setSecondaryWeaponVisuals(
        nullptr, nullptr, sf::IntRect({0, 0}, {0, 0}),
        sf::IntRect({0, 0}, {0, 0}), ItemQuality::Common, 1.f, {0.f, 0.f});
  } else {
    if (mainHandWeapon) {
      try {
        std::string texPath = mainHandWeapon->texturePath.empty()
                                  ? "assets/items/weapons/weapons-base.png"
                                  : mainHandWeapon->texturePath;
        auto isShield = [](const std::shared_ptr<Item>& item) {
          return item && item->isShield();
        };

        sf::IntRect baseRect = mainHandWeapon->textureRect;
        sf::IntRect overlayRect({0, 0}, {0, 0});

        int row = 0;
        if (isShield(mainHandWeapon)) {
          texPath = "assets/items/shields/16x16x10_escudos.png";
          int col = (mainHandWeapon->textureRect.size.x > 0) ? (mainHandWeapon->textureRect.position.x / 16) : 0;
          if (isGuardActive() || mSkin.isShieldAttacking()) {
            row = 2; // (X: 0..15, Y: 32..47) activo (tecla E o ataque con escudo)
          } else {
            // Mano Derecha: si mira a D (1) -> mano delantera (row 0), si mira a A (-1) -> mano trasera (row 1)
            row = (mFacingDir == 1) ? 0 : 1;
          }
          baseRect = sf::IntRect({col * 16, row * 16}, {16, 16});
          overlayRect = sf::IntRect({0, 0}, {0, 0});
          std::cout << "[ShieldVisuals] Mano Derecha (slot 4) | Facing: " << (mFacingDir == 1 ? "D (Derecha)" : "A (Izquierda)")
                    << " | Guard: " << (isGuardActive() ? "ON" : "OFF")
                    << " -> Atlas Tile: (col=" << col << ", row=" << row << ") [Y=" << row * 16 << "]\n";
        }

        const sf::Texture &baseTex = res.getTexture(texPath);
        const sf::Texture &layoutTex =
            res.getTexture("assets/items/weapons/weapons_layout.png");

        if (!isShield(mainHandWeapon) && mainHandWeapon->textureRect.size.x == 16 &&
            mainHandWeapon->textureRect.size.y == 16) {
          if (mainHandWeapon->overlayGridCoords.x >= 0 &&
              mainHandWeapon->overlayGridCoords.y >= 0) {
            overlayRect =
                sf::IntRect({mainHandWeapon->overlayGridCoords.x * 16,
                             mainHandWeapon->overlayGridCoords.y * 16},
                            {16, 16});
          } else {
            overlayRect = mainHandWeapon->textureRect;
          }
        }

        sf::Vector2f activeOffset = (isShield(mainHandWeapon) && (isGuardActive() || mSkin.isShieldAttacking())) ? mainHandWeapon->guardOffset : mainHandWeapon->offset;
        bool shieldOverHand = isShield(mainHandWeapon);

        mSkin.setWeaponVisuals(&baseTex, &layoutTex,
                               baseRect, overlayRect,
                               mainHandWeapon->quality, mainHandWeapon->scale,
                               activeOffset, false,
                               mainHandWeapon->fortificationLevel,
                               isShield(mainHandWeapon), shieldOverHand);
      } catch (...) {
        std::cout << "[Player] Error loading main hand visuals.\n";
      }
    } else {
      mSkin.setWeaponVisuals(nullptr, nullptr, sf::IntRect({0, 0}, {0, 0}),
                             sf::IntRect({0, 0}, {0, 0}), ItemQuality::Common,
                             1.f, {0.f, 0.f}, false);
    }

    if (offHandWeapon) {
      try {
        std::string texPath = offHandWeapon->texturePath.empty()
                                  ? "assets/items/weapons/weapons-base.png"
                                  : offHandWeapon->texturePath;
        auto isShield = [](const std::shared_ptr<Item>& item) {
          return item && item->isShield();
        };

        sf::IntRect baseRect = offHandWeapon->textureRect;
        sf::IntRect overlayRect({0, 0}, {0, 0});

        int row = 0;
        if (isShield(offHandWeapon)) {
          texPath = "assets/items/shields/16x16x10_escudos.png";
          int col = (offHandWeapon->textureRect.size.x > 0) ? (offHandWeapon->textureRect.position.x / 16) : 0;
          if (isGuardActive() || mSkin.isShieldAttacking()) {
            row = 2; // (X: 0..15, Y: 32..47) activo (tecla E o ataque con escudo)
          } else {
            // Mano Izquierda: si mira a D (1) -> trasera (row 1), si mira a A (-1) -> delantera (row 0)
            row = (mFacingDir == 1) ? 1 : 0;
          }
          baseRect = sf::IntRect({col * 16, row * 16}, {16, 16});
          overlayRect = sf::IntRect({0, 0}, {0, 0});
          std::cout << "[ShieldVisuals] Mano Izquierda (slot 6) | Facing: " << (mFacingDir == 1 ? "D (Derecha)" : "A (Izquierda)")
                    << " | Guard: " << (isGuardActive() ? "ON" : "OFF")
                    << " -> Atlas Tile: (col=" << col << ", row=" << row << ") [Y=" << row * 16 << "]\n";
        }

        const sf::Texture &baseTex = res.getTexture(texPath);
        const sf::Texture &layoutTex =
            res.getTexture("assets/items/weapons/weapons_layout.png");

        if (!isShield(offHandWeapon) && offHandWeapon->textureRect.size.x == 16 &&
            offHandWeapon->textureRect.size.y == 16) {
          if (offHandWeapon->overlayGridCoords.x >= 0 &&
              offHandWeapon->overlayGridCoords.y >= 0) {
            overlayRect =
                sf::IntRect({offHandWeapon->overlayGridCoords.x * 16,
                             offHandWeapon->overlayGridCoords.y * 16},
                            {16, 16});
          } else {
            overlayRect = offHandWeapon->textureRect;
          }
        }

        sf::Vector2f activeOffset = (isShield(offHandWeapon) && (isGuardActive() || mSkin.isShieldAttacking())) ? offHandWeapon->guardOffset : offHandWeapon->offset;
        bool shieldOverHand = isShield(offHandWeapon);

        mSkin.setSecondaryWeaponVisuals(
            &baseTex, &layoutTex, baseRect, overlayRect,
            offHandWeapon->quality, offHandWeapon->scale,
            activeOffset, offHandWeapon->fortificationLevel, isShield(offHandWeapon), shieldOverHand);
      } catch (...) {
        std::cout << "[Player] Error loading secondary hand visuals.\n";
      }
    } else {
      mSkin.setSecondaryWeaponVisuals(
          nullptr, nullptr, sf::IntRect({0, 0}, {0, 0}),
          sf::IntRect({0, 0}, {0, 0}), ItemQuality::Common, 1.f, {0.f, 0.f});
    }
  }
}

void Player::setGuardActive(bool active) {
  bool wasActive = isGuardActive();
  Entity::setGuardActive(active);
  if (mResourceManager) {
    updateWeaponVisuals(*mResourceManager);
  }
  if (!wasActive && active) {
    if (auto *ss = SoundSystem::getInstance()) {
      ss->playSound("assets/sounds/escudo_up.wav");
    }
  }
}

void Player::updateArmorVisuals(ResourceManager &res) {
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
        const sf::Texture &baseTex = res.getTexture(texPath);
        mSkin.setArmorVisuals(slot, &baseTex, item->textureRect, item->offset, item->scale, item->fortificationLevel);
      } catch (...) {
        std::cout << "[Player] Error loading armor visuals for slot " << static_cast<int>(slot) << "\n";
      }
    } else {
      mSkin.setArmorVisuals(slot, nullptr, sf::IntRect({0, 0}, {0, 0}), {0.f, 0.f}, 1.0f, 0);
    }
  }
}

void Player::emitWeaponGibs(GoreSystem &gore, float floorY,
                            sf::Vector2f sourcePos, float forceMultiplier) {
  mSkin.emitWeaponGibs(gore, floorY, sourcePos, forceMultiplier);
}
