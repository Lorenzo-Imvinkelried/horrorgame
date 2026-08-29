#pragma once
#include "../Skill.h"

class BerserkerFury : public Skill {
public:
    BerserkerFury() {
        id = 2;
        name = "Berserker Fury";
    }

    // Override generic cast to implement specific logic
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
