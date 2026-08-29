#include "CombatSystem.h"
#include "Config.h"
#include "entities/mob/Mob.h"
#include "core/managers/EntityManager.h"
#include "core/skills/SkillManager.h"
#include "core/engine/animation/Animation.h"
#include "core/systems/SoundSystem.h"
#include "GhostSlash.h"
#include "utils/CombatCalculator.h"
#include <iostream>
#include <cmath>

CombatSystem* CombatSystem::sInstance = nullptr;

CombatSystem::CombatSystem(Hud& hud, FXSystem& fxSystem, ParticleSystem* ps) 
    : mHud(hud), 
      mFeedback(fxSystem, ps),
      mParticleSystem(ps) 
{
    sInstance = this;
    mHud.setCombatSystem(this);

    Mob::s_onExperienceCallback = [this](Entity* player, int xp) {
        mFeedback.onExperience(player, xp);
    };
}

void CombatSystem::setParticleSystem(ParticleSystem* ps) {
    mFeedback.setParticleSystem(ps);
    mParticleSystem = ps; 
}

void CombatSystem::setPlayer(Player* player) {
    mPlayer = player;
}

void CombatSystem::setEntityManager(EntityManager* em) {
    mEntityManager = em;
}

void CombatSystem::setCombatTarget(Entity* target) {
    if (target == mPlayer || target == nullptr) {
        mCombatTarget = nullptr;
        cancelPendingAttack();
        return;
    }
    
    if (target && target->isReturningToSpawn()) {
        return; 
    }

    mCombatTarget = target;
}

void CombatSystem::requestAutoAttack() {
    mAutoAttackRequested = true;
    mAwaitingFirstHit = false;
}

void CombatSystem::notifyPlayerMoved() {
    mPlayerMovedFlag = true;
    if (!isCastingSkill()) {
        cancelPendingAttack();
    }
}

void CombatSystem::engageCombat() {
    if (mInCombat) return;

    mInCombat = true;
    mLeashTimer.restart();

    mHud.setCombatStatus("Entrando en combate", sf::Color::Red);
}

void CombatSystem::disengageCombat() {
    if (!mInCombat) return;

    Entity* oldTarget = mCombatTarget;

    mInCombat = false;
    mAutoAttackRequested = false;
    mCombatTarget = nullptr;
    mActualTarget = nullptr;

    mHud.setCombatStatus("Saliendo de batalla", sf::Color::White);

    if (mPlayer) {
        mPlayer->setFollowTarget(nullptr);
    }

    if (oldTarget && oldTarget->isAlive()) {
        oldTarget->returnToSpawn();
    }
}

bool CombatSystem::isEntityAllocated(Entity* candidate) const {
    if (!candidate || !mEntityManager) return false;
    return mEntityManager->isValid(candidate); 
}

void CombatSystem::resetAttackTimer() {
    mAttackReset = true;
}

void CombatSystem::cancelPendingAttack() {
    mDamagePending = false;
    mPendingTarget = nullptr;
    mPendingSkill = nullptr;
    mPendingMiss = false;
    mMultiStrike.active = false;
    mMultiStrike.target = nullptr;
    if (mPlayer) {
        mPlayer->setCasting(false);
        mPlayer->cancelAttackAnimation();
    }
}

