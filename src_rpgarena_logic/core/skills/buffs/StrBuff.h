#pragma once
#include "../Skill.h"

class StrBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
