#include "PlayerController.h"
#include "entities/player/Player.h"
#include "entities/mob/Mob.h" // [FIX] Required for dynamic_cast check
#include "core/systems/combat/CombatSystem.h"
#include "../../core/skills/SkillManager.h"
#include "../../core/managers/UIManager.h"
#include "../../core/managers/EntityManager.h"
#include "../../core/engine/Game.h"
#include "../../core/systems/ParticleSystem.h"
#include "../../core/systems/ShieldSystem.h"
#include "../../core/systems/DashSystem.h"
#include "../../core/skills/mage/basic-orb.h"
#include "../../core/skills/active/TotemHeal_1/TotemHeal_1.h"
#include "../../core/engine/ResourceManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>

PlayerController::PlayerController(CombatSystem& combat, SkillManager& skills, ParticleSystem& particles, EntityManager& entities, UIManager& ui)
    : mCombatSystem(combat), mSkillManager(skills), mParticleSystem(particles), mEntityManager(entities), mUIManager(ui) 
{
}

void PlayerController::setPlayer(Player* player) {
    mPlayerPtr = player;
}

// void PlayerController::setView(const sf::View& view) {
//     mView = &view;
// }

void PlayerController::setTargetedEntity(Entity* entity) {
    mTargetedEntity = entity;
    mTabTargetCandidates.clear();
    mTabTargetIndex = -1;
}

Entity* PlayerController::getTargetedEntity() const {
    return mTargetedEntity;
}

void PlayerController::reset() {
    mTargetedEntity = nullptr;
    mPendingSkillId = -1;
    mPendingSkillTarget = nullptr;
    mTargetingGroundSkillId = -1;
    if (mPlayerPtr) {
        mPlayerPtr->setFollowTarget(nullptr);
    }
    mTabTargetCandidates.clear();
    mTabTargetIndex = -1;
    mTabResetTimer = 0.f;
}


void PlayerController::update(sf::Time dt) {
    checkPendingSkill();
    if (mTabResetTimer > 0.f) {
        mTabResetTimer -= dt.asSeconds();
        if (mTabResetTimer <= 0.f) {
            mTabTargetCandidates.clear();
            mTabTargetIndex = -1;
        }
    }
}

void PlayerController::checkPendingSkill() {
    if (mPendingSkillId != -1 && mPlayerPtr) {
        if (!mEntityManager.isValid(mPendingSkillTarget) || !mPendingSkillTarget->isAlive() || mPendingSkillTarget->isReturningToSpawn()) {
             // Target Lost or Retreating
             mPendingSkillId = -1;
             mPendingSkillTarget = nullptr;
             mPlayerPtr->setFollowTarget(nullptr);
             if (mPlayerPtr->isCharging()) {
                 mPlayerPtr->setCharging(false);
             }
             std::cout << "[SKILL] Cancelled: Target lost or retreating.\n";
        }
        else {
             float dx = mPendingSkillTarget->getPosition().x - mPlayerPtr->getPosition().x;
             float dy = mPendingSkillTarget->getPosition().y - mPlayerPtr->getPosition().y;
             // [OPTIMIZATION] Use Squared Distance
             float distSq = dx*dx + dy*dy;

             const Skill *pendingSkill = mSkillManager.getSkill(mPendingSkillId);
      float range = mPlayerPtr->getAttackRange();
      if (pendingSkill && pendingSkill->range > 0 && pendingSkill->id != 4 && pendingSkill->id != 1 && pendingSkill->id != 5 && pendingSkill->id != 31) {
        range = (float)pendingSkill->range;
      }
      if (pendingSkill && pendingSkill->id == 4) {
        range = 30.f;
      }
             float effectiveRange = range + 10.f; // Tolerance
             float effectiveRangeSq = effectiveRange * effectiveRange; 
             
             // [SKILL VISUALS] Continuous Charge Effect while moving
             if (pendingSkill) {
                 if (!pendingSkill->canCast(mPlayerPtr)) {
                     mPendingSkillId = -1;
                     mPendingSkillTarget = nullptr;
                     mPlayerPtr->setFollowTarget(nullptr);
                     return;
                 }
                 const_cast<Skill*>(pendingSkill)->updateChargeVisuals(mPlayerPtr, &mParticleSystem);
             }

             if (distSq <= effectiveRangeSq) { // Tolerance +10 (Same as tryCastSkill)
                  // Reached target!
                  // Pay Cost (It wasn't paid on queue)
                  mPlayerPtr->restoreMana(-pendingSkill->manaCost);
                  float cdrPct = std::clamp(mPlayerPtr->getCooldownReductionPercent(), 0.f, 100.f);
                  mPlayerPtr->setSkillCooldown(pendingSkill->id, pendingSkill->cooldown * (1.f - cdrPct / 100.f), pendingSkill->cooldown);
                  
                  // Execute
                  if (pendingSkill->id == 4) {
                      mCombatSystem.performSkillAttack(pendingSkill, mPendingSkillTarget);
                  } else {
                      mCombatSystem.requestSkillAttack(pendingSkill, mPendingSkillTarget);
                  }

                  Entity* target = mPendingSkillTarget;

                  // Reset state
                  mPendingSkillId = -1;
                  mPendingSkillTarget = nullptr;
                  
                  // Select target on HUD but do not auto-attack or follow
                  mCombatSystem.setCombatTarget(target);
                  mPlayerPtr->setFollowTarget(nullptr);
             }
        }
    }
}

