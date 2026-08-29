#pragma once
#include "../Skill.h"

class MaxHpBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
