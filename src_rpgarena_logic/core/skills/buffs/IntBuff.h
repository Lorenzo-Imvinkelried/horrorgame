#pragma once
#include "../Skill.h"

class IntBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