void PlayerController::tryCastSkill(int slotIndex) {
    if (!mPlayerPtr) return;
    
    // [STUN FIX] Prevent skill usage if stunned
    if (mPlayerPtr->isStunned()) return;

    int skillId = mPlayerPtr->getEquippedSkill(slotIndex);
    if (skillId == -1) return; // No skill

    const Skill* skill = mSkillManager.getSkill(skillId);
    if (!skill) return;

    // Requirement Validation (e.g. Shield required)
    std::string failReason;
    if (!skill->canCast(mPlayerPtr, &failReason)) {
        if (!failReason.empty() && FXSystem::getInstance()) {
            FXSystem::getInstance()->createFloatingText(
                mPlayerPtr->getPosition(),
                failReason,
                sf::Color(255, 90, 90),
                13,
                1.0f,
                -35.f
            );
        }
        return;
    }

    // Si ya estamos casteando esta habilidad y el jugador presiona su tecla de nuevo, se cancela el casteo
    if (mCombatSystem.isCastingSkill() && mCombatSystem.getPendingSkill() && mCombatSystem.getPendingSkill()->id == skillId) {
        std::cout << "[SKILL] Casteo de " << skill->name << " cancelado por el jugador.\n";
        mCombatSystem.cancelPendingAttack();
        return;
    }

    // Check Cooldown & Mana
    if (mPlayerPtr->isSkillReady(skillId) && mPlayerPtr->getCurrentMp() >= skill->manaCost) {
        
        // [INSTANT CANCEL] Cancel any previous basic attack animation or pending attack
        if (mPlayerPtr->isCharging()) {
            mPlayerPtr->setCharging(false);
        }
        mPendingSkillId = -1;
        mPendingSkillTarget = nullptr;
        mPlayerPtr->cancelAttackAnimation();
        mCombatSystem.cancelPendingAttack();

        // 1. Identify Target (Selection or Closest)
        Entity* target = mTargetedEntity;
        if (!mEntityManager.isValid(target) || !target->isAlive()) {
            target = mCombatSystem.getCurrentTarget(); // Fallback to current combat target
        }
        
        // [GROUND TARGETING LOGIC]
        if (skill->targetType == "GROUND") {
            mTargetingGroundSkillId = skillId;
            std::cout << "[SKILL] Activado modo de colocacion en el suelo para " << skill->name << "\n";
            return;
        }

        // [SELF BUFF LOGIC]
        if (skill->targetType == "SELF") {
            // Self-cast or Global
            target = mPlayerPtr; 
        }

    // [DYNAMIC TARGET TYPE CHECK]
    if (skill->targetType == "ENEMY") {
      if (target && target != mPlayerPtr && target->isAlive()) {
        if (target->isReturningToSpawn()) {
          std::cout << "[SKILL] Target is retreating. Ignored.\n";
          return;
        }

        float dx = target->getPosition().x - mPlayerPtr->getPosition().x;
        float dy = target->getPosition().y - mPlayerPtr->getPosition().y;
        float distSq = dx * dx + dy * dy;

        float range = (skill->range > 0 && skill->id != 1 && skill->id != 5 && skill->id != 31) ? (float)skill->range
                                         : mPlayerPtr->getAttackRange();
        if (skill->id == 4) {
            float maxRangeSq = (float)(skill->range * skill->range);
            if (distSq > maxRangeSq) {
                std::cout << "[SKILL] Objetivo fuera de rango para Embestida.\n";
                return;
            }
            range = 30.f; // Fixed range for strike/charge impact
        }
        float effectiveRange = range + 10.f;
        float effectiveRangeSq = effectiveRange * effectiveRange;

        if (distSq <= effectiveRangeSq) { // Tolerance
          // Execute
          if (mPlayerPtr && target) {
              mPlayerPtr->setFacingDir(target->getPosition().x >= mPlayerPtr->getPosition().x ? 1 : -1);
          }
          mPlayerPtr->restoreMana(-skill->manaCost);
          float cdrPct =
              std::clamp(mPlayerPtr->getCooldownReductionPercent(), 0.f, 100.f);
          mPlayerPtr->setSkillCooldown(skill->id, skill->cooldown *
                                                      (1.f - cdrPct / 100.f), skill->cooldown);
          const_cast<Skill *>(skill)->onQueue(mPlayerPtr, &mParticleSystem);
          if (skill->id == 4) {
            mCombatSystem.performSkillAttack(skill, target);
          } else {
            mCombatSystem.requestSkillAttack(skill, target);
          }
          mCombatSystem.setCombatTarget(target);
          mPlayerPtr->setFollowTarget(nullptr);
        } else {
          // Queue
          mPendingSkillId = skillId;
          mPendingSkillTarget = target;
          float skillRange = (skill->range > 0 && skill->id != 1 && skill->id != 5 && skill->id != 31) ? (float)skill->range
                                                : mPlayerPtr->getAttackRange();
          if (skill->id == 4) {
              skillRange = 30.f;
          }
          mPlayerPtr->setFollowTarget(target, skillRange);
          const_cast<Skill *>(skill)->onQueue(mPlayerPtr, &mParticleSystem);
          std::cout << "[SKILL] Queued " << skill->name << ".\n";
        }
      } else {
        std::cout << "[SKILL] Needs a target!\n";
      }
    } else {
      // [GENERIC / BUFF / SELF / DIRECTION]
      if (skill->targetType == "DIRECTION" && mPlayerPtr) {
          mPlayerPtr->setFacingDir(mLastMouseWorldPos.x >= mPlayerPtr->getPosition().x ? 1 : -1);
      }
      mPlayerPtr->restoreMana(-skill->manaCost);
      float cdrPct =
          std::clamp(mPlayerPtr->getCooldownReductionPercent(), 0.f, 100.f);
      mPlayerPtr->setSkillCooldown(skill->id,
                                   skill->cooldown * (1.f - cdrPct / 100.f), skill->cooldown);

      Skill *mutableSkill = const_cast<Skill *>(skill);
      if (auto* orbSkill = dynamic_cast<BasicOrb*>(mutableSkill)) {
          orbSkill->setCastTargetPosition(mLastMouseWorldPos);
      }

      if (skill->castTime > 0.f) {
          // [CAST TIME / CHANNELING]
          mCombatSystem.requestSkillAttack(skill, target ? target : mPlayerPtr);
      } else {
          // [INSTANT CAST]
          mutableSkill->onCastStart(mPlayerPtr, target ? target : mPlayerPtr,
                                    &mParticleSystem);
          mutableSkill->onExecute(mPlayerPtr, target ? target : mPlayerPtr,
                                  &mParticleSystem);

          mPlayerPtr->recalculateStats();
          mPlayerPtr->notifyStatsChanged();
      }
    }

    } else {
        if (!mPlayerPtr->isSkillReady(skillId)) {
            std::cout << "[DEBUG] Skill Cooldown!\n";
        } else {
            std::cout << "[DEBUG] No Mana (Cost " << skill->manaCost << ")!\n";
        }
    }
}

