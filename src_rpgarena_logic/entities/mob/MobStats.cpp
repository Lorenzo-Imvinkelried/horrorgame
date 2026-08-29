#include "Mob.h"
#include "core/items/Item.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

void Mob::recalculateStats() {
    int oldMaxHp = mMaxHp;
    int oldMaxMp = mMaxMp;
    bool wasFullHp = (mCurrentHp >= oldMaxHp || mCurrentHp == 0);
    bool wasFullMp = (mCurrentMp >= oldMaxMp || mCurrentMp == 0);

    // 1. Reset all stats to blueprint base with scaling
    mName = mBlueprint.name;
    if (mLevel <= 0) {
        mLevel = mBlueprint.level;
    }
    mMaxHp = mBlueprint.maxHp;
    mMaxMp = mBlueprint.maxMp;
    mStrength = mBlueprint.strength;
    mAgility = mBlueprint.agility;
    mIntelligence = mBlueprint.intelligence;
    mVitality = mBlueprint.vitality;
    mBaseAttack = mBlueprint.attack;
    mDefense = mBlueprint.defense;
    mXp = mBlueprint.xp;
    mAtkSpeed = mBlueprint.atkSpeed;
    mCritChance = mBlueprint.critChance;
    mCritDamage = mBlueprint.critDamage;
    mArmorPenetration = mBlueprint.armorPenetration;
    mAccuracy = mBlueprint.accuracy;
    mEvasion = mBlueprint.evasion;
    mBaseMovementSpeed = mBlueprint.speed > 0.f ? mBlueprint.speed : 20.f;
    setSpeed(mBlueprint.speed);
    mAttackRange = mBlueprint.attackRange > 0.f ? mBlueprint.attackRange : cfg::Mob::BASE_RANGE;
    setWeightKg(mBlueprint.weightKg);

    int diff = mLevel - mBlueprint.level;
    if (diff > 0) {
        mMaxHp = static_cast<int>(mBlueprint.maxHp * (1.0f + diff * 0.2f + diff * diff * 0.12f));
        mBaseAttack = static_cast<int>(mBlueprint.attack * (1.0f + diff * 0.12f + diff * diff * 0.005f));
        mDefense = static_cast<int>(mBlueprint.defense * (1.0f + diff * 0.1f + diff * diff * 0.003f));
        mXp = static_cast<int>(mBlueprint.xp * (1.0f + diff * 0.3f + diff * diff * 0.01f));
        mStrength += diff;
        mAgility += diff;
        mIntelligence += diff;
        mVitality += diff;
    }
    mAttack = mBaseAttack;
    
    mArmorPenetrationPercent = mBlueprint.armorPenetrationPercent;
    mPhysicalDamageBonus = (mBlueprint.physicalDamageBonus > 0.1f) ? mBlueprint.physicalDamageBonus : 100.f;
    mLifestealPercent = mBlueprint.lifestealPercent;
    mCooldownReductionPercent = mBlueprint.cooldownReductionPercent;
    
    mStunChance = mBlueprint.stunChance;
    mStunDuration = mBlueprint.stunDuration;

    mSlowMovePercent = mBlueprint.slowMovePercent;
    mSlowMoveDuration = mBlueprint.slowMoveDuration;
    mSlowAttackPercent = mBlueprint.slowAttackPercent;
    mSlowAttackDuration = mBlueprint.slowAttackDuration;

    mTenacityPercent = mBlueprint.tenacityPercent;
    mDamageReductionPercent = mBlueprint.damageReductionPercent;
    mCritAvoidancePercent = mBlueprint.critAvoidancePercent;
    mAntiArmorPenPercent = mBlueprint.antiArmorPenPercent;
    mAntiArmorPenFlat = mBlueprint.antiArmorPenFlat;
    mManaStealPercent = mBlueprint.manaStealPercent;
    mXpBonusPercent = mBlueprint.xpBonusPercent;

    mBleedDurationFlat = mBlueprint.bleedDurationFlat;
    mBleedDurationPercent = mBlueprint.bleedDurationPercent;
    mBleedFlat = mBlueprint.bleedFlat;
    mBleedPercent = mBlueprint.bleedPercent;

    mDoubleStrikeChance = mBlueprint.doubleStrikeChance;
    mTripleStrikeChance = mBlueprint.tripleStrikeChance;
    mEnemyMaxHpDamagePercent = mBlueprint.enemyMaxHpDamagePercent;

    mBlockChance = mBlueprint.blockChance;
    mBlockValuePercent = mBlueprint.blockValuePercent;
    mThornsPercent = mBlueprint.thornsPercent;
    mHpRegenPercent = mBlueprint.hpRegenPercent;
    mMpRegenPercent = mBlueprint.mpRegenPercent;

    mAoeRadius = mBlueprint.aoeRadius;
    mAoeDamagePercent = mBlueprint.aoeDamagePercent;
    mExecuteDamagePercent = mBlueprint.executeDamagePercent;
    mTrueDamagePercent = mBlueprint.trueDamagePercent;

    if (mBlueprint.groundOffsetY != 0.f) {
        mGroundOffsetY = mBlueprint.groundOffsetY;
    } else {
        mGroundOffsetY = mSkin.getGroundOffsetY();
    }
    mBaseAnimSpeed = mBlueprint.baseAnimSpeed > 0.f ? mBlueprint.baseAnimSpeed : 1.f;

    // 2. Accumulate weapon stats
    int equipAttack = 0;
    int bonusStr = 0;
    int bonusVit = 0;
    int bonusInt = 0;
    int bonusAgi = 0;
    int equipHpBonus = 0;
    int equipMpBonus = 0;
    float equipAtkSpeed = 0.f;
    float equipSpeedBonus = 0.f;

    float equipAttackPercent = 0.f;
    float equipDefensePercent = 0.f;
    float equipHpPercent = 0.f;
    float equipMpPercent = 0.f;
    float equipStrPercent = 0.f;
    float equipAgiPercent = 0.f;
    float equipIntPercent = 0.f;
    float equipVitPercent = 0.f;

    auto applyItemStats = [&](const std::shared_ptr<Item>& item) {
        if (!item) return;
        mStrength += item->stats.strength;
        mAgility += item->stats.agility;
        mIntelligence += item->stats.intelligence;
        mVitality += item->stats.vitality;
        
        bonusStr += item->stats.strength;
        bonusVit += item->stats.vitality;
        bonusInt += item->stats.intelligence;
        bonusAgi += item->stats.agility;

        equipHpBonus += item->stats.hpBonus;
        equipMpBonus += item->stats.mpBonus;
        equipAtkSpeed += item->stats.attackSpeed;
        equipSpeedBonus += item->stats.moveSpeedBonus;

        equipAttack += item->stats.physicalDamage;
        mDefense += item->stats.defense;
        mAccuracy += item->stats.accuracy;
        mEvasion += item->stats.evasion;
        mCritChance += item->stats.critChance;
        mCritDamage += item->stats.critDamage;
        mArmorPenetration += item->stats.armorPenetration;
        
        mTenacityPercent += item->stats.tenacity;
        mDamageReductionPercent += item->stats.damageReduction;
        mCritAvoidancePercent += item->stats.critAvoidance;
        mAntiArmorPenPercent += item->stats.antiArmorPenPercent;
        mAntiArmorPenFlat += item->stats.antiArmorPenFlat;
        mManaStealPercent += item->stats.manaStealPercent;
        mXpBonusPercent += item->stats.xpBonusPercent;

        mLifestealPercent += item->stats.lifestealPercent;
        mCooldownReductionPercent += item->stats.cooldownReductionPercent;
        mStunChance += item->stats.stunChance;
        mStunDuration += item->stats.stunDuration;
        mBleedFlat += item->stats.bleedFlat;
        mBleedPercent += item->stats.bleedPercent;
        mBleedDurationFlat += item->stats.bleedDurationFlat;
        mBleedDurationPercent += item->stats.bleedDurationPercent;
        mSlowMovePercent += item->stats.slowMovePercent;
        mSlowMoveDuration += item->stats.slowMoveDuration;
        mSlowAttackPercent += item->stats.slowAttackPercent;
        mSlowAttackDuration += item->stats.slowAttackDuration;
        mAoeRadius += item->stats.aoeRadius;
        mAoeDamagePercent += item->stats.aoeDamagePercent;

        mBlockChance += item->stats.blockChance;
        mBlockValuePercent += item->stats.blockValuePercent;
        mThornsPercent += item->stats.thornsPercent;
        mHpRegenPercent += item->stats.hpRegenPercent;
        mMpRegenPercent += item->stats.mpRegenPercent;
        mPhysicalDamageBonus += item->stats.physicalDamageBonus;
        equipAttackPercent += item->stats.attackPercent;
        equipDefensePercent += item->stats.defensePercent;
        equipHpPercent += item->stats.hpPercent;
        equipMpPercent += item->stats.mpPercent;
        equipStrPercent += item->stats.strengthPercent;
        equipAgiPercent += item->stats.agilityPercent;
        equipIntPercent += item->stats.intelligencePercent;
        equipVitPercent += item->stats.vitalityPercent;
    };

    auto applyFullStats = [&](const std::shared_ptr<Item>& item) {
        if (!item) return;
        applyItemStats(item);
        for (const auto& stone : item->socketedStones) {
            if (stone) applyItemStats(stone);
        }
    };

    for (int i = 0; i < 12; ++i) {
        applyFullStats(mEquipment[i]);
    }

    // 3. Apply attribute effects
    int totalStr = std::max(0, mStrength + static_cast<int>(getStatModifier(Stat::STR)));
    int totalAgi = std::max(0, mAgility + static_cast<int>(getStatModifier(Stat::DEX)));
    int totalInt = std::max(0, mIntelligence + static_cast<int>(getStatModifier(Stat::INT)));
    int totalVit = std::max(0, mVitality + static_cast<int>(getStatModifier(Stat::VIT)));

    if (equipStrPercent > 0.001f) totalStr = static_cast<int>(totalStr * (1.0f + equipStrPercent / 100.0f));
    if (equipAgiPercent > 0.001f) totalAgi = static_cast<int>(totalAgi * (1.0f + equipAgiPercent / 100.0f));
    if (equipIntPercent > 0.001f) totalInt = static_cast<int>(totalInt * (1.0f + equipIntPercent / 100.0f));
    if (equipVitPercent > 0.001f) totalVit = static_cast<int>(totalVit * (1.0f + equipVitPercent / 100.0f));

    mStrength = totalStr;
    mAgility = totalAgi;
    mIntelligence = totalInt;
    mVitality = totalVit;

    mMaxHp = std::max(1, mMaxHp + totalVit * cfg::Player::HP_PER_VIT + equipHpBonus + mDirectMaxHp + static_cast<int>(getStatModifier(Stat::MAX_HP)));
    if (equipHpPercent > 0.001f) mMaxHp = static_cast<int>(mMaxHp * (1.0f + equipHpPercent / 100.0f));

    mDefense = std::max(0, mDefense + totalVit * cfg::Player::DEF_PER_VIT + mDirectDefense + static_cast<int>(getStatModifier(Stat::DEFENSE)));
    if (equipDefensePercent > 0.001f) mDefense = static_cast<int>(mDefense * (1.0f + equipDefensePercent / 100.0f));

    mMaxMp = std::max(0, mMaxMp + totalInt * cfg::Player::MP_PER_INT + equipMpBonus + mDirectMaxMp + static_cast<int>(getStatModifier(Stat::MAX_MP)));
    if (equipMpPercent > 0.001f) mMaxMp = static_cast<int>(mMaxMp * (1.0f + equipMpPercent / 100.0f));

    mAttack = std::max(0, mAttack + totalStr * cfg::Player::ATK_PER_STR + equipAttack + mDirectAttack + static_cast<int>(getStatModifier(Stat::ATTACK)));
    if (equipAttackPercent > 0.001f) mAttack = static_cast<int>(mAttack * (1.0f + equipAttackPercent / 100.0f));
    mAtkSpeed = std::max(0.05f, mAtkSpeed + equipAtkSpeed + (static_cast<float>(totalAgi) / 10.f) * cfg::Player::ATK_SPEED_PER_AGI + getStatModifier(Stat::ATK_SPEED));
    mCritChance = std::clamp(mCritChance + (static_cast<float>(totalAgi) / 10.f) * 1.f + getStatModifier(Stat::CRIT_CHANCE), 0.f, 100.f);
    mCritDamage = std::max(0.f, mCritDamage + getStatModifier(Stat::CRIT_DMG));
    mCooldownReductionPercent = std::clamp(mCooldownReductionPercent + getStatModifier(Stat::COOLDOWN_REDUCTION), 0.f, 100.f);

    float finalSpeed = std::max(0.f, mBlueprint.speed + equipSpeedBonus + getStatModifier(Stat::MOVE_SPEED));
    setSpeed(finalSpeed);

    // Scale or clamp Current HP/MP
    if (wasFullHp && isAlive()) {
        mCurrentHp = mMaxHp;
    } else if (isAlive() && oldMaxHp > 0) {
        float ratio = (float)mCurrentHp / (float)oldMaxHp;
        mCurrentHp = std::max(1, static_cast<int>(std::round(ratio * mMaxHp)));
        if (mCurrentHp > mMaxHp) mCurrentHp = mMaxHp;
    } else {
        if (mCurrentHp > mMaxHp) mCurrentHp = mMaxHp;
    }

    if (wasFullMp && isAlive()) {
        mCurrentMp = mMaxMp;
    } else if (isAlive() && oldMaxMp > 0) {
        float ratio = (float)mCurrentMp / (float)oldMaxMp;
        mCurrentMp = static_cast<int>(std::round(ratio * mMaxMp));
        if (mCurrentMp > mMaxMp) mCurrentMp = mMaxMp;
    } else {
        if (mCurrentMp > mMaxMp) mCurrentMp = mMaxMp;
    }
}

