#pragma once
#include "../Skill.h"

class AgiDebuff : public Skill {
public:
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
