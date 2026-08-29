#pragma once
#include "../Skill.h"

class MoveSpeedBuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