void PlayerController::handleInput(Game& game, sf::Time dt, const sf::View& view) {
    if (!mPlayerPtr) return;

    if (mPlayerPtr->isCharging()) {
        return; // Ignore inputs during charge
    }

    const auto& input = game.getInput(); 

    // [NEW] BLOCK ALL WORLD INPUT IF CHAT IS FOCUSED
    if (mUIManager.isChatFocused()) {
        if (input.isActionJustPressed(Action::Exit)) {
             game.getWindow().close();
        }
        return;
    }

    // [REFACTOR] Delegate Input to UIManager
    mUIManager.handleInput(game, dt, mPlayerPtr);

    if (input.isActionJustPressed(Action::Exit)) {
        game.getWindow().close();
    }

    // --- CORRECCIÓN COMBATE: CANCELAR AL MOVERSE ---
    if (input.isActionActive(Action::MoveUp) || 
        input.isActionActive(Action::MoveDown) ||
        input.isActionActive(Action::MoveLeft) || 
        input.isActionActive(Action::MoveRight)) 
    {
        mCombatSystem.notifyPlayerMoved();
        // Allow Run & Charge, only cancel on explicit commands if needed
    }

    // --- DASH (SHIFT) ---
    if (input.isActionJustPressed(Action::Dash)) {
        sf::Vector2f moveDir{0.f, 0.f};
        if (input.isActionActive(Action::MoveRight)) moveDir.x += 1.f;
        if (input.isActionActive(Action::MoveLeft))  moveDir.x -= 1.f;
        if (input.isActionActive(Action::MoveUp))    moveDir.y -= 1.f;
        if (input.isActionActive(Action::MoveDown))  moveDir.y += 1.f;

        if (auto* ds = DashSystem::getInstance()) {
            ds->triggerDash(mPlayerPtr, moveDir, &mParticleSystem);
        }
    }

    // --- DEBUG ---
    if (input.isActionJustPressed(Action::DebugXP) && mPlayerPtr) mPlayerPtr->addExperience(cfg::Debug::XP_GAIN, false);
    if (input.isActionJustPressed(Action::DebugStats)) {
        if (mPlayerPtr) mPlayerPtr->debugBoostStats();
        mExplosionMode = !mExplosionMode;
        if (mExplosionMode) std::cout << "Debug terrain on\n";
        else std::cout << "Debug terrain off\n";
    }
    // Action::DebugExplosion (O) remains as a secondary key or can be removed if U is preferred
    if (input.isActionJustPressed(Action::DebugExplosion)) {
        mExplosionMode = !mExplosionMode;
        if (mExplosionMode) std::cout << "Debug terrain on\n";
        else std::cout << "Debug terrain off\n";
    }
    
    // Mouse Handling
    handleMouseInput(game, view);

    // [SKILLS] Handle Skill 1 (Slot 0)
    if (game.getInput().isActionJustPressed(Action::Skill1)) {
        tryCastSkill(0);
    }
    
    // [SKILLS] Handle Skill 2 (Slot 1)
    if (game.getInput().isActionJustPressed(Action::Skill2)) {
         tryCastSkill(1);
    }

    // [SKILLS] Handle Skill 3 (Slot 2)
    if (game.getInput().isActionJustPressed(Action::Skill3)) {
         tryCastSkill(2);
    }

    // [SKILLS] Handle Skill 4 (Slot 3)
    if (game.getInput().isActionJustPressed(Action::Skill4)) {
         tryCastSkill(3);
    }

    // [SKILLS] Handle Skill 5 (Slot 4)
    if (game.getInput().isActionJustPressed(Action::Skill5)) {
         tryCastSkill(4);
    }

    // [SKILLS] Handle Skill 6 (Slot 5)
    if (game.getInput().isActionJustPressed(Action::Skill6)) {
         tryCastSkill(5);
    }

    // [SKILLS] Handle Skill 7 (Slot 6)
    if (game.getInput().isActionJustPressed(Action::Skill7)) {
         tryCastSkill(6);
    }

    // [SKILLS] Handle Skill 8 (Slot 7)
    if (game.getInput().isActionJustPressed(Action::Skill8)) {
         tryCastSkill(7);
    }

    // [SKILLS] Handle Skill 9 (Slot 8)
    if (game.getInput().isActionJustPressed(Action::Skill9)) {
         tryCastSkill(8);
    }

    // [SKILLS] Handle Skill 10 (Slot 9)
    if (game.getInput().isActionJustPressed(Action::Skill10)) {
         tryCastSkill(9);
    }

    // [SKILLS] Handle Skill 11 (Slot 10)
    if (game.getInput().isActionJustPressed(Action::Skill11)) {
         tryCastSkill(10);
    }

    // [SKILLS] Handle Skill 12 (Slot 11)
    if (game.getInput().isActionJustPressed(Action::Skill12)) {
         tryCastSkill(11);
    }

    // [CYCLE TARGET] Cycle targets using Tab key
    if (game.getInput().isActionJustPressed(Action::CycleTarget)) {
        mTabResetTimer = 2.5f; // Keep cycle list alive for 2.5 seconds
        
        // Remove candidates that are no longer valid, dead, or returning to spawn
        mTabTargetCandidates.erase(
            std::remove_if(mTabTargetCandidates.begin(), mTabTargetCandidates.end(),
                [this](Entity* ent) {
                    return !mEntityManager.isValid(ent) || !ent->isAlive() || ent->isReturningToSpawn();
                }),
            mTabTargetCandidates.end()
        );
        
        if (mTabTargetCandidates.empty()) {
            float maxRange = cfg::Player::TAB_TARGET_RANGE;
            float maxRangeSq = maxRange * maxRange;
            sf::Vector2f playerPos = mPlayerPtr->getPosition();
            int facingDir = mPlayerPtr->getFacingDir();
            
            // View bounds check to only select visible mobs
            sf::Vector2f viewSize = view.getSize();
            sf::Vector2f viewCenter = view.getCenter();
            float margin = cfg::Combat::AUTO_DESELECT_MARGIN;
            sf::FloatRect targetViewRect(
                sf::Vector2f(viewCenter.x - viewSize.x / 2.f - margin, viewCenter.y - viewSize.y / 2.f - margin),
                sf::Vector2f(viewSize.x + margin * 2.f, viewSize.y + margin * 2.f)
            );
            
            for (const auto& ent : mEntityManager.getActiveEntities()) {
                if (ent.get() == mPlayerPtr) continue;
                if (!ent->isAlive()) continue;
                if (ent->isReturningToSpawn()) continue;
                if (dynamic_cast<Mob*>(ent.get()) == nullptr) continue;
                
                // Must be inside the camera view rect to prevent auto-deselection
                if (!targetViewRect.contains(ent->getPosition())) continue;
                
                sf::Vector2f diff = ent->getPosition() - playerPos;
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq <= maxRangeSq) {
                    mTabTargetCandidates.push_back(ent.get());
                }
            }
            
            // Sort by facing direction first (those in front) and closest distance first
            std::sort(mTabTargetCandidates.begin(), mTabTargetCandidates.end(),
                [playerPos, facingDir](Entity* a, Entity* b) {
                    sf::Vector2f diffA = a->getPosition() - playerPos;
                    sf::Vector2f diffB = b->getPosition() - playerPos;
                    
                    bool inFrontA = (facingDir == 1 && diffA.x >= 0.f) || (facingDir == -1 && diffA.x <= 0.f);
                    bool inFrontB = (facingDir == 1 && diffB.x >= 0.f) || (facingDir == -1 && diffB.x <= 0.f);
                    
                    if (inFrontA != inFrontB) {
                        return inFrontA;
                    }
                    
                    float distSqA = diffA.x * diffA.x + diffA.y * diffA.y;
                    float distSqB = diffB.x * diffB.x + diffB.y * diffB.y;
                    return distSqA < distSqB;
                });
            
            // Sync: If player already had a manual target, start cycling from it
            int foundIdx = -1;
            if (mTargetedEntity) {
                for (size_t i = 0; i < mTabTargetCandidates.size(); ++i) {
                    if (mTabTargetCandidates[i] == mTargetedEntity) {
                        foundIdx = static_cast<int>(i);
                        break;
                    }
                }
            }
            mTabTargetIndex = foundIdx;
        }
        
        if (!mTabTargetCandidates.empty()) {
            mTabTargetIndex = (mTabTargetIndex + 1) % static_cast<int>(mTabTargetCandidates.size());
            Entity* newTarget = mTabTargetCandidates[mTabTargetIndex];
            
            mTargetedEntity = newTarget;
            mCombatSystem.setCombatTarget(newTarget);
            
            sf::Vector2f diff = newTarget->getPosition() - mPlayerPtr->getPosition();
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            std::cout << "[INPUT] TAB: Targeted Mob at dist " << dist 
                      << " (Index " << mTabTargetIndex + 1 << "/" << mTabTargetCandidates.size() << ")" << std::endl;
        }
    }
}

