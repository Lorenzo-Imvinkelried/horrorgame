#pragma once
#include "../Skill.h"

class DefenseDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
