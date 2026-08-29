#include "Player.h"
#include "Config.h"
#include "entities/mob/Mob.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/CultivoSystem.h"
#include "core/systems/DashSystem.h"
#include "core/systems/SoundSystem.h"
#include <iostream>
#include <vector>

int Player::takeDamage(int damageAmount, Entity *attacker, bool isCrit,
                       bool isTrueDamage) {
  // [DASH I-FRAMES] Completely dodge damage during active dash
  if (auto* ds = DashSystem::getInstance()) {
    if (ds->isInvulnerable(this) && !isTrueDamage) {
      if (auto* cs = CombatSystem::getInstance()) {
        cs->getFeedback().onMiss(this, attacker);
      }
      return 0;
    }
  }

  // [COMBAT FIX]
  mLastHitTimer.restart();

  // [AGGRO LIST] Si alguien nos pega, lo agregamos a la lista
  if (attacker && attacker->isAlive()) {
    addToAggro(attacker);
    setLastAttacker(attacker);
  }

  // 3. Aplicar Daño
  bool wasAlive = (mCurrentHp > 0);
  mCurrentHp -= damageAmount;
  if (mCurrentHp < 0)
    mCurrentHp = 0;
  if (mCurrentHp > getMaxHp())
    mCurrentHp = getMaxHp();

  if (wasAlive && mCurrentHp <= 0) {
    mCurrentHp = 0;
    mCurrentMp = 0;
    
    // Penalidad de EXP al morir: perder 10% del total requerido para el nivel
    int xpLoss = static_cast<int>(mNextLevelExp * 0.10f);
    mCurrentExp -= xpLoss;
    if (mCurrentExp < 0) {
      mCurrentExp = 0;
    }
  }

  notifyStatsChanged(); // [OBSERVER]

  return damageAmount;
}

int Player::addExperience(int amount, bool applyModifiers) {
  // [XP BONUS]
  if (applyModifiers && mXpBonusPercent > 0.f) {
    float bonus = amount * (mXpBonusPercent / 100.0f);
    amount += static_cast<int>(bonus);
  }

  // [CULTIVO SYSTEM EXP REDIRECTION]
  if (CultivoSystem::getInstance().hasCultivatedItem()) {
    auto cultivatedItem = CultivoSystem::getInstance().getCultivatedItem();
    if (cultivatedItem) {
      if (!isEquipped(cultivatedItem)) {
        CultivoSystem::getInstance().setCultivatedItem(nullptr);
      } else if (cultivatedItem->cultivoLocked) {
        bool leveledUp = CultivoSystem::getInstance().addExp(amount);
        if (leveledUp) {
          recalculateStats();
        }
        return amount; // Player gets 0 EXP
      }
    }
  }

  mCurrentExp += amount;

  checkLevelUp();
  notifyStatsChanged(); // [OBSERVER] XP Changed
  return amount;
}

void Player::checkLevelUp() {
  int safetyCount = 0;
  while (mCurrentExp >= mNextLevelExp) {
    if (++safetyCount > 100) {
      std::cerr << "[Player] WARNING: LevelUp loop protection hit!\n";
      mCurrentExp = mNextLevelExp - 1;
      break;
    }

    // 1. Restar la XP usada para este nivel
    mCurrentExp -= mNextLevelExp;

    // 2. Subir Nivel
    mLevel++;

    // 3. Calcular nueva meta (Curva Exponencial)
    mNextLevelExp = static_cast<long long>(mNextLevelExp *
                                           cfg::Player::EXP_CURVE_MULTIPLIER);

    // 4. Recompensa de Stats
    mStrength += cfg::Player::STAT_GAIN_ON_LEVEL_UP;
    mAgility += cfg::Player::STAT_GAIN_ON_LEVEL_UP;
    mIntelligence += cfg::Player::STAT_GAIN_ON_LEVEL_UP;
    mVitality += cfg::Player::STAT_GAIN_ON_LEVEL_UP;

    recalculateStats();

    if (isAlive()) {
      mCurrentHp = mMaxHp;
      mCurrentMp = mMaxMp;
    } else {
      mCurrentHp = 0;
      mCurrentMp = 0;
    }

    std::cout << "¡LEVEL UP! Ahora eres nivel " << mLevel
              << ". Siguiente meta: " << mNextLevelExp << "\n";

    if (auto *ss = SoundSystem::getInstance()) {
      ss->playSound("assets/sounds/player/level_up.wav");
    }
  }
}

bool Player::wasHitRecently(float seconds) const {
  return mLastHitTimer.getElapsedTime().asSeconds() < seconds;
}

void Player::addToAggro(Entity *entity) {
  if (entity && entity->isAlive() && entity != this &&
      !entity->isReturningToSpawn()) {
    if (Mob *mob = dynamic_cast<Mob *>(entity)) {
      if (mob->getStance() == MobStance::Passive)
        return;
    }
    mAggroList.insert(entity);
  }
}

void Player::removeFromAggro(Entity *entity) {
  if (entity) {
    mAggroList.erase(entity);
  }
}

void Player::updateAggro(sf::Time dt) {
  static sf::Clock cleanupClock;
  if (cleanupClock.getElapsedTime().asSeconds() < 0.5f)
    return;
  cleanupClock.restart();

  if (mAggroList.empty())
    return;

  const float CLEANUP_DIST_SQ = 1200.f * 1200.f;

  std::vector<Entity *> toRemove;
  toRemove.reserve(10);

  for (Entity *e : mAggroList) {
    if (!e) {
      toRemove.push_back(e);
      continue;
    }

    if (!e->isAlive()) {
      toRemove.push_back(e);
      continue;
    }

    if (e->isReturningToSpawn()) {
      toRemove.push_back(e);
      continue;
    }

    float dx = e->getPosition().x - mPos.x;
    float dy = e->getPosition().y - mPos.y;
    if ((dx * dx + dy * dy) > CLEANUP_DIST_SQ) {
      toRemove.push_back(e);
    }
  }

  for (Entity *e : toRemove) {
    if (e) {
      if (auto *mob = dynamic_cast<Mob *>(e)) {
        mob->resetAggro(this);
      }
    }
    mAggroList.erase(e);
  }
}