// [REFACTOR] Updated signature to accept view
void PlayerController::handleMouseInput(Game& game, const sf::View& view) {
    const auto& input = game.getInput();
    sf::Vector2i mouseI = input.getMousePosition();
    
    // [REFACTOR] Use passed view
    // sf::Vector2f uiMousePos = game.getWindow().mapPixelToCoords(mouseI, game.getWindow().getDefaultView());
    // Use RenderWindow logic directly from Game if possible, or just use the window reference
    sf::RenderWindow& window = game.getWindow();
    
    sf::Vector2f uiMousePos = window.mapPixelToCoords(mouseI, mUIManager.getUIView());
    sf::Vector2f worldPos = window.mapPixelToCoords(mouseI, view);
    mLastMouseWorldPos = worldPos;

    // Clic Izquierdo (Seleccionar / UI)
    if (input.isActionJustPressed(Action::Interact)) {
        
        if (mExplosionMode) {
            mHasExplosionClick = true;
            mExplosionPos = worldPos;
            return;
        }

        // [DEBUG LOG]
        std::cout << "[INPUT] Left Click @ Screen: " << mouseI.x << "," << mouseI.y 
                  << " | World: " << worldPos.x << "," << worldPos.y << std::endl;

        if (mUIManager.handleInteract(uiMousePos, mPlayerPtr)) {
            // UI Handled it
        } else if (mTargetingGroundSkillId != -1) {
            // Cast ground skill at worldPos
            const Skill* skill = mSkillManager.getSkill(mTargetingGroundSkillId);
            if (skill && mPlayerPtr && mPlayerPtr->isSkillReady(skill->id) && mPlayerPtr->getCurrentMp() >= skill->manaCost) {
                mPlayerPtr->restoreMana(-skill->manaCost);
                float cdrPct = std::clamp(mPlayerPtr->getCooldownReductionPercent(), 0.f, 100.f);
                mPlayerPtr->setSkillCooldown(skill->id, skill->cooldown * (1.f - cdrPct / 100.f), skill->cooldown);

                Skill* mutableSkill = const_cast<Skill*>(skill);
                if (auto* totemSkill = dynamic_cast<TotemHeal_1*>(mutableSkill)) {
                    totemSkill->setCastTargetPosition(worldPos);
                }
                mutableSkill->onCastStart(mPlayerPtr, mPlayerPtr, &mParticleSystem);
                mutableSkill->onExecute(mPlayerPtr, mPlayerPtr, &mParticleSystem);

                mPlayerPtr->recalculateStats();
                mPlayerPtr->notifyStatsChanged();
            }
            mTargetingGroundSkillId = -1;
            return;
        } else {
            // Seleccionar en el mundo
            bool hit = false;
            
            // [OPTIMIZATION] Spatial Grid Query
            auto candidates = mEntityManager.querySpatialGrid(worldPos, 32.f); 
            
            // Helper Lambda for hit detection
            auto isPixelPerfectHit = [&](Entity* e) -> bool {
                return e->getGlobalBounds().contains(worldPos);
            };

            // Iterate reverse to respect Z-order (draw order) roughly? 
            for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
                Entity* e = *it;
                if (!e->isAlive()) continue;
                if (e == mPlayerPtr) continue; // [FIX] Prevent self-selection
                
                if (isPixelPerfectHit(e)) {
                    setTargetedEntity(e); hit = true; 
                    if (Mob* mob = dynamic_cast<Mob*>(e)) {
                        std::cout << "[INPUT] Selected Mob: " << mob->getName() << " (Level " << mob->getLevel() << ")" << std::endl;
                        std::cout << "  Equipped Armor:" << std::endl;
                        for (int slotIdx = 0; slotIdx < 12; ++slotIdx) {
                            auto slot = static_cast<EquipmentSlot>(slotIdx);
                            if (slot == EquipmentSlot::Head || slot == EquipmentSlot::Chest || 
                                slot == EquipmentSlot::Hands || slot == EquipmentSlot::Feet) {
                                auto item = mob->getEquippedItem(slot);
                                if (item) {
                                    std::cout << "    Slot " << slotIdx << ": " << item->name << " [ID: " << item->id << "]" << std::endl;
                                } else {
                                    std::cout << "    Slot " << slotIdx << ": Empty" << std::endl;
                                }
                            }
                        }
                    } else {
                        std::cout << "[INPUT] Selected: Entity" << std::endl;
                    }
                    break;
                }
            }

            if (!hit) {
                // Do NOT deselect on empty ground left click (only right click or selecting another target changes selection)
                // mTargetedEntity = nullptr;
                // std::cout << "[INPUT] Selection Cleared." << std::endl;
            }
        }
    }

    // Clic Derecho (Atacar / Equipar / Desequipar / Inspeccionar / Cancelar Colocación)
    if (input.isActionJustPressed(Action::Attack)) {
        
        if (mTargetingGroundSkillId != -1) {
            mTargetingGroundSkillId = -1;
            std::cout << "[SKILL] Cancelado modo de colocacion en el suelo.\n";
            return;
        }

        // [DEBUG LOG]
        std::cout << "[INPUT] Right Click @ Screen: " << mouseI.x << "," << mouseI.y 
                  << " | World: " << worldPos.x << "," << worldPos.y << std::endl;

        mPendingSkillId = -1;
        mPendingSkillTarget = nullptr;
        
        bool uiHandled = mUIManager.handleRightClick(uiMousePos, mPlayerPtr, mTargetedEntity, mEntityManager, game.getResources(), input);

        if (!uiHandled) {
            if (mPlayerPtr && mPlayerPtr->isStunned()) {
                return;
            }

            Entity* closest = nullptr;
            float minDistSq = std::numeric_limits<float>::max();

            const auto& activeEntities = mEntityManager.getActiveEntities();
            for (const auto& ent : activeEntities) {
                if (ent.get() == mPlayerPtr) continue;
                if (!ent->isAlive()) continue;

                if (ent->isReturningToSpawn()) continue; 
                
                if (ent->getGlobalBounds().contains(worldPos)) {
                    sf::Vector2f diff = ent->getPosition() - mPlayerPtr->getPosition();
                    float distSq = diff.x * diff.x + diff.y * diff.y;
                    if (distSq < minDistSq) { minDistSq = distSq; closest = ent.get(); }
                }
            }

            if (closest) {
                std::cout << "[INPUT] Right Click Target Found: " << (dynamic_cast<Mob*>(closest) ? "Mob" : "Entity") << std::endl;
                
                // [SAFETY] Double check pointers
                if (mEntityManager.isValid(mPlayerPtr) && mEntityManager.isValid(closest)) {
                    mCombatSystem.setCombatTarget(closest);
                    mCombatSystem.requestAutoAttack();
                    mPlayerPtr->setFollowTarget(closest);
                    setTargetedEntity(closest);
                }
            } else {
                std::cout << "[INPUT] Right Click on Empty Ground. Deselecting." << std::endl;
                // [IMPROVEMENT] Deselect on empty ground click
                if (mPlayerPtr) {
                    mPlayerPtr->setFollowTarget(nullptr);
                }
                setTargetedEntity(nullptr); 
                mCombatSystem.setCombatTarget(nullptr);
            }
        }
    }

    // --- HOLD TO GUARD (Tecla Q sostenida para cubrirse con el escudo) ---
    if (mPlayerPtr) {
        bool canGuard = mPlayerPtr->hasShieldEquipped() && !mPlayerPtr->isStunned();
        bool guardKeyHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q) && !mUIManager.isChatFocused();

        if (canGuard && guardKeyHeld) {
            if (auto* ss = ShieldSystem::getInstance()) {
                if (!ss->isGuardActive(mPlayerPtr)) {
                    ss->setGuardActive(mPlayerPtr, true);
                }
            }
        } else {
            if (auto* ss = ShieldSystem::getInstance()) {
                if (ss->isGuardActive(mPlayerPtr)) {
                    ss->setGuardActive(mPlayerPtr, false);
                }
            }
        }
    }
}

