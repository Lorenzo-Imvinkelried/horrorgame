#pragma once
#include "../Skill.h"

class StrDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
