#pragma once
#include "../Skill.h"

class CritChanceBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
