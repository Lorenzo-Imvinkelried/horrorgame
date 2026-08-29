#pragma once
#include "../Skill.h"

class AttackBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
