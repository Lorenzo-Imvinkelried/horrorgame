#pragma once
#include "../Skill.h"

class IntDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
