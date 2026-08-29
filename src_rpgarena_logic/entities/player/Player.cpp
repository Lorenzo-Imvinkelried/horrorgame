#include "Player.h"
#include "Config.h"
#include "core/managers/TitleManager.h"
#include "core/systems/WeightSystem.h"
#include <algorithm>
#include <iostream>

Player::Player() {
  // 1. Configuración Inicial Nivel 1
  mLevel = 1;

  // Configuración Inicial de XP
  mCurrentExp = 0;
  mNextLevelExp = cfg::Player::BASE_NEXT_LEVEL_EXP;

  // 2. ATRIBUTOS BASE (10 en cada uno)
  mStrength = cfg::Player::BASE_STR;
  mAgility = cfg::Player::BASE_DEX;
  mIntelligence = cfg::Player::BASE_INT;
  mVitality = cfg::Player::BASE_VIT;

  // [NEW PERSISTENT BASE STATS]
  mHpPerVit = cfg::Player::HP_PER_VIT;
  mDefPerVit = cfg::Player::DEF_PER_VIT;
  mMpPerInt = cfg::Player::MP_PER_INT;
  mAtkPerStr = cfg::Player::ATK_PER_STR;
  mAtkSpeedPerAgi = cfg::Player::ATK_SPEED_PER_AGI;
  // 3. Modificadores base (Persistentes)
  mBaseSpeed = cfg::Player::SPEED;
  mBaseAtkSpeed = cfg::Player::BASE_ATK_SPEED;
  mBaseAccuracy = cfg::Player::BASE_ACCURACY;
  mBaseEvasion = cfg::Player::BASE_EVASION;
  mMalice = cfg::Player::BASE_MALICE;
  mBaseCritChance = cfg::Player::BASE_CRIT_CHANCE;
  mBaseCritDamage = cfg::Player::BASE_CRIT_DAMAGE;

  mPhysicalDamageBonusBase = cfg::Player::BASE_PHYSICAL_DMG_BONUS;
  mArmorPenetrationPercentBase = cfg::Player::BASE_ARMOR_PEN_PERCENT;
  mArmorPenetrationFlatBase = cfg::Player::BASE_ARMOR_PEN_FLAT;

  mLifestealPercentBase = cfg::Player::BASE_LIFESTEAL_PERCENT;
  mDoubleStrikeChanceBase = cfg::Player::BASE_DOUBLE_STRIKE_CHANCE;
  mTripleStrikeChanceBase = cfg::Player::BASE_TRIPLE_STRIKE_CHANCE;
  mEnemyMaxHpDamagePercentBase = cfg::Player::BASE_ENEMY_MAX_HP_DAMAGE_PERCENT;

  mBlockChanceBase = cfg::Player::BASE_BLOCK_CHANCE;
  mBlockValuePercentBase = cfg::Player::BASE_BLOCK_VALUE_PERCENT;
  mThornsPercentBase = cfg::Player::BASE_THORNS_PERCENT;
  mHpRegenPercentBase = cfg::Player::BASE_HP_REGEN_PERCENT;
  mMpRegenPercentBase = cfg::Player::BASE_MP_REGEN_PERCENT;

  mTenacityPercentBase = cfg::Player::BASE_TENACITY_PERCENT;
  mDamageReductionPercentBase = cfg::Player::BASE_DAMAGE_REDUCTION_PERCENT;
  mCritAvoidancePercentBase = cfg::Player::BASE_CRIT_AVOIDANCE_PERCENT;
  mAntiArmorPenPercentBase = cfg::Player::BASE_ANTI_ARMOR_PEN_PERCENT;
  mAntiArmorPenFlatBase = cfg::Player::BASE_ANTI_ARMOR_PEN_FLAT;
  mManaStealPercentBase = cfg::Player::BASE_MANA_STEAL_PERCENT;
  mXpBonusPercentBase = cfg::Player::BASE_XP_BONUS_PERCENT;
  mCooldownReductionPercentBase = cfg::Player::BASE_COOLDOWN_REDUCTION_PERCENT;

  mAoeRadiusBase = cfg::Player::BASE_AOE_RADIUS;
  mAoeDamagePercentBase = cfg::Player::BASE_AOE_DAMAGE_PERCENT;

  mBleedFlatBase = cfg::Player::BASE_BLEED_FLAT;
  mBleedPercentBase = cfg::Player::BASE_BLEED_PERCENT;
  mBleedDurationFlatBase = cfg::Player::BASE_BLEED_DURATION_FLAT;
  mBleedDurationPercentBase = cfg::Player::BASE_BLEED_DURATION_PERCENT;

  mStunChanceBase = cfg::Player::BASE_STUN_CHANCE;
  mStunDurationBase = cfg::Player::BASE_STUN_DURATION;

  mSlowMovePercentBase = cfg::Player::BASE_SLOW_MOVE_PERCENT;
  mSlowMoveDurationBase = cfg::Player::BASE_SLOW_MOVE_DURATION;
  mSlowAttackPercentBase = cfg::Player::BASE_SLOW_ATTACK_PERCENT;
  mSlowAttackDurationBase = cfg::Player::BASE_SLOW_ATTACK_DURATION;

  mExecuteDamagePercentBase = cfg::Player::BASE_EXECUTE_DAMAGE_PERCENT;
  mExecuteHealthThresholdPercentBase =
      cfg::Player::BASE_EXECUTE_THRESHOLD_PERCENT;
  mTrueDamagePercentBase = cfg::Player::BASE_TRUE_DAMAGE_PERCENT;

  mBaseMaxHp = 0;
  mBaseMaxMp = 0;
  mBaseAttack = 0;
  mBaseDefense = 0;

  // Temporal bonus reset
  mBonusStrength = 0;
  mBonusAgility = 0;
  mBonusIntelligence = 0;
  mBonusVitality = 0;
  mBonusAccuracy = 0;
  mBonusEvasion = 0;
  mBonusAttackRange = 0.f;
  mIsCharging = false;

  // [WEIGHT SYSTEM]
  setWeightKg(cfg::Weight::PLAYER_DEFAULT_KG);

  // 4. CALCULAR STATS DERIVADOS
  recalculateStats();

  // 5. Llenar vida y maná al máximo al nacer
  mCurrentHp = mMaxHp;
  mCurrentMp = mMaxMp;

  // Velocidad de movimiento del personaje (mundo)
  mSpeed = cfg::Player::SPEED;

  // [SKILLS] Init
  mEquippedSkills[0] = 1; // Power Strike
  mEquippedSkills[1] = 2; // Berserker Fury
  mEquippedSkills[2] = 3; // Metamorphosis

  notifyStatsChanged();
}

