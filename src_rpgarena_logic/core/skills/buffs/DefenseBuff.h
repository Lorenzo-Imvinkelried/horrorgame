#pragma once
#include "../Skill.h"

class DefenseBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
