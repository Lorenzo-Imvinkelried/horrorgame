#include "Player.h"
#include "Config.h"
#include "core/managers/TitleManager.h"
#include "core/systems/CultivoSystem.h"
#include "core/systems/WeightSystem.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// --- ¡LA FÓRMULA MÁGICA! ---
void Player::recalculateStats() {
  auto cultivated = CultivoSystem::getInstance().getCultivatedItem();
  if (cultivated && !isEquipped(cultivated)) {
    CultivoSystem::getInstance().setCultivatedItem(nullptr);
  }

  float oldCDR = mCooldownReductionPercent;
  int oldMaxHp = mMaxHp;
  int oldMaxMp = mMaxMp;
  bool wasFullHp = (oldMaxHp > 0) ? (mCurrentHp >= oldMaxHp) : true;
  bool wasFullMp = (oldMaxMp > 0) ? (mCurrentMp >= oldMaxMp) : true;

  // [FIX] Reset Bonus Stats
  mBonusStrength = 0;
  mBonusAgility = 0;
  mBonusIntelligence = 0;
  mBonusVitality = 0;
  mBonusAccuracy = 0;
  mBonusEvasion = 0;
  mBonusAttackRange = 0.f;

  // [RESET PERSISTENT OVERRIDES]
  mExecuteDamagePercent = mExecuteDamagePercentBase;
  mExecuteHealthThresholdPercent = mExecuteHealthThresholdPercentBase;
  mTrueDamagePercent = mTrueDamagePercentBase;
  mTenacityPercent = mTenacityPercentBase;
  mDamageReductionPercent = mDamageReductionPercentBase;
  mCritAvoidancePercent = mCritAvoidancePercentBase;
  mAntiArmorPenPercent = mAntiArmorPenPercentBase;
  mAntiArmorPenFlat = mAntiArmorPenFlatBase;
  mManaStealPercent = mManaStealPercentBase;
  mXpBonusPercent = mXpBonusPercentBase;
  mLifestealPercent = mLifestealPercentBase;
  mCooldownReductionPercent = mCooldownReductionPercentBase;
  mBlockChance = mBlockChanceBase;

  mBleedFlat = mBleedFlatBase;
  mBleedPercent = mBleedPercentBase;
  mBleedDurationFlat = mBleedDurationFlatBase;
  mBleedDurationPercent = mBleedDurationPercentBase;
  mStunChance = mStunChanceBase;
  mStunDuration = mStunDurationBase;
  mSlowMovePercent = mSlowMovePercentBase;
  mSlowMoveDuration = mSlowMoveDurationBase;
  mSlowAttackPercent = mSlowAttackPercentBase;
  mSlowAttackDuration = mSlowAttackDurationBase;

  mAoeRadius = mAoeRadiusBase;
  mAoeDamagePercent = mAoeDamagePercentBase;

  mArmorPenetrationPercent = mArmorPenetrationPercentBase;
  mArmorPenetration = mArmorPenetrationFlatBase;
  mPhysicalDamageBonus = mPhysicalDamageBonusBase;
  mCritDamage = mBaseCritDamage;
  mDoubleStrikeChance = mDoubleStrikeChanceBase;
  mTripleStrikeChance = mTripleStrikeChanceBase;
  mEnemyMaxHpDamagePercent = mEnemyMaxHpDamagePercentBase;
  mBlockValuePercent = mBlockValuePercentBase;
  mThornsPercent = mThornsPercentBase;
  mHpRegenPercent = mHpRegenPercentBase;
  mMpRegenPercent = mMpRegenPercentBase;

  // 1. Sumar Stats de Equipo
  int equipAttack = 0;
  int equipDefense = 0;
  int equipHpBonus = 0;
  int equipMpBonus = 0;
  float equipAtkSpeed = 0.f;
  float equipCritChance = 0.f;
  float equipCritDamage = 0.f;
  float equipSpeedBonus = 0.f;

  float equipAttackPercent = 0.f;
  float equipDefensePercent = 0.f;
  float equipHpPercent = 0.f;
  float equipMpPercent = 0.f;
  float equipStrPercent = 0.f;
  float equipAgiPercent = 0.f;
  float equipIntPercent = 0.f;
  float equipVitPercent = 0.f;

  auto addStats = [&](const ItemStats& stats) {
    equipAttack += stats.physicalDamage;
    equipDefense += stats.defense;
    equipHpBonus += stats.hpBonus;
    equipMpBonus += stats.mpBonus;
    mBonusStrength += stats.strength;
    mBonusAgility += stats.agility;
    mBonusIntelligence += stats.intelligence;
    mBonusVitality += stats.vitality;
    mBonusAccuracy += stats.accuracy;
    mBonusEvasion += stats.evasion;
    equipAtkSpeed += stats.attackSpeed;
    equipCritChance += stats.critChance;
    equipCritDamage += stats.critDamage;
    equipSpeedBonus += stats.moveSpeedBonus;
    mExecuteDamagePercent += stats.executeDamagePercent;
    mExecuteHealthThresholdPercent = std::max(mExecuteHealthThresholdPercent, stats.executeHealthThresholdPercent);
    mTrueDamagePercent += stats.trueDamagePercent;
    mTenacityPercent += stats.tenacity;
    mDamageReductionPercent += stats.damageReduction;
    mCritAvoidancePercent += stats.critAvoidance;
    mAntiArmorPenPercent += stats.antiArmorPenPercent;
    mAntiArmorPenFlat += stats.antiArmorPenFlat;
    mManaStealPercent += stats.manaStealPercent;
    mXpBonusPercent += stats.xpBonusPercent;
    mCooldownReductionPercent += stats.cooldownReductionPercent;
    mLifestealPercent += stats.lifestealPercent;
    mStunChance += stats.stunChance;
    mStunDuration += stats.stunDuration;
    mBleedFlat += stats.bleedFlat;
    mBleedPercent += stats.bleedPercent;
    mBleedDurationFlat += stats.bleedDurationFlat;
    mBleedDurationPercent += stats.bleedDurationPercent;
    mSlowMovePercent += stats.slowMovePercent;
    mSlowMoveDuration += stats.slowMoveDuration;
    mSlowAttackPercent += stats.slowAttackPercent;
    mSlowAttackDuration += stats.slowAttackDuration;
    mAoeRadius += stats.aoeRadius;
    mAoeDamagePercent += stats.aoeDamagePercent;
    mBlockChance += stats.blockChance;
    mBlockValuePercent += stats.blockValuePercent;
    mThornsPercent += stats.thornsPercent;
    mHpRegenPercent += stats.hpRegenPercent;
    mMpRegenPercent += stats.mpRegenPercent;
    equipAttackPercent += stats.attackPercent;
    equipDefensePercent += stats.defensePercent;
    equipHpPercent += stats.hpPercent;
    equipMpPercent += stats.mpPercent;
    equipStrPercent += stats.strengthPercent;
    equipAgiPercent += stats.agilityPercent;
    equipIntPercent += stats.intelligencePercent;
    equipVitPercent += stats.vitalityPercent;
    mPhysicalDamageBonus += stats.physicalDamageBonus;
  };

  auto applyItemStats = [&](const Item &item) {
    addStats(item.stats);
    addStats(item.cultivoBonusStats);
    for (const auto& stone : item.socketedStones) {
      if (stone) {
        addStats(stone->stats);
      }
    }
  };

  for (int i = 0; i < 12; ++i) {
    if (mEquipment[i] != nullptr) {
      applyItemStats(*mEquipment[i]);
    }
  }

  // Title bonuses
  int titleAttack = 0;
  int titleDefense = 0;
  int titleMaxHp = 0;
  int titleMaxMp = 0;
  float titleSpeed = 0.f;
  float titleAtkSpeed = 0.f;
  float titleCritChance = 0.f;
  float titleCritDamage = 0.f;
  int titleAccuracy = 0;
  int titleEvasion = 0;

  // Apply Title Stats
  if (!mActiveTitleId.empty()) {
    const Title *title = TitleManager::getInstance().getTitle(mActiveTitleId);
    if (title) {
      for (const auto &pair : title->stats) {
        Stat stat = pair.first;
        float val = pair.second;
        int valI = static_cast<int>(val);

        switch (stat) {
        case Stat::STR:
          mBonusStrength += valI;
          break;
        case Stat::DEX:
          mBonusAgility += valI;
          break;
        case Stat::INT:
          mBonusIntelligence += valI;
          break;
        case Stat::VIT:
          mBonusVitality += valI;
          break;

        case Stat::ATTACK:
          titleAttack += valI;
          break;
        case Stat::DEFENSE:
          titleDefense += valI;
          break;
        case Stat::MAX_HP:
          titleMaxHp += valI;
          break;
        case Stat::MAX_MP:
          titleMaxMp += valI;
          break;

        case Stat::ACCURACY:
          titleAccuracy += valI;
          break;
        case Stat::EVASION:
          titleEvasion += valI;
          break;

        case Stat::ATK_SPEED:
          titleAtkSpeed += val;
          break;
        case Stat::CRIT_CHANCE:
          titleCritChance += val;
          break;
        case Stat::CRIT_DMG:
          titleCritDamage += val;
          break;

        case Stat::LIFESTEAL:
          mLifestealPercent += val;
          break;
        case Stat::TENACITY:
          mTenacityPercent += val;
          break;
        case Stat::MOVE_SPEED:
          titleSpeed += val;
          break;
        case Stat::ATTACK_RANGE:
          mBonusAttackRange += val;
          break;

        case Stat::ARMOR_PEN_FLAT:
          mArmorPenetration += valI;
          break;
        case Stat::ARMOR_PEN_PERCENT:
          mArmorPenetrationPercent += val;
          break;
        case Stat::TRUE_DMG_PERCENT:
          mTrueDamagePercent += valI;
          break;
        case Stat::EXECUTE_DMG_PERCENT:
          mExecuteDamagePercent += valI;
          break;
        case Stat::EXECUTE_THRESH_PERCENT:
          mExecuteHealthThresholdPercent =
              std::max(mExecuteHealthThresholdPercent, valI);
          break;

        case Stat::BLOCK_CHANCE:
          mBlockChance += val;
          break;
        case Stat::BLOCK_VALUE:
          mBlockValuePercent += val;
          break;
        case Stat::THORNS:
          mThornsPercent += val;
          break;
        case Stat::HP_REGEN:
          mHpRegenPercent += val;
          break;
        case Stat::MP_REGEN:
          mMpRegenPercent += val;
          break;
        case Stat::DMG_REDUCTION:
          mDamageReductionPercent += val;
          break;
        case Stat::CRIT_AVOIDANCE:
          mCritAvoidancePercent += val;
          break;
        case Stat::MANA_STEAL:
          mManaStealPercent += val;
          break;
        case Stat::XP_BONUS:
          mXpBonusPercent += val;
          break;
        case Stat::COOLDOWN_REDUCTION:
          mCooldownReductionPercent += val;
          break;

        case Stat::DOUBLE_STRIKE:
          mDoubleStrikeChance += val;
          break;
        case Stat::TRIPLE_STRIKE:
          mTripleStrikeChance += val;
          break;

        case Stat::STUN_CHANCE:
          mStunChance += val;
          break;
        case Stat::STUN_DURATION:
          mStunDuration += val;
          break;

        case Stat::BLEED_DMG_PERCENT:
          mBleedPercent += val;
          break;
        case Stat::BLEED_FLAT:
          mBleedFlat += valI;
          break;
        case Stat::BLEED_DURATION_FLAT:
          mBleedDurationFlat += val;
          break;
        case Stat::BLEED_DURATION_PERCENT:
          mBleedDurationPercent += val;
          break;

        case Stat::SLOW_MOVE_PERCENT:
          mSlowMovePercent += val;
          break;
        case Stat::SLOW_MOVE_DURATION:
          mSlowMoveDuration += val;
          break;
        case Stat::SLOW_ATTACK_PERCENT:
          mSlowAttackPercent += val;
          break;
        case Stat::SLOW_ATTACK_DURATION:
          mSlowAttackDuration += val;
          break;

        case Stat::AOE_RADIUS:
          mAoeRadius += val;
          break;
        case Stat::AOE_DAMAGE_PERCENT:
          mAoeDamagePercent += val;
          break;

        case Stat::ANTI_ARMOR_PEN_FLAT:
          mAntiArmorPenFlat += valI;
          break;
        case Stat::ANTI_ARMOR_PEN_PERCENT:
          mAntiArmorPenPercent += val;
          break;

        case Stat::PHYSICAL_DAMAGE_BONUS:
          mPhysicalDamageBonus += val;
          break;
        case Stat::ENEMY_MAX_HP_DAMAGE_PERCENT:
          mEnemyMaxHpDamagePercent += val;
          break;

        default:
          break;
        }
      }
    }
  }

  mSpeed = std::max(0.f, mBaseSpeed + titleSpeed +
           (mMorphActive ? mMorphBlueprint.speed : 0.f) + equipSpeedBonus +
           getStatModifier(Stat::MOVE_SPEED));

  if (mIsCharging) {
    mSpeed = 1000.f; // Fixed speed during charge
  }

  mAttackRange = std::max(10.f, cfg::Player::BASE_RANGE + mBonusAttackRange +
                 getStatModifier(Stat::ATTACK_RANGE));

  // 2. Calcular Totales
  int totalStr = std::max(0, mStrength + mBonusStrength + static_cast<int>(getStatModifier(Stat::STR)));
  int totalAgi = std::max(0, mAgility + mBonusAgility + static_cast<int>(getStatModifier(Stat::DEX)));
  int totalInt = std::max(0, mIntelligence + mBonusIntelligence + static_cast<int>(getStatModifier(Stat::INT)));
  int totalVit = std::max(0, mVitality + mBonusVitality + static_cast<int>(getStatModifier(Stat::VIT)));

  if (mMorphActive) {
    totalStr = std::max(0, totalStr + mMorphBlueprint.strength);
    totalAgi = std::max(0, totalAgi + mMorphBlueprint.agility);
    totalInt = std::max(0, totalInt + mMorphBlueprint.intelligence);
    totalVit = std::max(0, totalVit + mMorphBlueprint.vitality);
  }

  // Multiplicadores porcentuales a Atributos
  if (equipStrPercent > 0.001f) totalStr = static_cast<int>(totalStr * (1.0f + equipStrPercent / 100.0f));
  if (equipAgiPercent > 0.001f) totalAgi = static_cast<int>(totalAgi * (1.0f + equipAgiPercent / 100.0f));
  if (equipIntPercent > 0.001f) totalInt = static_cast<int>(totalInt * (1.0f + equipIntPercent / 100.0f));
  if (equipVitPercent > 0.001f) totalVit = static_cast<int>(totalVit * (1.0f + equipVitPercent / 100.0f));

  // A. VITALIDAD (Usando Overrides)
  mMaxHp = std::max(1, (totalVit * mHpPerVit) + mBaseMaxHp + equipHpBonus + titleMaxHp +
           (mMorphActive ? mMorphBlueprint.maxHp : 0) +
           static_cast<int>(getStatModifier(Stat::MAX_HP)));
  if (equipHpPercent > 0.001f) mMaxHp = static_cast<int>(mMaxHp * (1.0f + equipHpPercent / 100.0f));

  mDefense = std::max(0, (totalVit * mDefPerVit) + mBaseDefense + equipDefense + titleDefense +
             (mMorphActive ? mMorphBlueprint.defense : 0) +
             static_cast<int>(getStatModifier(Stat::DEFENSE)));
  if (equipDefensePercent > 0.001f) mDefense = static_cast<int>(mDefense * (1.0f + equipDefensePercent / 100.0f));

  // B. INTELIGENCIA
  mMaxMp = std::max(0, (totalInt * mMpPerInt) + mBaseMaxMp + equipMpBonus + titleMaxMp +
           (mMorphActive ? mMorphBlueprint.maxMp : 0) +
           static_cast<int>(getStatModifier(Stat::MAX_MP)));
  if (equipMpPercent > 0.001f) mMaxMp = static_cast<int>(mMaxMp * (1.0f + equipMpPercent / 100.0f));

  // C. FUERZA
  mAttack = std::max(0, (totalStr * mAtkPerStr) + mBaseAttack + equipAttack + titleAttack +
            (mMorphActive ? mMorphBlueprint.attack : 0) +
            static_cast<int>(getStatModifier(Stat::ATTACK)));
  if (equipAttackPercent > 0.001f) mAttack = static_cast<int>(mAttack * (1.0f + equipAttackPercent / 100.0f));

  // D. DESTREZA
  float agiUnits = (float)totalAgi / 10.0f;

  // Accuracy / Evasion (Usando Overrides)
  mAccuracy = std::max(0, mBaseAccuracy + mBonusAccuracy + titleAccuracy +
              static_cast<int>(getStatModifier(Stat::ACCURACY)));
  mEvasion = std::max(0, mBaseEvasion + mBonusEvasion + titleEvasion +
             static_cast<int>(getStatModifier(Stat::EVASION)));

  // Crit
  float rawCrit = mBaseCritChance + equipCritChance + titleCritChance +
                  (agiUnits * 1.0f) + getStatModifier(Stat::CRIT_CHANCE);
  mCritChance = std::clamp(rawCrit, 0.0f, 100.0f);
  mCritDamage = std::max(0.f, mBaseCritDamage + equipCritDamage + titleCritDamage +
                getStatModifier(Stat::CRIT_DMG));

  // Velocidad de Ataque:
  float modAtkSpeed = getStatModifier(Stat::ATK_SPEED);
  mAtkSpeed = std::max(0.05f, mBaseAtkSpeed + equipAtkSpeed + titleAtkSpeed +
              (agiUnits * mAtkSpeedPerAgi) + modAtkSpeed);

  // LIFESTEAL
  mLifestealPercent = std::max(0.f, mLifestealPercent + getStatModifier(Stat::LIFESTEAL));

  // Armor Pen
  mArmorPenetrationPercent = std::max(0.f, mArmorPenetrationPercent + getStatModifier(Stat::ARMOR_PEN_PERCENT));
  mArmorPenetration = std::max(0, mArmorPenetration + static_cast<int>(getStatModifier(Stat::ARMOR_PEN_FLAT)));

  // Multi Strike
  mDoubleStrikeChance += getStatModifier(Stat::DOUBLE_STRIKE);
  mTripleStrikeChance += getStatModifier(Stat::TRIPLE_STRIKE);

  // Defensive & Utility
  mBlockChance += getStatModifier(Stat::BLOCK_CHANCE);
  mBlockValuePercent += getStatModifier(Stat::BLOCK_VALUE);
  mThornsPercent += getStatModifier(Stat::THORNS);
  mHpRegenPercent += getStatModifier(Stat::HP_REGEN);
  mMpRegenPercent += getStatModifier(Stat::MP_REGEN);
  mTenacityPercent += getStatModifier(Stat::TENACITY);
  mDamageReductionPercent += getStatModifier(Stat::DMG_REDUCTION);
  mCritAvoidancePercent += getStatModifier(Stat::CRIT_AVOIDANCE);
  mManaStealPercent += getStatModifier(Stat::MANA_STEAL);
  mXpBonusPercent += getStatModifier(Stat::XP_BONUS);
  mCooldownReductionPercent += getStatModifier(Stat::COOLDOWN_REDUCTION);

  // [OPTIMIZATION] Pre-calcular factores para evitar divisiones en CombatSystem
  mExecuteDamageMultiplier =
      1.0f + (static_cast<float>(mExecuteDamagePercent +
                                 getStatModifier(Stat::EXECUTE_DMG_PERCENT)) /
              100.0f);
  mExecuteThresholdFactor =
      static_cast<float>(mExecuteHealthThresholdPercent +
                         getStatModifier(Stat::EXECUTE_THRESH_PERCENT)) /
      100.0f;
  mTrueDamagePercent +=
      static_cast<int>(getStatModifier(Stat::TRUE_DMG_PERCENT));

  mBleedPercent += getStatModifier(Stat::BLEED_DMG_PERCENT);
  mStunChance += getStatModifier(Stat::STUN_CHANCE);
  mStunDuration += getStatModifier(Stat::STUN_DURATION);

  mBleedFlat += static_cast<int>(getStatModifier(Stat::BLEED_FLAT));
  mBleedDurationFlat += getStatModifier(Stat::BLEED_DURATION_FLAT);
  mBleedDurationPercent += getStatModifier(Stat::BLEED_DURATION_PERCENT);

  mSlowMovePercent += getStatModifier(Stat::SLOW_MOVE_PERCENT);
  mSlowMoveDuration += getStatModifier(Stat::SLOW_MOVE_DURATION);
  mSlowAttackPercent += getStatModifier(Stat::SLOW_ATTACK_PERCENT);
  mSlowAttackDuration += getStatModifier(Stat::SLOW_ATTACK_DURATION);

  mAoeRadius += getStatModifier(Stat::AOE_RADIUS);
  mAoeDamagePercent += getStatModifier(Stat::AOE_DAMAGE_PERCENT);

  mAntiArmorPenFlat +=
      static_cast<int>(getStatModifier(Stat::ANTI_ARMOR_PEN_FLAT));
  mAntiArmorPenPercent += getStatModifier(Stat::ANTI_ARMOR_PEN_PERCENT);

  mPhysicalDamageBonus += getStatModifier(Stat::PHYSICAL_DAMAGE_BONUS);
  mEnemyMaxHpDamagePercent +=
      getStatModifier(Stat::ENEMY_MAX_HP_DAMAGE_PERCENT);

  // Scale or clamp Current HP/MP
  if (wasFullHp && isAlive()) {
    mCurrentHp = mMaxHp;
  } else if (isAlive() && oldMaxHp > 0) {
    float ratio = (float)mCurrentHp / (float)oldMaxHp;
    mCurrentHp = std::max(1, static_cast<int>(std::round(ratio * mMaxHp)));
    if (mCurrentHp > mMaxHp)
      mCurrentHp = mMaxHp;
  } else {
    if (mCurrentHp > mMaxHp)
      mCurrentHp = mMaxHp;
  }

  if (wasFullMp && isAlive()) {
    mCurrentMp = mMaxMp;
  } else if (isAlive() && oldMaxMp > 0) {
    float ratio = (float)mCurrentMp / (float)oldMaxMp;
    mCurrentMp = static_cast<int>(std::round(ratio * mMaxMp));
    if (mCurrentMp > mMaxMp)
      mCurrentMp = mMaxMp;
  } else {
    if (mCurrentMp > mMaxMp)
      mCurrentMp = mMaxMp;
  }

  if (!isAlive()) {
    mCurrentHp = 0;
    mCurrentMp = 0;
  }

  notifyStatsChanged(); // [OBSERVER]

  float newCDR = mCooldownReductionPercent;
  if (std::abs(oldCDR - newCDR) > 0.001f) {
      recalculateCooldowns(oldCDR, newCDR);
  }
}

