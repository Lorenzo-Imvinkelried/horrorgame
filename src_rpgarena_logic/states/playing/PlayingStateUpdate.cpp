#include "PlayingState.h"
#include "Config.h"
#include "entities/mob/Mob.h"
#include "core/systems/WeightSystem.h"
#include "core/systems/WindSystem.h"
#include "core/systems/InteractionSystem.h"
#include "utils/CombatCalculator.h"
#include "utils/Random.h"
#include "core/skills/mage/basic-orb.h"
#include "core/skills/active/TotemHeal_1/TotemHeal_1.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

void PlayingState::update(Game &game, sf::Time dt) {
  if (!mInitialized) {
    initializeStep(game, mInitStep);
    mInitStep++;
    if (mInitStep >= 10) {
      mInitialized = true;
      mIsTransitioning = false;
    }
    return;
  }

  if (mIsTransitioning)
    return;

  mTerrainDeform.update(dt);
  WindSystem::get().update(dt);
  mDashSystem.update(dt, &mParticleSystem, &mWorldManager.getMap(), &mTerrainDeform);

  size_t activeMobs = mEntityManager.getActiveEntities().size();
  if (activeMobs > 0)
    activeMobs -= 1;
  size_t renderCount = mRenderSystem.getRenderedCount();

  mUIManager.update(game, dt, mPlayerPtr, (int)activeMobs, (int)renderCount,
                    (int)mTerrainDeform.getDebugActiveVisualChunks());

  const auto &input = game.getInput();

  if (dt.asSeconds() > 0.1f)
    dt = sf::seconds(0.1f);

  mArrowTimer += dt.asSeconds();

  if (mPlayerPtr) {
    if (mPlayerPtr->getCurrentHp() > 0) {
      if (!mUIManager.isChatFocused()) {
        mPlayerPtr->handleInput(input, dt);
      } else {
        mPlayerPtr->stopMovement();
      }
    } else {
      if (mPlayerController.getTargetedEntity())
        mPlayerController.setTargetedEntity(nullptr);
      if (mCombatSystem.getCurrentTarget())
        mCombatSystem.setCombatTarget(nullptr);
    }
  }

  if (mPlayerPtr) {
    if (!mPlayerPtr->isAlive()) {
        game.getPostFX().setDesaturateTarget(1.f);
    } else {
        game.getPostFX().setDesaturateTarget(0.f);
    }

    mPlayerPtr->setTerrainDeform(
        mTerrainDeform.isInitialized() ? &mTerrainDeform : nullptr);
    mPlayerPtr->setDepthOffset(0.f);
    mEntityManager.update(dt, mView.getCenter(), mView.getSize(),
                          mTerrainDeform.isInitialized() ? &mTerrainDeform
                                                         : nullptr);
    mAggroSystem.update(dt, mPlayerPtr, mEntityManager.getActiveEntities(), &mCombatSystem, &mUIManager.getFXSystem());
    mPlayerPtr->setCombatState(mCombatSystem.isInCombat());

    // Colisiones Player
    mCollisionCache.clear();
    mWorldManager.getDecorSystem().getObstaclesNearby(mPlayerPtr->getPosition(),
                                                      mCollisionCache);
    mPlayerPtr->handleEnvironmentCollisions(mCollisionCache);

    // Walk Particles
    if (mPlayerPtr->isAlive() && mPlayerPtr->isMoving()) {
      mDustTimer += dt.asSeconds();
      if (mDustTimer > 0.12f) {
        mDustTimer = 0.f;

        sf::Vector2f leftFoot = mPlayerPtr->getLeftFootPosition();
        sf::Vector2f rightFoot = mPlayerPtr->getRightFootPosition();

        sf::Color colL = mWorldManager.getMap().getColorAtWorldPos(leftFoot);
        sf::Color colR = mWorldManager.getMap().getColorAtWorldPos(rightFoot);

        if (mTerrainDeform.isInitialized()) {
          colL = mTerrainDeform.getDeformedColorAt(leftFoot, colL);
          colR = mTerrainDeform.getDeformedColorAt(rightFoot, colR);
        }

        if (colL != sf::Color::Transparent)
          mParticleSystem.emitWalkDust(leftFoot, colL, mPlayerPtr);
        if (colR != sf::Color::Transparent)
          mParticleSystem.emitWalkDust(rightFoot, colR, mPlayerPtr);
      }
    } else {
      mDustTimer = 0.12f;
    }

    // Footstep audio & terrain stamps
    if (mPlayerPtr->isAlive()) {
      float playerWeight = mPlayerPtr->getWeightKg();
      float footDepth = WeightSystem::calcFootprintDepth(playerWeight);

      if (mPlayerPtr->didLeftFootLand()) {
        mSoundSystem.playSound("assets/sounds/barefoot_step.wav", cfg::Audio::FOOTSTEP_VOLUME, false);
        if (mTerrainDeform.isInitialized() && playerWeight > 0.0f) {
          TerrainDeformSystem::FootprintStamp stamp;
          stamp.pos = mPlayerPtr->getLandedLeftFootPos();
          stamp.rotDeg = mPlayerPtr->getLandedLeftFootRot();
          stamp.scale = mPlayerPtr->getLandedLeftFootScale();
          stamp.origin = mPlayerPtr->getLandedLeftFootOrigin();
          stamp.depthPx = footDepth;
          stamp.customTex = mPlayerPtr->getFootprintTexture();
          stamp.customImg = mPlayerPtr->getFootprintImage();
          mTerrainDeform.queueFootprint(stamp);
        }
      }
      if (mPlayerPtr->didRightFootLand()) {
        mSoundSystem.playSound("assets/sounds/barefoot_step.wav", cfg::Audio::FOOTSTEP_VOLUME, false);
        if (mTerrainDeform.isInitialized() && playerWeight > 0.0f) {
          TerrainDeformSystem::FootprintStamp stamp;
          stamp.pos = mPlayerPtr->getLandedRightFootPos();
          stamp.rotDeg = mPlayerPtr->getLandedRightFootRot();
          stamp.scale = mPlayerPtr->getLandedRightFootScale();
          stamp.origin = mPlayerPtr->getLandedRightFootOrigin();
          stamp.depthPx = footDepth;
          stamp.customTex = mPlayerPtr->getFootprintTexture();
          stamp.customImg = mPlayerPtr->getFootprintImage();
          mTerrainDeform.queueFootprint(stamp);
        }
      }
    }

    // Mob footsteps
    sf::Vector2f playerPos = mPlayerPtr->getPosition();
    float maxAudioDist = cfg::Audio::MOB_FOOTSTEP_MAX_DISTANCE;

    for (auto &entity : mEntityManager.getActiveEntities()) {
      Mob *mob = dynamic_cast<Mob *>(entity.get());
      if (!mob || !mob->isAlive())
        continue;

      float mobWeight = mob->getWeightKg();
      float mobDepth = WeightSystem::calcFootprintDepth(mobWeight);

      float mobVol = 0.0f;
      if (maxAudioDist > 0.0f) {
        sf::Vector2f mobPos = mob->getPosition();
        float dx = mobPos.x - playerPos.x;
        float dy = mobPos.y - playerPos.y;
        float distSq = dx * dx + dy * dy;
        if (distSq < maxAudioDist * maxAudioDist) {
          float dist = std::sqrt(distSq);
          float factor = 1.0f - (dist / maxAudioDist);
          mobVol = cfg::Audio::FOOTSTEP_VOLUME * factor;
        }
      }

      if (mob->didLeftFootLand()) {
        if (mob->getBlueprintName() == "goblin" && mobVol > 0.0f) {
          mSoundSystem.playSound("assets/sounds/barefoot_step.wav", mobVol, false);
        }
        if (mTerrainDeform.isInitialized() && mobWeight > 0.0f) {
          TerrainDeformSystem::FootprintStamp stamp;
          stamp.pos = mob->getLandedLeftFootPos();
          stamp.rotDeg = mob->getLandedLeftFootRot();
          stamp.scale = mob->getLandedLeftFootScale();
          stamp.origin = mob->getLandedLeftFootOrigin();
          stamp.depthPx = mobDepth;
          stamp.customTex = mob->getFootprintTexture();
          stamp.customImg = mob->getFootprintImage();
          mTerrainDeform.queueFootprint(stamp);
        }
      }
      if (mob->didRightFootLand()) {
        if (mob->getBlueprintName() == "goblin" && mobVol > 0.0f) {
          mSoundSystem.playSound("assets/sounds/barefoot_step.wav", mobVol, false);
        }
        if (mTerrainDeform.isInitialized() && mobWeight > 0.0f) {
          TerrainDeformSystem::FootprintStamp stamp;
          stamp.pos = mob->getLandedRightFootPos();
          stamp.rotDeg = mob->getLandedRightFootRot();
          stamp.scale = mob->getLandedRightFootScale();
          stamp.origin = mob->getLandedRightFootOrigin();
          stamp.depthPx = mobDepth;
          stamp.customTex = mob->getFootprintTexture();
          stamp.customImg = mob->getFootprintImage();
          mTerrainDeform.queueFootprint(stamp);
        }
      }
    }

    // Item Drops Update & Pickup
    sf::Vector2f wMouse =
        game.getWindow().mapPixelToCoords(input.getMousePosition(), mView);
    mItemDrops.update(dt, mPlayerPtr->getPosition(), wMouse, game.getResources());

    const DroppedItem *drop = mItemDrops.getHoveredItem();
    mUIManager.getHud().setHoveredWorldItem(drop ? drop->item : nullptr);

    mItemDrops.tryPickup(mPlayerPtr, mPlayerPtr->getPosition(), wMouse, input,
                         [this](std::shared_ptr<Item> item) -> bool {
                           return mUIManager.getHud().addItemToInventory(item);
                         });
  }

  bool inCombat = mCombatSystem.isInCombat();
  if (mWasInCombat && !inCombat) {
    mTargetedEntity = nullptr;
  }
  mWasInCombat = inCombat;

  mWorldManager.update(dt);
  mPlayerController.update(dt);

  // Auto-deselect target if it goes outside view
  if (mPlayerPtr) {
    Entity *currentTarget = mPlayerController.getTargetedEntity();
    if (!currentTarget)
      currentTarget = mCombatSystem.getCurrentTarget();

    if (currentTarget && currentTarget != mPlayerPtr) {
      sf::Vector2f tPos = currentTarget->getPosition();
      sf::Vector2f viewSize = mView.getSize();
      sf::Vector2f viewCenter = mView.getCenter();

      float margin = cfg::Combat::AUTO_DESELECT_MARGIN;
      sf::FloatRect targetViewRect(
          {viewCenter.x - viewSize.x / 2.f - margin,
           viewCenter.y - viewSize.y / 2.f - margin},
          {viewSize.x + margin * 2.f, viewSize.y + margin * 2.f});

      if (!targetViewRect.contains(tPos)) {
        mPlayerController.setTargetedEntity(nullptr);
        mCombatSystem.setCombatTarget(nullptr);
        mPlayerPtr->clearFollowTargetIfMatches(currentTarget);
      }
    }
  }

  if (mPlayerController.hasExplosionClick() && mTerrainDeform.isInitialized()) {
    mTerrainDeform.queueExplosion(mPlayerController.consumeExplosionClick());
  }

  mCombatSystem.update(dt);
  BasicOrb::updateAll(dt.asSeconds(), mEntityManager, mCombatSystem, mParticleSystem, game.getResources(), &mWorldManager);
  TotemHeal_1::updateAll(dt.asSeconds(), mPlayerPtr, mCombatSystem.getFeedback(), mParticleSystem, game.getResources(), &mWorldManager);

  if (mPlayerPtr && mPlayerPtr->isAlive() && mPlayerPtr->hasBuffFromSkill(2)) {
      static float furyParticleTimer = 0.f;
      furyParticleTimer += dt.asSeconds();
      if (furyParticleTimer >= 0.08f) {
          furyParticleTimer = 0.f;
          mParticleSystem.emitBerserkerFury(mPlayerPtr, 2);
      }
  }

  mParticleSystem.update(dt);
  mGoreSystem.update(dt);

  for (const auto& drop : mGoreSystem.getLandedDrops()) {
    if (drop.item) {
        std::cout << "[DEBUG_STATE] Collected landed drop from GoreSystem: " << drop.item->name 
                  << " [ID: " << drop.item->id << "] at position " << drop.position.x << ", " << drop.position.y 
                  << " (isArmor: " << drop.isArmor << ")" << std::endl;
    }
    mItemDrops.dropItem(drop.item, drop.position, game.getResources(),
                        {0.f, 0.f}, 0.f, 0.f, {1.f, 1.f},
                        drop.isArmor, drop.customTexture, &drop.customVertices);
  }
  mGoreSystem.clearLandedDrops();

  // Player Death Gibs
  if (mPlayerPtr && !mPlayerPtr->isAlive() && !mPlayerDeathTriggered) {
    mPlayerDeathTriggered = true;

    mUIManager.getHud().closeAllPanels(mPlayerPtr);
    mUIManager.closeInspection();

    Entity *killer = mPlayerPtr->getLastAttacker();
    sf::Vector2f killerPos =
        killer ? killer->getPosition() : sf::Vector2f{0.f, 0.f};

    float forceMult = 0.15f;
    sf::Vector2f goreSourcePos = {0.f, 0.f};
    if (mPlayerPtr->wasLastHitDirectAttack()) {
      forceMult = CombatCalculator::calculateDeathKnockbackMultiplier(
          killer, mPlayerPtr);
      goreSourcePos = killerPos;
    }

    std::vector<std::shared_ptr<Item>> armorItems(12, nullptr);
    armorItems[static_cast<size_t>(EquipmentSlot::Head)] = mPlayerPtr->getEquippedItem(EquipmentSlot::Head);
    armorItems[static_cast<size_t>(EquipmentSlot::Chest)] = mPlayerPtr->getEquippedItem(EquipmentSlot::Chest);
    armorItems[static_cast<size_t>(EquipmentSlot::Hands)] = mPlayerPtr->getEquippedItem(EquipmentSlot::Hands);
    armorItems[static_cast<size_t>(EquipmentSlot::Feet)] = mPlayerPtr->getEquippedItem(EquipmentSlot::Feet);

    float deathSortY = mPlayerPtr->getLayerSortingY(2);
    mPlayerPtr->getSkin().emitGibs(mGoreSystem, mPlayerPtr->getGoreFloorY(),
                                   goreSourcePos, forceMult,
                                   mPlayerPtr->getVelocity(), armorItems, deathSortY);

    for (int slot = 0; slot < 2; ++slot) {
      auto weapon = mPlayerPtr->getWeapon(slot);
      if (weapon) {
        auto anim = mPlayerPtr->getAnimation();
        auto info = anim ? anim->getWeaponSpawnInfo(slot) : Animation::WeaponSpawnInfo{};
        if (info.exists) {
          float floorY = mPlayerPtr->getGoreFloorY() - 10.f;
          float heightAboveGround = std::max(0.f, floorY - info.position.y);
          float upwardBoost = cfg::Gore::UPWARD_BOOST_BASE - heightAboveGround * cfg::Gore::HEIGHT_MULTIPLIER;

          float angleRad = Random::Float(0.f, 6.28f);
          if (goreSourcePos != sf::Vector2f{0.f, 0.f}) {
            sf::Vector2f diff = info.position - goreSourcePos;
            float baseAngle = std::atan2(diff.y, diff.x);
            angleRad = baseAngle + Random::Float(-0.52f, 0.52f);
          }
          float hSpeed = Random::Float(cfg::Gore::H_SPEED_MIN, cfg::Gore::H_SPEED_MAX) * forceMult;
          float verticalForceMult = 1.0f + (forceMult - 1.0f) * 0.3f;
          sf::Vector2f initialVelocity = {std::cos(angleRad) * hSpeed, upwardBoost * verticalForceMult};

          mItemDrops.dropItem(weapon, info.position, game.getResources(),
                              initialVelocity, floorY, info.rotation, info.scale,
                              false, nullptr, nullptr, true);
        } else {
          mItemDrops.dropItem(weapon, mPlayerPtr->getGroundPosition() - sf::Vector2f(0.f, 8.f), game.getResources(),
                              {0.f, 0.f}, -9999.f, 0.f, {1.5f, 1.5f},
                              false, nullptr, nullptr, true);
        }
      }
    }

    mPlayerPtr->setIsVisible(false);
  }

  // Cleanup dead entities
  mEntityManager.cleanupDeadEntities([this, &game](Entity *deadEntity) {
    if (deadEntity == mPlayerController.getTargetedEntity())
      mPlayerController.setTargetedEntity(nullptr);
    if (deadEntity == mCombatSystem.getCurrentTarget())
      mCombatSystem.setCombatTarget(nullptr);
    if (mPlayerPtr) {
      mPlayerPtr->clearFollowTargetIfMatches(deadEntity);
      mPlayerPtr->removeFromAggro(deadEntity);
    }

    mUIManager.notifyEntityDeath(deadEntity);
    mCombatSystem.onEntityDeath(deadEntity);
    mParticleSystem.onEntityDeath(deadEntity);

    if (Mob *mob = dynamic_cast<Mob *>(deadEntity)) {
      Entity *killer = deadEntity->getLastAttacker();
      sf::Vector2f killerPos = killer ? killer->getPosition() : sf::Vector2f{0.f, 0.f};

      std::cout << "[DEBUG_DEATH] Mob " << mob->getName() << " (Level " << mob->getLevel() << ") is dying." << std::endl;

      float forceMult = 0.15f;
      sf::Vector2f goreSourcePos = {0.f, 0.f};
      if (deadEntity->wasLastHitDirectAttack()) {
        forceMult = CombatCalculator::calculateDeathKnockbackMultiplier(killer, deadEntity);
        goreSourcePos = killerPos;
      }

      auto launchAndDropItem = [&](std::shared_ptr<Item> item) {
          if (!item) return;

          float floorY = deadEntity->getGoreFloorY() - 10.f;
          sf::Vector2f startPos = mob->getGroundPosition() - sf::Vector2f(0.f, 16.f);
          sf::Vector2f startScale = {item->scale, item->scale};
          float startRotation = 0.f;

          if (item == mob->getWeapon(0)) {
              auto info = mob->getWeaponSpawnInfo(0);
              if (info.exists) {
                  startPos = info.position;
                  startScale = info.scale;
                  startRotation = info.rotation;
              }
          } else if (item == mob->getWeapon(1)) {
              auto info = mob->getWeaponSpawnInfo(1);
              if (info.exists) {
                  startPos = info.position;
                  startScale = info.scale;
                  startRotation = info.rotation;
              }
          } else {
              std::string nodeName = "";
              if (item->slotType == EquipmentSlot::Head) nodeName = "head";
              else if (item->slotType == EquipmentSlot::Chest) nodeName = "body";
              else if (item->slotType == EquipmentSlot::Hands) nodeName = "hand_l";
              else if (item->slotType == EquipmentSlot::Feet) nodeName = "foot_l";

              if (!nodeName.empty()) {
                  sf::Vector2f pos = mob->getSkin().getNodePosition(nodeName);
                  if (pos != sf::Vector2f{0.f, 0.f}) {
                      startPos = pos;
                  }
              }
          }

          float heightAboveGround = std::max(0.f, floorY - startPos.y);
          float upwardBoost = cfg::Gore::UPWARD_BOOST_BASE - heightAboveGround * cfg::Gore::HEIGHT_MULTIPLIER;

          float angleRad = Random::Float(0.f, 6.28f);
          if (goreSourcePos != sf::Vector2f{0.f, 0.f}) {
              sf::Vector2f diff = startPos - goreSourcePos;
              float baseAngle = std::atan2(diff.y, diff.x);
              angleRad = baseAngle + Random::Float(-0.52f, 0.52f);
          }
          float hSpeed = Random::Float(cfg::Gore::H_SPEED_MIN, cfg::Gore::H_SPEED_MAX) * forceMult;
          float verticalForceMult = 1.0f + (forceMult - 1.0f) * 0.3f;
          sf::Vector2f initialVelocity = {std::cos(angleRad) * hSpeed, upwardBoost * verticalForceMult};

          mItemDrops.dropItem(item, startPos, game.getResources(),
                              initialVelocity, floorY, startRotation, startScale);
      };

      std::vector<std::shared_ptr<Item>> itemsToDrop;
      
      for (int slotIdx = 0; slotIdx < 12; ++slotIdx) {
          auto slot = static_cast<EquipmentSlot>(slotIdx);
          auto item = mob->getEquippedItem(slot);
          if (item) {
              bool isArmorSlot = (slot == EquipmentSlot::Head ||
                                  slot == EquipmentSlot::Chest || slot == EquipmentSlot::Hands ||
                                  slot == EquipmentSlot::Feet);
              if (isArmorSlot) {
                  std::cout << "  [DEBUG_DEATH] Equipped armor in slot " << slotIdx << ": " << item->name 
                            << " [ID: " << item->id << "], LOOT_ARMOR_ATTACHED = " 
                            << (cfg::Gore::LOOT_ARMOR_ATTACHED ? "true" : "false") << std::endl;
                  if (!cfg::Gore::LOOT_ARMOR_ATTACHED) {
                      itemsToDrop.push_back(item);
                  }
              } else {
                  std::cout << "  [DEBUG_DEATH] Equipped non-armor item in slot " << slotIdx << ": " << item->name 
                            << " [ID: " << item->id << "] (will drop directly)" << std::endl;
                  itemsToDrop.push_back(item);
              }
          }
      }

      auto extraDrops = game.getLootManager().rollLoot(mob->getBlueprintName(), mob->getLevel());
      if (!extraDrops.empty()) {
          std::cout << "  [DEBUG_DEATH] Loot table rolled " << extraDrops.size() << " extra drop(s)." << std::endl;
          itemsToDrop.insert(itemsToDrop.end(), extraDrops.begin(), extraDrops.end());
      }

      uint64_t rolledGold = game.getGoldSystem().rollGold(mob->getBlueprintName(), mWorldManager.getCurrentWorldID(), mob->getLevel());
      if (rolledGold > 0 && mPlayerPtr) {
          mPlayerPtr->addBronzeCoins(rolledGold);
          std::cout << "  [DEBUG_DEATH] Awarded " << rolledGold << " bronze coins to player.\n";
      }
      
      for (auto& item : itemsToDrop) {
          launchAndDropItem(item);
      }

      if (deadEntity != mPlayerPtr) {
        float deathSortY = deadEntity->getLayerSortingY(2);
        std::vector<sf::Vertex> parts;
        const sf::Texture *tex = nullptr;
        deadEntity->getRenderData(parts, tex);

        std::vector<std::shared_ptr<Item>> armorItems(12, nullptr);
        if (cfg::Gore::LOOT_ARMOR_ATTACHED) {
            armorItems[static_cast<size_t>(EquipmentSlot::Head)] = mob->getEquippedItem(EquipmentSlot::Head);
            armorItems[static_cast<size_t>(EquipmentSlot::Chest)] = mob->getEquippedItem(EquipmentSlot::Chest);
            armorItems[static_cast<size_t>(EquipmentSlot::Hands)] = mob->getEquippedItem(EquipmentSlot::Hands);
            armorItems[static_cast<size_t>(EquipmentSlot::Feet)] = mob->getEquippedItem(EquipmentSlot::Feet);
        }

        if (tex && !parts.empty()) {
          mGoreSystem.emitGibs(
              parts, tex, deadEntity->getGoreFloorY(), goreSourcePos, forceMult,
              deadEntity->getNodeNames(), deadEntity->getVelocity(), deadEntity->getAnimation(), armorItems, mob ? mob->getBlueprint().type : "", deathSortY);
        } else {
          std::cout << "  [DEBUG_DEATH] WARNING: Cannot emit vertex gibs (texture or parts empty). Dropping armor items directly." << std::endl;
          for (const auto& item : armorItems) {
              if (item) {
                  std::cout << "    [DEBUG_DEATH] Fallback direct drop for armor item: " << item->name << " [ID: " << item->id << "]" << std::endl;
                  launchAndDropItem(item);
              }
          }
          if (sf::Sprite *s = deadEntity->getSprite()) {
            mGoreSystem.emitGibs(*s, deadEntity->getGoreFloorY(), goreSourcePos,
                                 forceMult, deadEntity->getVelocity(), deathSortY);
          }
        }

        deadEntity->emitWeaponGibs(mGoreSystem, deadEntity->getGoreFloorY(),
                                   goreSourcePos, forceMult);
      }
    }
  });

  // Camera Follow
  if (mPlayerPtr) {
    sf::Vector2f pPos = mPlayerPtr->getPosition();
    sf::Vector2u mapPx = mWorldManager.getMapSizePx();

    const float halfW = mView.getSize().x * 0.5f;
    const float halfH = mView.getSize().y * 0.5f;

    float targetX =
        std::clamp(pPos.x, halfW, static_cast<float>(mapPx.x) - halfW);
    float targetY =
        std::clamp(pPos.y, halfH, static_cast<float>(mapPx.y) - halfH);

    if (game.isUsingVirtualResolution()) {
      float roundedX = std::round(targetX - halfW) + halfW;
      float roundedY = std::round(targetY - halfH) + halfH;
      game.setVirtualOffset({roundedX - targetX, roundedY - targetY});
      mView.setCenter({roundedX, roundedY});
    } else {
      game.setVirtualOffset({0.f, 0.f});
      mView.setCenter({targetX, targetY});
    }
  }

  // Portal Collisions
  if (mPlayerPtr) {
    sf::FloatRect pBounds = mPlayerPtr->getGlobalBounds();
    const Portal *hitPortal = mWorldManager.checkPortalCollision(pBounds);
    if (hitPortal) {
      performLevelChange(*hitPortal, game);
      return;
    }
  }

  mUIManager.updateRTs(game, mWorldManager, mEntityManager, mPlayerPtr,
                       mPlayerController.getTargetedEntity(), &mTerrainDeform);
}
