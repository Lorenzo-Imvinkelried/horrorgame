#pragma once
#include "../Skill.h"

class AtkSpeedDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
