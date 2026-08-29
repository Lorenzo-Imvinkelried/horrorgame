#pragma once
#include "../Skill.h"

class Whirlwind : public Skill {
public:
    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
