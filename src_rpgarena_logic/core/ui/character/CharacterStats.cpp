#include "CharacterPanel.h"
#include <iomanip>
#include <sstream>

void CharacterPanel::updateTexts() {
    if (!mEntity) return;

    // --- BLOQUE PRINCIPAL (Nivel) ---
    mLvlStr = std::to_string(mEntity->getLevel());

    // --- STATS BASE ---
    mStrStr = std::to_string(mEntity->getStrength());
    mAgiStr = std::to_string(mEntity->getAgility());
    mIntStr = std::to_string(mEntity->getIntelligence());
    mVitStr = std::to_string(mEntity->getVitality());
    mHpStr  = std::to_string(mEntity->getCurrentHp()) + "/" + std::to_string(mEntity->getMaxHp());
    mMpStr  = std::to_string(mEntity->getCurrentMp()) + "/" + std::to_string(mEntity->getMaxMp());
    
    {
        std::stringstream ssWeight;
        ssWeight << std::fixed << std::setprecision(1) << mEntity->getWeightKg() << " kg";
        mWeightStr = ssWeight.str();
    }

    // --- COMBATE OFENSIVO ---
    int attackVal = mEntity->getAttack();
    mAtkStr = std::to_string(attackVal);
    mDefStr = std::to_string(mEntity->getDefense());
    mRangeStr = std::to_string(static_cast<int>(mEntity->getAttackRange()));

    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << mEntity->getPhysicalDamageBonus() << "%";
    mAtkBonusStr = ss.str();

    ss.str("");
    float effAtkSpeed = mEntity->getAtkSpeed() * mEntity->getAttackSpeedMultiplier();
    ss << std::fixed << std::setprecision(1) << effAtkSpeed;
    mAtkSpdStr = ss.str();
    mAtkSpdColor = (mEntity->getAttackSpeedMultiplier() < 1.0f) ? sf::Color(100, 255, 255) : sf::Color::White;

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getCritChance() << "%";
    mCritStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getCritDamage() << "%";
    mCritDmgStr = ss.str();

    mPenStr = std::to_string(mEntity->getArmorPenetration());

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getArmorPenetrationPercent() << "%";
    mPenPctStr = ss.str();

    mAccStr = std::to_string(mEntity->getAccuracy());

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getLifestealPercent() << "%";
    mLifeStealStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getAoeRadius();
    mAoeRngStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getAoeDamagePercent() << "%";
    mAoeDmgStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getDoubleStrikeChance() << "%";
    mDblHitStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getTripleStrikeChance() << "%";
    mTriHitStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getEnemyMaxHpDamagePercent() << "%";
    mHpDmgStr = ss.str();

    ss.str("");
    float effMoveSpeed = mEntity->getMovementSpeed() * mEntity->getSpeedMultiplier();
    ss << std::fixed << std::setprecision(0) << effMoveSpeed;
    mMoveSpdStr = ss.str();
    mMoveSpdColor = (mEntity->getSpeedMultiplier() < 1.0f) ? sf::Color(100, 255, 255) : sf::Color::White;

    // Defense
    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getBlockChance() << "% (" << mEntity->getBlockValuePercent() << "%)";
    mBlockStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getThornsPercent() << "%";
    mThornsStr = ss.str();

    mEvasionStr = std::to_string(mEntity->getEvasion());

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getHpRegenPercent() << "%/s";
    mHpRegenStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getMpRegenPercent() << "%/s";
    mMpRegenStr = ss.str();

    // Execute
    ss.str("");
    ss << "+" << mEntity->getExecuteDamagePercent() << "%";
    mExecDmgStr = ss.str();

    ss.str("");
    ss << "< " << (int)(mEntity->getExecuteThresholdFactor() * 100) << "% HP";
    mExecThrStr = ss.str();

    ss.str("");
    ss << mEntity->getTrueDamagePercent() << "%";
    mTrueDmgStr = ss.str();

    // Bleed
    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getBleedDurationFlat() << "s";
    mBleedDurationFlatStr = ss.str();

    mBleedFlatDmgStr = std::to_string(mEntity->getBleedFlat());

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getBleedDurationPercent() << "s";
    mBleedDurationPctStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getBleedPercent() << "%";
    mBleedPctDmgStr = ss.str();

    // Debuffs
    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getStunChance() << "% (" << std::setprecision(1) << mEntity->getStunDuration() << "s)";
    mStunStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getSlowMovePercent() << "% (" << std::setprecision(1) << mEntity->getSlowMoveDuration() << "s)";
    mSlowMovStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getSlowAttackPercent() << "% (" << std::setprecision(1) << mEntity->getSlowAttackDuration() << "s)";
    mSlowAtkStr = ss.str();

    // Stats adicionales
    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getTenacityPercent() << "%";
    mTenacityStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getDamageReductionPercent() << "%";
    mDmgRedStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getCritAvoidancePercent() << "%";
    mCritAvoidStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getAntiArmorPenPercent() << "% (" << mEntity->getAntiArmorPenFlat() << ")";
    mAntiPenStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getManaStealPercent() << "%";
    mManaStealStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getXpBonusPercent() << "%";
    mXpBonusStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(1) << mEntity->getCooldownReductionPercent() << "%";
    mCdrStr = ss.str();

    ss.str("");
    ss << std::fixed << std::setprecision(0) << mEntity->getMalice();
    mMaliceStr = ss.str();
}
