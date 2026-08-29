#include "Mob.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/DashSystem.h"
#include "core/systems/SoundSystem.h"
#include "entities/player/Player.h"
#include <cmath>
#include <iostream>

int Mob::takeDamage(int damageAmount, Entity *attacker, bool isCrit,
                    bool isTrueDamage) {
  if (!isAlive())
    return 0;

  // [DASH I-FRAMES] Completely dodge damage during active dash
  if (auto *ds = DashSystem::getInstance()) {
    if (ds->isInvulnerable(this) && !isTrueDamage) {
      if (auto *cs = CombatSystem::getInstance()) {
        cs->getFeedback().onMiss(this, attacker);
      }
      return 0;
    }
  }

  if (isReturningToSpawn())
    return 0;

  if (attacker)
    setLastAttacker(attacker);

  bool wasAlive = (mCurrentHp > 0);

  mCurrentHp -= damageAmount;
  if (mCurrentHp < 0)
    mCurrentHp = 0;

  // --- ¡LÓGICA DE AGRO! ---
  if (mCurrentHp > 0) {
    if (mBlueprintName == "goblin") {
      if (auto *ss = SoundSystem::getInstance()) {
        int rNum = (rand() % 2) + 1;
        ss->playSound("assets/sounds/goblin/goblin_take_damage_" +
                          std::to_string(rNum) + ".wav",
                      25.f);
      }
    }

    if (mStance != MobStance::Passive) {
      mAggroTarget = attacker;
      mForceAiUpdate = true;

      if (mAiDecision != AiDecision::Chasing) {
        mAiDecision = AiDecision::Chasing;
      }
    }
  } else {
    if (mBlueprintName == "goblin") {
      if (auto *ss = SoundSystem::getInstance()) {
        int rNum = (rand() % 3) + 1;
        ss->playSound("assets/sounds/goblin/goblin_dead_" +
                          std::to_string(rNum) + ".wav",
                      35.f);
      }
    }

    mCurrentHp = 0;
    mCurrentMp = 0;

    if (wasAlive) {
      Player *playerKiller = nullptr;
      if (attacker) {
        playerKiller = dynamic_cast<Player *>(attacker);
      }
      if (!playerKiller && getLastAttacker()) {
        playerKiller = dynamic_cast<Player *>(getLastAttacker());
      }
      if (!playerKiller && mAggroTarget) {
        playerKiller = dynamic_cast<Player *>(mAggroTarget);
      }

      if (playerKiller) {
        int xp = getXp();
        if (xp > 0) {
          int actualXp = playerKiller->addExperience(xp);
          if (s_onExperienceCallback) {
            s_onExperienceCallback(playerKiller, actualXp);
          }
        }
      }
    }

    die();
    mAggroTarget = nullptr;
  }

  notifyStatsChanged();

  return damageAmount;
}

void Mob::die() {
  if (mAggroTarget) {
    auto *p = dynamic_cast<Player *>(mAggroTarget);
    if (p) {
      p->removeFromAggro(this);
    }
  }

  mCurrentState = State::Dead;
  mMoving = false;
  mVelocity = {0.f, 0.f};
}

void Mob::startAttackAnimation(Entity *target, float speedMultiplier) {
  if (speedMultiplier <= 0.f)
    speedMultiplier = 1.0f;
  mCurrentState = State::Attacking;
  const AnimationClip *attackClipToUse =
      (mHasTwoHandedWeapon && mAttackTwoHandedClip) ? mAttackTwoHandedClip
                                                    : mAttackClip;
  if (attackClipToUse) {
    mSkin.playAnimation(attackClipToUse);
  } else {
    float attackDuration = (mAtkSpeed > 0.f)
                               ? (0.9f / (mAtkSpeed * speedMultiplier))
                               : (0.45f / speedMultiplier);
    mSkin.attack(attackDuration, false, true);
  }

  int facing = mSkin.getLastFacingDir();
  sf::Vector2f atkDir = {static_cast<float>(facing), 0.f};
  if (target) {
    sf::Vector2f diff = target->getPosition() - mPos;
    float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (len > 0.001f) {
      atkDir = diff / len;
    }
  }
  Animation *anim = getAnimation();
  if (anim) {
    float weightFactor = mHasTwoHandedWeapon ? 2.5f : 1.2f;
    anim->applyAttackImpulse(atkDir, weightFactor);
  }
}
