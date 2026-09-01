#pragma once
#include <cstdlib>
#include <algorithm>

struct AttackDamageResult {
    int Damage = 0;
    bool IsCrit = false;
    bool IsHit = true;
};

class CombatCalculator {
public:
    static AttackDamageResult CalculatePlayerAttack(int rawAttack, float critChance, float critMult, int targetDefense, int targetEvasion) {
        AttackDamageResult result;

        // 1. Guaranteed hit on physical melee contact (No phantom misses)
        result.IsHit = true;

        // 2. Damage Variance (±10%)
        float variance = 0.90f + (rand() % 21) * 0.01f;
        float baseDmg = rawAttack * variance;

        // 3. Defense Mitigation
        float mitigated = std::max(1.0f, baseDmg - targetDefense * 0.75f);

        // 4. Critical Strike Check
        float critRoll = (float)(rand() % 1000) * 0.1f;
        if (critRoll <= critChance) {
            result.IsCrit = true;
            mitigated *= critMult;
        }

        result.Damage = std::max(1, (int)mitigated);
        return result;
    }
};
