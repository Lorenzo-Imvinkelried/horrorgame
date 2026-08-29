#pragma once
#include "../Skill.h"

class CritChanceDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
