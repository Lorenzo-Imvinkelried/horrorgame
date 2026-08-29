#pragma once
#include "../Skill.h"

class PowerStrike : public Skill {
public:
    // Override methods
    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem = nullptr) override;
    void onQueue(Entity* caster, ParticleSystem* particles) override;
    void onRemoveEffects(Entity* caster, ParticleSystem* particles) override;
    void updateChargeVisuals(Entity* caster, ParticleSystem* particles) override; // [NEW]
    float getSpeedMultiplier() const override { return 1.70f; }
};