void Player::debugBoostStats() {
  mAgility += cfg::Debug::STAT_BOOST;
  recalculateStats();
  std::cout << "[DEBUG] Stats subidos! Nueva Velocidad: " << mAtkSpeed << "\n";
}

bool Player::debugAddStat(const std::string &statL, float amount, bool isFixed) {
  int amountI = static_cast<int>(amount);
  bool recognized = true;

  #define SET_OR_ADD(var, val) if (isFixed) var = (val); else var += (val)

  // Base Attributes
  if (statL == "str")
    SET_OR_ADD(mStrength, amountI);
  else if (statL == "dex" || statL == "agi")
    SET_OR_ADD(mAgility, amountI);
  else if (statL == "int")
    SET_OR_ADD(mIntelligence, amountI);
  else if (statL == "vit")
    SET_OR_ADD(mVitality, amountI);
  else if (statL == "maxhp")
    SET_OR_ADD(mBaseMaxHp, amountI);
  else if (statL == "maxmp")
    SET_OR_ADD(mBaseMaxMp, amountI);
  else if (statL == "attack" || statL == "atk")
    SET_OR_ADD(mBaseAttack, amountI);
  else if (statL == "defense" || statL == "def")
    SET_OR_ADD(mBaseDefense, amountI);
  else if (statL == "peso" || statL == "weight")
    setWeightKg(isFixed ? amount : getWeightKg() + amount);

  // Combat Stats
  else if (statL == "speed")
    SET_OR_ADD(mBaseSpeed, amount);
  else if (statL == "atkspeed")
    SET_OR_ADD(mBaseAtkSpeed, amount);
  else if (statL == "atkspeedperagi")
    SET_OR_ADD(mAtkSpeedPerAgi, amount);
  else if (statL == "accuracy")
    SET_OR_ADD(mBaseAccuracy, amountI);
  else if (statL == "evasion")
    SET_OR_ADD(mBaseEvasion, amountI);
  else if (statL == "malice" || statL == "malicia")
    SET_OR_ADD(mMalice, amount);
  else if (statL == "range" || statL == "attack_range") {
    if (isFixed) {
      setStatModifier(Stat::ATTACK_RANGE, amount);
    } else {
      addStatModifier(Stat::ATTACK_RANGE, amount);
    }
  }

  // Offensive %
  else if (statL == "armorpenpercent")
    SET_OR_ADD(mArmorPenetrationPercentBase, amount);
  else if (statL == "armorpenflat")
    SET_OR_ADD(mArmorPenetrationFlatBase, amountI);
  else if (statL == "physicaldmgbonus")
    SET_OR_ADD(mPhysicalDamageBonusBase, amount);
  else if (statL == "critchance")
    SET_OR_ADD(mBaseCritChance, amount);
  else if (statL == "critdamage")
    SET_OR_ADD(mBaseCritDamage, amount);
  else if (statL == "lifestealpercent")
    SET_OR_ADD(mLifestealPercentBase, amount);
  else if (statL == "doublestrikechance" || statL == "doublechance")
    SET_OR_ADD(mDoubleStrikeChanceBase, amount);
  else if (statL == "triplestrikechance" || statL == "triplechance")
    SET_OR_ADD(mTripleStrikeChanceBase, amount);
  else if (statL == "enemymaxhpdamagepercent")
    SET_OR_ADD(mEnemyMaxHpDamagePercentBase, amount);
  else if (statL == "manastealpercent")
    SET_OR_ADD(mManaStealPercentBase, amount);
  else if (statL == "truedamagepercent")
    SET_OR_ADD(mTrueDamagePercentBase, amountI);
  else if (statL == "aoeradius")
    SET_OR_ADD(mAoeRadiusBase, amount);
  else if (statL == "aoedamagepercent")
    SET_OR_ADD(mAoeDamagePercentBase, amount);

  // Defensive %
  else if (statL == "blockchance")
    SET_OR_ADD(mBlockChanceBase, amount);
  else if (statL == "blockvaluepercent")
    SET_OR_ADD(mBlockValuePercentBase, amount);
  else if (statL == "thornspercent")
    SET_OR_ADD(mThornsPercentBase, amount);
  else if (statL == "tenacitypercent")
    SET_OR_ADD(mTenacityPercentBase, amount);
  else if (statL == "damagereductionpercent")
    SET_OR_ADD(mDamageReductionPercentBase, amount);
  else if (statL == "critavoidancepercent")
    SET_OR_ADD(mCritAvoidancePercentBase, amount);
  else if (statL == "antiarmorpenpercent")
    SET_OR_ADD(mAntiArmorPenPercentBase, amount);
  else if (statL == "antiarmorpenflat")
    SET_OR_ADD(mAntiArmorPenFlatBase, amountI);

  // Scaling & Regeneration
  else if (statL == "hppervit")
    SET_OR_ADD(mHpPerVit, amountI);
  else if (statL == "defpervit")
    SET_OR_ADD(mDefPerVit, amountI);
  else if (statL == "mpperint")
    SET_OR_ADD(mMpPerInt, amountI);
  else if (statL == "atkperstr")
    SET_OR_ADD(mAtkPerStr, amountI);
  else if (statL == "hpregenpercent")
    SET_OR_ADD(mHpRegenPercentBase, amount);
  else if (statL == "mpregenpercent")
    SET_OR_ADD(mMpRegenPercentBase, amount);
  else if (statL == "xpbonuspercent")
    SET_OR_ADD(mXpBonusPercentBase, amount);
  else if (statL == "cooldownreductionpercent" || statL == "cdr")
    SET_OR_ADD(mCooldownReductionPercentBase, amount);
  else if (statL == "executedamagepercent")
    SET_OR_ADD(mExecuteDamagePercentBase, amountI);
  else if (statL == "executethresholdpercent")
    SET_OR_ADD(mExecuteHealthThresholdPercentBase, amountI);

  // Status Effects
  else if (statL == "bleeddurationflat")
    SET_OR_ADD(mBleedDurationFlatBase, amount);
  else if (statL == "bleeddurationpercent")
    SET_OR_ADD(mBleedDurationPercentBase, amount);
  else if (statL == "bleedflat")
    SET_OR_ADD(mBleedFlatBase, amountI);
  else if (statL == "bleedpercent")
    SET_OR_ADD(mBleedPercentBase, amount);
  else if (statL == "stunchance")
    SET_OR_ADD(mStunChanceBase, amount);
  else if (statL == "stunduration")
    SET_OR_ADD(mStunDurationBase, amount);
  else if (statL == "slowmovepercent")
    SET_OR_ADD(mSlowMovePercentBase, amount);
  else if (statL == "slowmoveduration")
    SET_OR_ADD(mSlowMoveDurationBase, amount);
  else if (statL == "slowattackpercent")
    SET_OR_ADD(mSlowAttackPercentBase, amount);
  else if (statL == "slowattackduration")
    SET_OR_ADD(mSlowAttackDurationBase, amount);
  else {
    recognized = false;
  }

  #undef SET_OR_ADD

  if (recognized) {
    recalculateStats();
  }

  return recognized;
}
