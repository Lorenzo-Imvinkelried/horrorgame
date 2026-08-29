#include "CombatSystem.h"
#include "Config.h"
#include "entities/mob/Mob.h"
#include "utils/Random.h"
#include "utils/CombatCalculator.h"
#include "core/engine/animation/Animation.h"
#include "core/systems/SoundSystem.h"
#include "GhostSlash.h"
#include <iostream>

void CombatSystem::initiateAttack(Entity* target) {
    if (!mPlayer || !target) return;
    if (!isEntityAllocated(target)) return;
    if (!target->isAlive()) return;

    int numHits = 1;
    float tripleChance = mPlayer->getTripleStrikeChance();
    float doubleChance = mPlayer->getDoubleStrikeChance();

    if (Random::Roll(tripleChance)) {
        numHits = 3;
    } else if (Random::Roll(doubleChance)) {
        numHits = 2;
    }

    if (numHits > 1) {
        std::cout << "[COMBAT] MULTI-STRIKE ACTIVADO: " << numHits << " hits!\n";
    }

    // 1. Normal Base Attack (always runs at normal attack speed)
    mMultiStrike.active = true;
    mMultiStrike.totalStrikes = numHits;
    mMultiStrike.currentStrike = 0;
    mMultiStrike.speedMultiplier = 1.0f;
    mMultiStrike.strikeDuration = (1.f / mPlayer->getAtkSpeed());
    mMultiStrike.target = target;
    mMultiStrike.isMiss = !CombatCalculator::tryToHit(mPlayer, target);

    mPlayer->startAttackAnimation(target, 1.0f);

    const AnimationClip* clip = mPlayer->getSkin().getAnimation() ? mPlayer->getSkin().getAnimation()->getCurrentClip() : nullptr;
    std::string clipName = clip ? clip->name : "";
    float delayFactor = Animation::getHitDelayFactor(clipName);
    mMultiStrike.strikeImpactDelay = mMultiStrike.strikeDuration * delayFactor;
    mMultiStrike.strikeClock.restart();
    mMultiStrike.strikeDamageApplied = false;

    mCombatTimer.restart();
    mLeashTimer.restart();
}

void CombatSystem::spawnGhostSlashes(Entity* target, int totalHits) {
    if (!mPlayer || !target || totalHits <= 1) return;

    GhostSlashVisualData vData;
    const Animation* playerAnim = mPlayer->getSkin().getAnimation();
    const WeaponSprite* ws = nullptr;
    if (playerAnim) {
        if (playerAnim->getWeapon()) {
            ws = playerAnim->getWeapon();
            vData.offset = playerAnim->getWeaponOffset();
        } else if (playerAnim->getWeaponSecondary()) {
            ws = playerAnim->getWeaponSecondary();
            vData.offset = playerAnim->getSecondaryWeaponOffset();
        }
    }
    if (ws && playerAnim) {
        vData.baseTexture = ws->getBaseTexture();
        vData.layoutTexture = ws->getLayoutTexture();
        vData.baseRect = ws->getBaseRect();
        vData.overlayRect = ws->getOverlayRect();
        vData.rarityColor = ws->getRarityColor();
        vData.fortificationLevel = ws->getFortificationLevel();
        vData.origin = ws->getOrigin();
        vData.scale = playerAnim->getBaseScale();
        vData.isTwoHanded = playerAnim->isWeaponTwoHanded();
    }

    const AnimationClip* clip = playerAnim ? playerAnim->getCurrentClip() : nullptr;
    float ghostSpeedMult = 3.5f;

    // Hit 1: Spawns immediately on main hit impact (+0.03s)
    float ghost1Delay = 0.03f;
    bool ghost1Miss = !CombatCalculator::tryToHit(mPlayer, target);
    GhostSlashSystem::spawn(mPlayer, target, 1, totalHits, clip, ghost1Delay, ghostSpeedMult, vData, ghost1Miss);

    // Hit 2: Spawns staggered after Ghost 1 (+0.09s)
    if (totalHits >= 3) {
        float ghost2Delay = ghost1Delay + 0.09f;
        bool ghost2Miss = !CombatCalculator::tryToHit(mPlayer, target);
        GhostSlashSystem::spawn(mPlayer, target, 2, totalHits, clip, ghost2Delay, ghostSpeedMult, vData, ghost2Miss);
    }
}