void PlayerController::drawGroundTargeting(sf::RenderTarget& target, SkillManager& skillManager, ResourceManager& res) {
    if (mTargetingGroundSkillId == -1) return;

    const Skill* skill = skillManager.getSkill(mTargetingGroundSkillId);
    if (!skill) return;

    float radiusX = (skill->range > 0) ? static_cast<float>(skill->range) : 140.f;
    float radiusY = radiusX * 0.5f; // 2.5D perspectiva (2:1)

    // 1. Elipse trazada con píxeles de 1x1 (sin área de relleno verde)
    const float pixelSize = 1.0f;
    int totalSteps = static_cast<int>(std::ceil(6.2831853f * std::max(radiusX, radiusY) * 2.0f));
    if (totalSteps < 64) totalSteps = 64;

    std::vector<sf::Vertex> pixelVerts;
    pixelVerts.reserve(totalSteps * 6);

    sf::Color outlineColor(35, 130, 60, 220); // Verde más oscuro

    float prevGridX = -999999.f;
    float prevGridY = -999999.f;

    for (int i = 0; i < totalSteps; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(totalSteps)) * 6.2831853f;
        float rawX = mLastMouseWorldPos.x + std::cos(angle) * radiusX;
        float rawY = mLastMouseWorldPos.y + std::sin(angle) * radiusY;

        float gridX = std::floor(rawX / pixelSize) * pixelSize;
        float gridY = std::floor(rawY / pixelSize) * pixelSize;

        if (gridX == prevGridX && gridY == prevGridY) {
            continue;
        }
        prevGridX = gridX;
        prevGridY = gridY;

        // Quad de 1x1 píxel (2 triángulos)
        sf::Vector2f tl(gridX, gridY);
        sf::Vector2f tr(gridX + pixelSize, gridY);
        sf::Vector2f bl(gridX, gridY + pixelSize);
        sf::Vector2f br(gridX + pixelSize, gridY + pixelSize);

        pixelVerts.emplace_back(sf::Vertex{tl, outlineColor});
        pixelVerts.emplace_back(sf::Vertex{tr, outlineColor});
        pixelVerts.emplace_back(sf::Vertex{bl, outlineColor});

        pixelVerts.emplace_back(sf::Vertex{tr, outlineColor});
        pixelVerts.emplace_back(sf::Vertex{br, outlineColor});
        pixelVerts.emplace_back(sf::Vertex{bl, outlineColor});
    }

    if (!pixelVerts.empty()) {
        target.draw(pixelVerts.data(), pixelVerts.size(), sf::PrimitiveType::Triangles);
    }

    // 2. Sprite preview del tótem en gris oscuro que sigue al cursor
    try {
        const sf::Texture& totemTex = res.getTexture("src/core/skills/active/TotemHeal_1/totem.png");
        sf::Sprite previewSprite(totemTex);
        previewSprite.setOrigin({9.5f, 46.0f});
        previewSprite.setPosition(mLastMouseWorldPos);
        previewSprite.setColor(sf::Color(65, 65, 65, 215)); // Color gris oscuro semi-transparente
        target.draw(previewSprite);
    } catch (...) {}
}
