#pragma once
#include "../Skill.h"

class BasicAttack : public Skill {
public:
    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem = nullptr) override;
    void onQueue(Entity* caster, ParticleSystem* particles) override;
    void onRemoveEffects(Entity* caster, ParticleSystem* particles) override;
    void updateChargeVisuals(Entity* caster, ParticleSystem* particles) override;
};
