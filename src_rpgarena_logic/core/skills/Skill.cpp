#include "Skill.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/SkillUpgradeSystem.h"
#include "utils/CombatCalculator.h"

int Skill::getEffectiveDamageFlat() const {
    return SkillUpgradeSystem::getInstance().getScaledDamageFlat(this->id, this->damageFlat);
}

float Skill::getEffectiveStunDuration() const {
    return SkillUpgradeSystem::getInstance().getScaledStunDuration(this->id, this->stunDuration);
}

float Skill::getEffectiveBuffDuration() const {
    return SkillUpgradeSystem::getInstance().getScaledBuffDuration(this->id, this->buffDuration);
}

float Skill::getEffectiveValue(float baseVal) const {
    return SkillUpgradeSystem::getInstance().getScaledEffectValue(this->id, "", baseVal);
}

float Skill::getEffectiveValue(Stat stat, float baseVal) const {
    return SkillUpgradeSystem::getInstance().getScaledEffectValue(this->id, stat, baseVal);
}

bool Skill::canCast(const Entity* caster, std::string* outReason) const {
    if (requiresShield) {
        if (!caster || !caster->hasShieldEquipped()) {
            if (outReason) *outReason = "¡Requiere un escudo equipado!";
            return false;
        }
    }
    return true;
}

void Skill::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster || !target) return;
    
    // Default Behavior: Just Animation
    caster->startAttackAnimation(target);
    
    // Generic Hand Particles from JSON could go here later
}

void Skill::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target) return;

    // Self-targeted skills (such as heals or buffs) do not deal offensive hit damage to the caster
    if (this->targetType == "SELF" || (target == caster && this->targetType != "ENEMY")) {
        return;
    }

    // [COMBAT ENGAGE] If this is an offensive/debuff skill targeting an enemy, trigger aggro
    if (this->targetType == "ENEMY") {
        target->onAggroedBy(caster);
        if (auto* p = dynamic_cast<Player*>(caster)) {
            p->addToAggro(target);
        }
    }

    // [TAP SYSTEM] Grant charges to caster if configured
    if (this->chargesGranted > 0 && caster) {
        if (auto* p = dynamic_cast<Player*>(caster)) {
            p->getTapSystem().addCharges(this->chargesGranted);
        }
    }

    // Default Behavior: Process "effects" list from JSON
    // Start with Generic Damage Calculation using damageFlat
    // usually: Base Atk + Flat
    
    // Logic mostly for "Active" damage skills
    int effDmg = getEffectiveDamageFlat();
    if (effDmg > 0 || this->effects.empty()) {
         float inputRaw = (float)caster->getAttack() + (float)effDmg;
         auto res = CombatCalculator::calculateSkillDamage(caster, target, inputRaw, true);
         
         target->takeDamage(res.totalDamage, caster, res.isCrit);
         if (!res.isBlocked && res.totalDamage > 0 && caster) {
             target->triggerHitEffect(caster->getPosition());
         }
         feedback.onHit(target, res.totalDamage, res.isCrit, false, res.isBlocked, true, 0.f, 1.0f, caster);
    }

    // Process Effects List
    for (const auto& eff : this->effects) {
        float scaledVal = getEffectiveValue(eff.statToBuff, eff.value);
        if (eff.type == EffectType::DAMAGE) {
             // Secondary DMG effect?
             auto mpg = CombatCalculator::calculateSkillDamage(caster, target, scaledVal, true);
             target->takeDamage(mpg.totalDamage, caster);
             if (mpg.totalDamage > 0 && caster) {
                 target->triggerHitEffect(caster->getPosition());
             }
             feedback.onHit(target, mpg.totalDamage, false, false, false, true, 0.f, 1.0f, caster);
        }
        else if (eff.type == EffectType::BUFF_STAT) {
             Entity* realTarget = (this->targetType == "SELF") ? caster : target;
             if (realTarget) {
                 float dur = (eff.duration > 0.f) ? eff.duration : getEffectiveBuffDuration();
                 realTarget->applyBuff(eff.statToBuff, scaledVal, dur, this->id, this->statusEffectId);
             }
        }
        // ... Handle others
    }
}