// [SKILLS] Implementation
void Player::equipSkill(int slotIndex, int skillId) {
  mEquippedSkills[slotIndex] = skillId;
}

int Player::getEquippedSkill(int slotIndex) const {
  auto it = mEquippedSkills.find(slotIndex);
  if (it != mEquippedSkills.end())
    return it->second;
  return -1; // None
}

bool Player::isEquipped(const std::shared_ptr<Item>& item) const {
  if (!item) return false;
  for (int i = 0; i < static_cast<int>(EquipmentSlot::Count); ++i) {
    if (mEquipment[i] == item) return true;
  }
  return false;
}

void Player::setWorldBounds(sf::FloatRect bounds) {
  mWorldBounds = bounds;
}

void Player::heal(int amount) {
  if (mCurrentHp <= 0)
    return; // Cannot heal a dead player
  if (amount <= 0)
    return;
  mCurrentHp += amount;
  if (mCurrentHp > getMaxHp())
    mCurrentHp = getMaxHp();
  notifyStatsChanged(); // [OBSERVER]
}

void Player::restoreMana(int amount) {
  if (mCurrentHp <= 0)
    return; // Cannot restore mana if dead
  mCurrentMp += amount;
  if (mCurrentMp < 0)
    mCurrentMp = 0;
  if (mCurrentMp > getMaxMp())
    mCurrentMp = getMaxMp();
  notifyStatsChanged(); // [OBSERVER]
}

void Player::setCurrentHp(int hp) {
  mCurrentHp = std::max(0, std::min(hp, getMaxHp()));
  notifyStatsChanged(); // [OBSERVER]
}

void Player::setCurrentMp(int mp) {
  mCurrentMp = std::max(0, std::min(mp, getMaxMp()));
  notifyStatsChanged(); // [OBSERVER]
}

void Player::setCharging(bool charging) {
  mIsCharging = charging;
  recalculateStats();
}

float Player::getAttackRange() const {
  return mAttackRange;
}

std::string Player::getTitleName() const {
  if (mActiveTitleId.empty())
    return "";
  const Title *title = TitleManager::getInstance().getTitle(mActiveTitleId);
  return title ? title->name : "";
}

sf::Color Player::getTitleColor() const {
  if (mActiveTitleId.empty())
    return sf::Color::White;
  const Title *title = TitleManager::getInstance().getTitle(mActiveTitleId);
  return title ? title->color : sf::Color::White;
}

// [BUFFS]
void Player::onBuffsChanged() {
  recalculateStats();
  notifyStatsChanged();
}
