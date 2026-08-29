#pragma once
#include "Config.h"
#include "Random.h"
#include <algorithm> // for std::clamp

class CombatCalculator {
public:
  struct DamageResult {
    int totalDamage = 0;
    int physicalDamage = 0;
    int trueDamage = 0;
    bool isCrit = false;
    bool isBlocked = false; // [NEW] Centralized Block Flag
  };

  // Nueva Fórmula "Profesional" de Acierto (Level-Scaled)
  static bool tryToHit(Entity *attacker, Entity *defender) {
    if (!attacker || !defender)
      return false;

    float accuracy = (float)attacker->getAccuracy();
    float evasion = (float)defender->getEvasion();
    int attLvl = attacker->getLevel();
    int defLvl = defender->getLevel();

    // 1. Base Accuracy Factor (Diminishing Returns)
    // Si Acc == Eva: 100 / (100 + 100*0.25) = 100/125 = 80% Hit Chance
    // Si Acc >> Eva: tiende a 100%
    // Si Eva >> Acc: tiende a 0%
    // Factor 0.25 hace que necesites 4x Evasion para igualar 1x Accuracy en
    // peso
    float k = 0.25f;
    float statChance = 0.f;

    if (accuracy > 0.f)
      statChance = accuracy / (accuracy + (evasion * k));
    else
      statChance = 0.f;

    // 2. Level Correction (Configurable)
    // Default 0.02 (2% per level). 0.002 = 0.2% per level.
    float levelDiff = (float)(attLvl - defLvl);
    float levelFactor = levelDiff * cfg::Combat::LEVEL_DIFF_HIT_PENALTY;

    // 3. Final Calculation
    float finalChance = statChance + levelFactor;

    // Base Floor/Ceiling (5% min, 100% max - si, permitimos 100% si stats son
    finalChance = std::clamp(finalChance, 0.05f, 1.0f);

    // Debug Log (Opcional, desactivar en release)
    // std::cout << "[HIT CALC] Acc:" << accuracy << " Eva:" << evasion
    //           << " LvlDiff:" << levelDiff << " Chance:" << finalChance*100 <<
    //           "%\n";

    return Random::Roll(finalChance * 100.f);
  }

  static DamageResult calculateDamage(Entity *attacker, Entity *defender,
                                      bool allowTrueDamage = true,
                                      float flatBonusDamage = 0.f) {
    DamageResult result;
    if (!attacker || !defender)
      return result;

    // =========================================================
    // 1. GENERATE RAW ATTACK
    // =========================================================
    float rawAttack = (float)attacker->getAttack() + flatBonusDamage;

    // Bonus %
    float damageMultiplier = attacker->getPhysicalDamageBonus() / 100.0f;
    float enhancedDamage = rawAttack * damageMultiplier;

    // [BONUS DAMAGE] Enemy Max HP %
    float hpPct = attacker->getEnemyMaxHpDamagePercent();
    if (hpPct > 0.0f) {
      enhancedDamage += (float)defender->getMaxHp() * (hpPct / 100.0f);
    }

    // [DAMAGE VARIANCE]
    int variancePercent = cfg::Combat::DAMAGE_VARIANCE_PERCENT;
    float randomOffset =
        static_cast<float>(Random::Int(-variancePercent, variancePercent));
    float varianceFactor = 1.0f + (randomOffset / 100.0f);
    enhancedDamage *= varianceFactor;

    // [EXECUTE DAMAGE]
    float thresholdFactor = attacker->getExecuteThresholdFactor();
    if (thresholdFactor > 0.0f) {
      float executionHP = (float)defender->getMaxHp() * thresholdFactor;
      if ((float)defender->getCurrentHp() < executionHP) {
        enhancedDamage *= attacker->getExecuteDamageMultiplier();
      }
    }

    // [CRIT GEN]
    bool isCrit = false;
    float critChance = attacker->getCritChance();
    float critAvoid = defender->getCritAvoidancePercent();
    float finalCritChance = std::max(0.f, critChance - critAvoid);
    if (Random::Roll(finalCritChance)) {
      float critMult = (attacker->getCritDamage() / 100.f);
      enhancedDamage *= critMult;
      isCrit = true;
    }

    // =========================================================
    // 2. MITIGATE
    // =========================================================
    // [FIX] Pass allowTrueDamage flag
    DamageResult res =
        calculateMitigatedDamage(attacker, defender, enhancedDamage,
                                 true /*isPhysical*/, allowTrueDamage);

    // Restore Crit Flag (helper doesn't know about generated crit, it just
    // mitigates value) Wait, helper doesn't re-roll crit, but we need to pass
    // IsCrit flag out?
    res.isCrit = isCrit;

    // Wait, calculateMitigatedDamage handles "Mitigation".
    // Crit multiplication Logic?
    // In my helper I added Crit Logic? No.
    // I removed Crit Logic from helper (it was in my previous copy paste
    // block). I put Crit Logic in "Generate Raw" phase here. So
    // `enhancedDamage` already includes Crit Multiplier.

    return res;
  }

