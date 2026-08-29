#pragma once
#include "../Skill.h"

class MoveSpeedDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