void CombatSystem::applyDamageImpact(Entity* target) {
    if (!mPlayer || !target) return;
    
    if (!mInCombat && mPlayer && mPlayer->isInCombat()) {
        engageCombat();
    }
    
    performSingleStrike(mPlayer, target, 0, 1);
}

void CombatSystem::performSingleStrike(Entity* attacker, Entity* defender, int hitIndex, int totalHits) {
    if (!attacker || !defender) return;
    if (!isEntityAllocated(defender)) return;
    if (!defender->isAlive()) return;
    if (defender->isReturningToSpawn()) return;

    bool isBonusHit = (hitIndex > 0);
    float scale = isBonusHit ? cfg::UI::DAMAGE_SCALE_BONUS_HIT : 1.0f;
    bool allowTrue = !isBonusHit;
    float skillBonus = 0.f;

    CombatCalculator::DamageResult result = CombatCalculator::calculateDamage(attacker, defender, allowTrue, skillBonus);

    Player* playerAttacker = dynamic_cast<Player*>(attacker);

    // Tap System bonus for Player
    if (playerAttacker && hitIndex == 0) {
        bool thresholdHit = playerAttacker->getTapSystem().onBasicAttackHit();
        if (thresholdHit) {
            float thresholdMult = playerAttacker->getTapSystem().getThresholdDamageMultiplier();
            result.totalDamage = static_cast<int>(result.totalDamage * thresholdMult);
            result.physicalDamage = static_cast<int>(result.physicalDamage * thresholdMult);
        }
        float chargeMult = playerAttacker->getTapSystem().getChargeDamageMultiplier();
        if (chargeMult > 1.0f) {
            result.totalDamage = static_cast<int>(result.totalDamage * chargeMult);
            result.physicalDamage = static_cast<int>(result.physicalDamage * chargeMult);
        }
    }
    
    bool isPureTrue = (result.trueDamage > 0 && result.physicalDamage == 0);
    
    defender->setLastHitDirect(true);
    defender->takeDamage(result.totalDamage, attacker, result.isCrit, isPureTrue);
    
    if (result.isBlocked) {
         if (mSoundSystem) {
              mSoundSystem->playSound("assets/sounds/block.wav", 100.f);
         }
    } else if (result.totalDamage > 0) {
         defender->triggerHitEffect(attacker->getPosition());
         if (mSoundSystem) {
              if (result.isCrit && attacker && attacker->hasTwoHandedWeaponEquipped()) {
                  mSoundSystem->playSound("assets/sounds/crit_2h.wav", 100.f);
              } else if (attacker && attacker->hasTwoHandedWeaponEquipped()) {
                  mSoundSystem->playSound("assets/sounds/efecto_sonido_ia/2h_basic_attack.wav", 100.f);
              } else if (result.isCrit && attacker && !attacker->hasWeaponEquipped()) {
                  mSoundSystem->playSound("assets/sounds/player/crit_hand.wav", 100.f);
              } else {
                  mSoundSystem->playSound("assets/sounds/barehand_hit.wav", 100.f);
              }
         }
    }
    
    // Floating Damage Feedback
    float currentOffsetY = cfg::UI::DAMAGE_OFFSET_BASE + hitIndex * cfg::UI::DAMAGE_OFFSET_STACK;
    if (result.physicalDamage > 0 && result.trueDamage > 0) {
        mFeedback.onHit(defender, result.physicalDamage, result.isCrit, false, result.isBlocked, true, currentOffsetY, scale, attacker);
        mFeedback.onHit(defender, result.trueDamage, result.isCrit, true, false, false, currentOffsetY + 10.f, scale, attacker);
    } 
    else if (result.trueDamage > 0) {
         mFeedback.onHit(defender, result.trueDamage, result.isCrit, true, result.isBlocked, true, currentOffsetY, scale, attacker);
    } 
    else {
         mFeedback.onHit(defender, result.physicalDamage, result.isCrit, false, result.isBlocked, true, currentOffsetY, scale, attacker);
    }

    // Apply AOE
    applyAoE(attacker, defender, result.totalDamage);
    
    // Lifesteal
    float lifestealPct = attacker->getLifestealPercent();
    if (lifestealPct > 0.0f && result.totalDamage > 0 && attacker->isAlive()) {
        int healAmount = static_cast<int>(result.totalDamage * (lifestealPct / 100.0f));
        if (healAmount > 0) {
            attacker->heal(healAmount);
            mFeedback.onHeal(attacker, healAmount, attacker); 
        }
    }

    // Mana Steal
    float manaStealPct = attacker->getManaStealPercent();
    if (manaStealPct > 0.0f && result.totalDamage > 0 && attacker->isAlive()) {
         int mpAmount = static_cast<int>(result.totalDamage * (manaStealPct / 100.0f));
         if (mpAmount > 0) {
             if (auto* p = dynamic_cast<Player*>(attacker)) {
                 p->restoreMana(mpAmount);
             }
         }
    }

    // Thorns
    float thornsPct = defender->getThornsPercent();
    if (thornsPct > 0.0f && attacker && !isPureTrue && defender->isAlive()) { 
         int thornsDmg = static_cast<int>(result.totalDamage * (thornsPct / 100.0f));
         
         if (thornsDmg > 0) {
              auto thornsRes = CombatCalculator::calculateMitigatedDamage(attacker, defender, (float)thornsDmg, true);
              attacker->setLastHitDirect(false);
              attacker->takeDamage(thornsRes.totalDamage, defender, false, false);
              mFeedback.onHit(attacker, thornsRes.totalDamage, false, false, thornsRes.isBlocked, false);
         }
    }
    
    // Bleed Application
    float bleedDurFlat = attacker->getBleedDurationFlat();
    float bleedDurPercent = attacker->getBleedDurationPercent();
    
    if (bleedDurFlat > 0.f || bleedDurPercent > 0.f) {
         float bleedFromPercent = 0.f;
         float attackerBleedPct = attacker->getBleedPercent();
         if (attackerBleedPct > 0.f) {
             float playerAttack = (float)attacker->getAttack(); 
             bleedFromPercent = playerAttack * (attackerBleedPct / 100.f);
         }

         defender->applyBleed(bleedDurFlat, bleedDurPercent, attacker->getBleedFlat(), bleedFromPercent, attacker);
         registerDebuff(defender);
    }

    // Stun Application
    float stunChance = attacker->getStunChance();
    if (Random::Roll(stunChance)) {
        float stunDur = attacker->getStunDuration();
        if (stunDur > 0.f) {
            defender->applyStun(stunDur);
            registerDebuff(defender);
            mFeedback.onStun(defender, stunDur);
        }
    }
    
    // Slow Move Application
    float slowMovePct = attacker->getSlowMovePercent();
    if (slowMovePct > 0.f) {
        float slowMoveDur = attacker->getSlowMoveDuration();
        if (slowMoveDur > 0.f) {
            defender->applySlowMove(slowMoveDur, slowMovePct);
            registerDebuff(defender);
        }
    }

    // Slow Attack Application
    float slowAtkPct = attacker->getSlowAttackPercent();
    if (slowAtkPct > 0.f) {
        float slowAtkDur = attacker->getSlowAttackDuration();
        if (slowAtkDur > 0.f) {
            defender->applySlowAttack(slowAtkDur, slowAtkPct);
            registerDebuff(defender);
        }
    }

    if (playerAttacker) {
        playerAttacker->addToAggro(defender);
    }
}

void CombatSystem::performAttack(Entity* attacker, Entity* defender) {
    if (!attacker || !defender) return;
    if (!isEntityAllocated(defender)) return;
    if (!defender->isAlive()) return;
    if (defender->isReturningToSpawn()) return;

    int numHits = 1;
    float tripleChance = attacker->getTripleStrikeChance();
    float doubleChance = attacker->getDoubleStrikeChance();

    if (Random::Roll(tripleChance)) {
        numHits = 3;
    } else if (Random::Roll(doubleChance)) {
        numHits = 2;
    }

    for (int i = 0; i < numHits; ++i) {
        if (!defender || !defender->isAlive()) break;
        performSingleStrike(attacker, defender, i, numHits);
    }
}

void CombatSystem::createMissEffect(Entity* target, Entity* attacker) {
    if (target) {
        mFeedback.onMiss(target, attacker ? attacker : mPlayer);
    }
}