  // [SKILL DAMAGE]
  static DamageResult calculateSkillDamage(Entity *attacker, Entity *defender,
                                           float rawDamage,
                                           bool isPhysical = true) {
    // 1. Generate Raw Output (Variance, Execute, Crit)
    float enhancedDamage = rawDamage;

    // [DAMAGE VARIANCE]
    int variancePercent = cfg::Combat::DAMAGE_VARIANCE_PERCENT;
    float randomOffset =
        static_cast<float>(Random::Int(-variancePercent, variancePercent));
    float varianceFactor = 1.0f + (randomOffset / 100.0f);
    enhancedDamage *= varianceFactor;

    // [EXECUTE DAMAGE]
    float thresholdFactor = attacker->getExecuteThresholdFactor();
    if (thresholdFactor > 0.0f) {
      float executionHP = (float)defender->getMaxHp() * thresholdFactor;
      if ((float)defender->getCurrentHp() < executionHP) {
        enhancedDamage *= attacker->getExecuteDamageMultiplier();
      }
    }

    bool isCrit = false;

    float critChance = attacker->getCritChance();
    float critAvoid = defender->getCritAvoidancePercent();
    float finalCritChance = std::max(0.f, critChance - critAvoid);
    if (Random::Roll(finalCritChance)) {
      float critMult = (attacker->getCritDamage() / 100.f);
      enhancedDamage *= critMult;
      isCrit = true;
    }

    // 2. Mitigate
    DamageResult res = calculateMitigatedDamage(attacker, defender,
                                                enhancedDamage, isPhysical);
    res.isCrit = isCrit;
    return res;
  }

  // [MITIGATION HELPER] - The "Centralized" logic for Defense/Armor/Resist
  static DamageResult calculateMitigatedDamage(Entity *attacker,
                                               Entity *defender, float totalRaw,
                                               bool isPhysical,
                                               bool allowTrueDamage = true) {
    DamageResult result;
    if (!defender)
      return result;

    float truePct = 0.0f;

    // If attacker exists, check for True Damage stats
    // [FIX] Check allowTrueDamage flag
    if (attacker && allowTrueDamage) {
      truePct = (float)attacker->getTrueDamagePercent() / 100.0f;
    }

    // Clamp pct
    if (truePct > 1.0f)
      truePct = 1.0f;
    if (truePct < 0.0f)
      truePct = 0.0f;

    // If magical/non-physical, bypass armor?
    if (!isPhysical) {
      result.physicalDamage = 0;
      result.trueDamage = (int)totalRaw;
      result.totalDamage = result.trueDamage;
      return result;
    }

    float rawTrue = totalRaw * truePct;
    float rawPhys = totalRaw * (1.0f - truePct);

    // --- BLOCK LOGIC [NEW] ---
    // Check block BEFORE armor.
    float blockChance = defender->getBlockChance();
    if (blockChance > 0.f && rawPhys > 0.001f) {
      if (Random::Roll(blockChance)) {
        result.isBlocked = true;
        float blockVal = defender->getBlockValuePercent();
        float multiplier = std::max(0.0f, 1.0f - (blockVal / 100.0f));
        rawPhys *= multiplier;
      }
    }

    // --- ARMOR CALC ---
    float targetTotalArmor = (float)defender->getDefense();

    float effectivePenPercent = 0.f;
    float effectivePenFlat = 0.f;

    if (attacker) {
      float attackerPenPercent = attacker->getArmorPenetrationPercent();
      float attackerPenFlat = (float)attacker->getArmorPenetration();

      float antiPenPercent = defender->getAntiArmorPenPercent();
      int antiPenFlat = defender->getAntiArmorPenFlat();

      effectivePenPercent = std::max(0.0f, attackerPenPercent - antiPenPercent);
      effectivePenFlat = std::max(0.0f, attackerPenFlat - (float)antiPenFlat);
    }

    float armorAfterPercent =
        targetTotalArmor * (1.0f - (effectivePenPercent / 100.0f));
    float effectiveDefense =
        std::max(0.0f, armorAfterPercent - effectivePenFlat);

    int targetLevel = defender->getLevel();
    float defenseConstant =
        (float)(targetLevel * cfg::Combat::DEFENSE_CONSTANT_LEVEL_SCALE) + cfg::Combat::DEFENSE_CONSTANT_BASE;
    float armorReduction =
        effectiveDefense / (effectiveDefense + defenseConstant);

    float reducedPhys = rawPhys * (1.0f - armorReduction);

    // --- LEVEL PENALTY ---
    if (attacker) {
      int levelDiff = attacker->getLevel() - targetLevel;
      // Use DAMAGE penalty
      float levelMultiplier =
          1.0f + (levelDiff * cfg::Combat::LEVEL_DIFF_DAMAGE_PENALTY);
      levelMultiplier =
          std::clamp(levelMultiplier, cfg::Combat::MIN_LEVEL_MULTIPLIER,
                     cfg::Combat::MAX_LEVEL_MULTIPLIER);

      reducedPhys *= levelMultiplier;
      rawTrue *= levelMultiplier;
    }

    // --- DAMAGE REDUCTION ---
    float dmgRed = defender->getDamageReductionPercent();
    if (dmgRed > 0.0f) {
      float multiplier = std::max(0.0f, 1.0f - (dmgRed / 100.0f));
      reducedPhys *= multiplier;
    }

    result.physicalDamage = std::max(0, static_cast<int>(reducedPhys));
    result.trueDamage = std::max(0, static_cast<int>(rawTrue));

    result.totalDamage = result.physicalDamage + result.trueDamage;
    if (result.totalDamage < 1 && totalRaw > 0) { // Keep 1 dmg min if raw > 0
      result.totalDamage = 1;
      if (result.physicalDamage == 0 && result.trueDamage == 0)
        result.physicalDamage = 1;
    }

    return result;
  }