bool Mob::debugAddStat(const std::string& statL, float amount, bool isFixed) {
    int amountI = static_cast<int>(amount);
    bool recognized = true;

    #define SET_OR_ADD(var, val) if (isFixed) var = (val); else var += (val)

    if (statL == "str") SET_OR_ADD(mBlueprint.strength, amountI);
    else if (statL == "dex" || statL == "agi") SET_OR_ADD(mBlueprint.agility, amountI);
    else if (statL == "int") SET_OR_ADD(mBlueprint.intelligence, amountI);
    else if (statL == "vit") SET_OR_ADD(mBlueprint.vitality, amountI);
    else if (statL == "maxhp") SET_OR_ADD(mDirectMaxHp, amountI);
    else if (statL == "maxmp") SET_OR_ADD(mDirectMaxMp, amountI);
    else if (statL == "attack" || statL == "atk") SET_OR_ADD(mDirectAttack, amountI);
    else if (statL == "defense" || statL == "def") SET_OR_ADD(mDirectDefense, amountI);
    else if (statL == "peso" || statL == "weight") mBlueprint.weightKg = isFixed ? amount : mBlueprint.weightKg + amount;
    else if (statL == "speed") SET_OR_ADD(mBlueprint.speed, amount);
    else if (statL == "atkspeed") SET_OR_ADD(mBlueprint.atkSpeed, amount);
    else if (statL == "accuracy") SET_OR_ADD(mBlueprint.accuracy, amountI);
    else if (statL == "evasion") SET_OR_ADD(mBlueprint.evasion, amountI);
    else if (statL == "malice" || statL == "malicia") SET_OR_ADD(mMalice, amount);
    else if (statL == "armorpenpercent") SET_OR_ADD(mBlueprint.armorPenetrationPercent, amount);
    else if (statL == "armorpenflat") SET_OR_ADD(mBlueprint.armorPenetration, amountI);
    else if (statL == "physicaldmgbonus") SET_OR_ADD(mBlueprint.physicalDamageBonus, amount);
    else if (statL == "critchance") SET_OR_ADD(mBlueprint.critChance, amount);
    else if (statL == "critdamage") SET_OR_ADD(mBlueprint.critDamage, amount);
    else if (statL == "lifestealpercent") SET_OR_ADD(mBlueprint.lifestealPercent, amount);
    else if (statL == "cooldownreductionpercent" || statL == "cdr") SET_OR_ADD(mBlueprint.cooldownReductionPercent, amount);
    else if (statL == "doublestrikechance" || statL == "doublechance") SET_OR_ADD(mBlueprint.doubleStrikeChance, amount);
    else if (statL == "triplestrikechance" || statL == "triplechance") SET_OR_ADD(mBlueprint.tripleStrikeChance, amount);
    else if (statL == "enemymaxhpdamagepercent") SET_OR_ADD(mBlueprint.enemyMaxHpDamagePercent, amount);
    else if (statL == "manastealpercent") SET_OR_ADD(mBlueprint.manaStealPercent, amount);
    else if (statL == "truedamagepercent") SET_OR_ADD(mBlueprint.trueDamagePercent, amountI);
    else if (statL == "aoeradius") SET_OR_ADD(mBlueprint.aoeRadius, amount);
    else if (statL == "aoedamagepercent") SET_OR_ADD(mBlueprint.aoeDamagePercent, amount);
    else if (statL == "blockchance") SET_OR_ADD(mBlueprint.blockChance, amount);
    else if (statL == "blockvaluepercent") SET_OR_ADD(mBlueprint.blockValuePercent, amount);
    else if (statL == "thornspercent") SET_OR_ADD(mBlueprint.thornsPercent, amount);
    else if (statL == "tenacitypercent") SET_OR_ADD(mBlueprint.tenacityPercent, amount);
    else if (statL == "damagereductionpercent") SET_OR_ADD(mBlueprint.damageReductionPercent, amount);
    else if (statL == "critavoidancepercent") SET_OR_ADD(mBlueprint.critAvoidancePercent, amount);
    else if (statL == "antiarmorpenpercent") SET_OR_ADD(mBlueprint.antiArmorPenPercent, amount);
    else if (statL == "antiarmorpenflat") SET_OR_ADD(mBlueprint.antiArmorPenFlat, amountI);
    else if (statL == "hpregenpercent") SET_OR_ADD(mBlueprint.hpRegenPercent, amount);
    else if (statL == "mpregenpercent") SET_OR_ADD(mBlueprint.mpRegenPercent, amount);
    else if (statL == "xpbonuspercent") SET_OR_ADD(mBlueprint.xpBonusPercent, amount);
    else if (statL == "executedamagepercent") SET_OR_ADD(mBlueprint.executeDamagePercent, amountI);
    else if (statL == "executethresholdpercent") SET_OR_ADD(mBlueprint.executeHealthThresholdPercent, amountI);
    else if (statL == "bleeddurationflat") SET_OR_ADD(mBlueprint.bleedDurationFlat, amount);
    else if (statL == "bleeddurationpercent") SET_OR_ADD(mBlueprint.bleedDurationPercent, amount);
    else if (statL == "bleedflat") SET_OR_ADD(mBlueprint.bleedFlat, amountI);
    else if (statL == "bleedpercent") SET_OR_ADD(mBlueprint.bleedPercent, amount);
    else if (statL == "stunchance") SET_OR_ADD(mBlueprint.stunChance, amount);
    else if (statL == "stunduration") SET_OR_ADD(mBlueprint.stunDuration, amount);
    else if (statL == "slowmovepercent") SET_OR_ADD(mBlueprint.slowMovePercent, amount);
    else if (statL == "slowmoveduration") SET_OR_ADD(mBlueprint.slowMoveDuration, amount);
    else if (statL == "slowattackpercent") SET_OR_ADD(mBlueprint.slowAttackPercent, amount);
    else if (statL == "slowattackduration") SET_OR_ADD(mBlueprint.slowAttackDuration, amount);
    else {
        recognized = false;
    }

    #undef SET_OR_ADD

    if (recognized) {
        recalculateStats();
        notifyStatsChanged();
    }
    return recognized;
}
