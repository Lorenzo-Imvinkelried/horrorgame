#pragma once
#include "core/skills/Skill.h"

class OracionCurativa : public Skill {
public:
    OracionCurativa();

    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void updateChargeVisuals(Entity* caster, ParticleSystem* particles) override;
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem = nullptr) override {}
};
