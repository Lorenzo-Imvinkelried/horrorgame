#pragma once
#include "../Skill.h"

class AttackDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
