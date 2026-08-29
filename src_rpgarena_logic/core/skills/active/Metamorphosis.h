#pragma once
#include "../Skill.h"

class Metamorphosis : public Skill {
public:
    Metamorphosis() {
        id = 3;
        name = "Metamorphosis";
    }

    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem = nullptr) override;
};