  // [KNOCKBACK] Calculates force multiplier when an entity dies
  static float calculateDeathKnockbackMultiplier(Entity *killer,
                                                 Entity *victim) {
    if (!killer || !victim)
      return 1.0f;

    float killerWeight = std::max(1.f, killer->getWeightKg());
    float victimWeight = std::max(1.f, victim->getWeightKg());

    // Use total Attack (which includes STR and weapon damage) to represent
    // hitting power
    float killerStrength = std::max(1.f, (float)killer->getAttack());

    // --- Conservación de Cantidad de Movimiento (Física 1: p = m * v) ---

    // 1. Velocidad de impacto: Usamos la Velocidad de Ataque real, pero con una
    // curva de raíz cuadrada. Esto genera rendimientos decrecientes
    // (diminishing returns) para que a 10 atk/s no explote la física.
    float atkSpeed = std::max(0.1f, killer->getAtkSpeed() *
                                        killer->getAttackSpeedMultiplier());
    float hittingVelocity =
        std::sqrt(atkSpeed) * 15.0f * cfg::Combat::KNOCKBACK_STRENGTH_FACTOR;

    // 2. Masa del impacto: Un brazo/arma suele ser ~10% del peso corporal.
    // Alguien con mucha Fuerza logra rotar y poner más porcentaje de su peso
    // detrás del golpe.
    float bodyFraction = 0.10f + (killerStrength * 0.002f);
    bodyFraction = std::clamp(bodyFraction, 0.10f,
                              0.60f); // Máximo 60% de su peso en un golpe

    float hittingMass =
        killerWeight * bodyFraction * cfg::Combat::KNOCKBACK_WEIGHT_FACTOR;

    // 3. Cantidad de movimiento (Momento) inicial: p = m * v
    float initialMomentum = hittingMass * hittingVelocity;

    // 4. Colisión inelástica (el atacante transmite su impulso al cuerpo de la
    // víctima):
    float finalVelocity = initialMomentum / (hittingMass + victimWeight);

    // 5. Normalización y Atenuación:
    // Aplicamos la constante 1/a sugerida para hacer más dura la ganancia de
    // distancia.
    float attenuationConstant = 0.5f;
    float finalMult = (finalVelocity / 4.0f) * attenuationConstant;

    // Aplicamos otra leve curva logarítmica final para robustez extrema a
    // números absurdos
    finalMult = std::log10(1.0f + finalMult) * 2.5f;

    // Clamp para evitar bugs de motor (bajamos el límite máximo a 4.0 para
    // mayor control)
    return std::clamp(finalMult, 0.10f, 4.0f);
  }
};
