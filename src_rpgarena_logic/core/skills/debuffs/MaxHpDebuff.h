#pragma once
#include "../Skill.h"

class MaxHpDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
