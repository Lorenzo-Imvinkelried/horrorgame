#pragma once
#include "../Skill.h"

class AtkSpeedBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
