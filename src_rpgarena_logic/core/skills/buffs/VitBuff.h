#pragma once
#include "../Skill.h"

class VitBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
