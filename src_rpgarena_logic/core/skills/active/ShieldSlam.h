#pragma once
#include "core/skills/Skill.h"

class ShieldSlam : public Skill {
public:
    ShieldSlam();
    virtual ~ShieldSlam() = default;

    virtual void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    virtual void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, class CombatSystem* combatSystem = nullptr) override;
    virtual float getSpeedMultiplier() const override { return 1.85f; }
};