void CombatSystem::update(sf::Time dt) {
    if (!mPlayer) return;

    // 1. Process AoE Waves
    updateAoEWaves(dt);

    // 2. Process Pending Damage Delay (Skills)
    if (mDamagePending) {
         if (mPendingTarget && !isEntityAllocated(mPendingTarget)) { 
             mPendingTarget = nullptr; 
             mDamagePending = false; 
             mPendingSkill = nullptr;
         } else if (mPendingTarget) {
            if (mDamageTimer.getElapsedTime().asSeconds() >= mDamageDelay) {
                  float baseRange = mPlayer->getAttackRange();
                  if (mPendingSkill) {
                       if (mPendingSkill->id == 4) {
                           baseRange = 30.f;
                       } else if (mPendingSkill->id != 1 && mPendingSkill->id != 31 && mPendingSkill->range > 0) {
                           baseRange = static_cast<float>(mPendingSkill->range);
                       }
                  }
                  float rangeTolerance = baseRange + 25.f;

                sf::Vector2f pPos = mPlayer->getPosition();
                sf::Vector2f tPos = mPendingTarget->getPosition();
                float distSq = (pPos.x - tPos.x)*(pPos.x - tPos.x) + (pPos.y - tPos.y)*(pPos.y - tPos.y);

                if (mPendingTarget == mPlayer || distSq <= rangeTolerance * rangeTolerance) {
                    if (mPendingSkill) {
                        performSkillAttack(mPendingSkill, mPendingTarget);
                        mPendingSkill = nullptr;
                    }
                    else {
                        if (mPlayer && mPendingTarget) {
                            mPlayer->addToAggro(mPendingTarget);
                            mPendingTarget->onAggroedBy(mPlayer);
                        }
                        if (!mInCombat && mPlayer && mPlayer->isInCombat()) {
                            engageCombat();
                        }

                        if (mPendingMiss) {
                            mFeedback.onMiss(mPendingTarget, mPlayer);
                        } else {
                            applyDamageImpact(mPendingTarget);
                        }
                    }
                } else {
                    std::cout << "[COMBAT] Objetivo fuera de rango de tolerancia (" << std::sqrt(distSq) << " > " << rangeTolerance << "). Golpe cancelado.\n";
                }
                
                mDamagePending = false;
                mPendingTarget = nullptr;
                mPendingMiss = false;
            }
         }
    }

    // 2.5. Process Player Base Auto-Attack
    if (mMultiStrike.active) {
        if (!mMultiStrike.target || !isEntityAllocated(mMultiStrike.target) || !mMultiStrike.target->isAlive()) {
            mMultiStrike.active = false;
            mMultiStrike.target = nullptr;
        } else {
            float elapsed = mMultiStrike.strikeClock.getElapsedTime().asSeconds();

            // Impact phase for base strike (Hit #0)
            if (!mMultiStrike.strikeDamageApplied && elapsed >= mMultiStrike.strikeImpactDelay) {
                float rangeTolerance = mPlayer->getAttackRange() + 25.f;

                sf::Vector2f pPos = mPlayer->getPosition();
                sf::Vector2f tPos = mMultiStrike.target->getPosition();
                float distSq = (pPos.x - tPos.x)*(pPos.x - tPos.x) + (pPos.y - tPos.y)*(pPos.y - tPos.y);

                if (distSq <= rangeTolerance * rangeTolerance) {
                    if (mPlayer && mMultiStrike.target) {
                        mPlayer->addToAggro(mMultiStrike.target);
                        mMultiStrike.target->onAggroedBy(mPlayer);
                    }
                    if (!mInCombat && mPlayer && mPlayer->isInCombat()) {
                        engageCombat();
                    }

                    if (mMultiStrike.isMiss) {
                        mFeedback.onMiss(mMultiStrike.target, mPlayer);
                    } else {
                        performSingleStrike(mPlayer, mMultiStrike.target, 0, mMultiStrike.totalStrikes);
                    }

                    // --- Spawn Ghost Slashes on multi-strike ONLY when main strike connects/executes ---
                    if (mMultiStrike.totalStrikes > 1) {
                        spawnGhostSlashes(mMultiStrike.target, mMultiStrike.totalStrikes);
                    }
                } else {
                    std::cout << "[COMBAT] Objetivo fuera de rango. Golpe cancelado.\n";
                }
                mMultiStrike.strikeDamageApplied = true;
            }

            // Completion phase of player's attack animation
            if (elapsed >= mMultiStrike.strikeDuration) {
                mMultiStrike.active = false;
                mMultiStrike.target = nullptr;
            }
        }
    }

    // 2.6. Process Ghost Slashes (Hit #1 & Hit #2 on-hit procs)
    GhostSlashSystem::updateAll(dt.asSeconds(), this);

    // 3. Process Debuffs and Bleed Ticks O(M)
    updateDebuffsAndBleeds(dt);

    // 4. Charge Visuals for Pending Skills
    if (mPendingSkill && mPlayer) {
         const_cast<Skill*>(mPendingSkill)->updateChargeVisuals(mPlayer, mParticleSystem);
    }

    // 5. Input Movement Interruption
    if (mPlayerMovedFlag) {
        mAutoAttackRequested = false;
        mPlayerMovedFlag = false;
        if (!isCastingSkill()) {
            mCombatTarget = nullptr;
            mPendingSkill = nullptr;
            mPendingTarget = nullptr;
            mDamagePending = false;
        }
        mMultiStrike.active = false;
        mMultiStrike.target = nullptr;
        mLeashTimer.restart();
        if (!isCastingSkill()) {
            return;
        }
    }

    if (!mCombatTarget) {
        if (mInCombat) {
            if (mPlayer && !mPlayer->isInCombat()) {
                 disengageCombat();
            }
        }
        return;
    }

    if (!isEntityAllocated(mCombatTarget)) {
        mCombatTarget = nullptr;
        mActualTarget = nullptr;
        return;
    }

    if (mCombatTarget->isReturningToSpawn()) {
        mCombatTarget = nullptr; 
        return;
    }

    if (!mCombatTarget->isAlive()) {
        mCombatTarget = nullptr;
        return;
    }

    // 6. Range Check & Leash
    const float ATTACK_RANGE_PX = mPlayer->getAttackRange();
    sf::Vector2f pPos = mPlayer->getPosition();
    sf::Vector2f tPos = mCombatTarget->getPosition();
    float distSq = (pPos.x - tPos.x)*(pPos.x - tPos.x) + (pPos.y - tPos.y)*(pPos.y - tPos.y);

    if (distSq > (ATTACK_RANGE_PX * ATTACK_RANGE_PX)) {
        if (mInCombat) {
            if (mLeashTimer.getElapsedTime().asSeconds() > mLeashTime) {
                 mCombatTarget = nullptr;
                 mAutoAttackRequested = false;
            }
        }
        return; 
    }

    // 7. In Range Attacks
    if (!mInCombat && !mDamagePending && !mMultiStrike.active) {
        if (!mAutoAttackRequested) return;
        if (mPlayer && mPlayer->isStunned()) return;

        mActualTarget = mCombatTarget;

        if (!mAwaitingFirstHit) {
            mAwaitingFirstHit = true;
        }

        float timeBetweenAttacks = 1.f / mPlayer->getAtkSpeed();
        
        if (mCombatTimer.getElapsedTime().asSeconds() >= timeBetweenAttacks) {
            initiateAttack(mActualTarget);
            mAwaitingFirstHit = false;
        } 
    }
    
    if (!mInCombat && mPlayer && mPlayer->isInCombat()) {
        engageCombat();
    }

    if (mInCombat) {
        mLeashTimer.restart();
    }
    mActualTarget = mCombatTarget;

    // 8. Sustained Attacks
    if (mInCombat && mAutoAttackRequested && !mDamagePending && !mMultiStrike.active) {
        if (mPlayer && mPlayer->isStunned()) return;

        float timeBetweenAttacks = 1.f / mPlayer->getAtkSpeed();
        
        if (mAttackReset || mCombatTimer.getElapsedTime().asSeconds() >= timeBetweenAttacks) {
            initiateAttack(mActualTarget);
            mAttackReset = false;
        }
    }
    
    if (mInCombat && mPlayer) {
         if (!mPlayer->isInCombat()) {
             disengageCombat();
         }
    }
}
