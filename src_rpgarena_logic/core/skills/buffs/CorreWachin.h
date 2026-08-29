#pragma once
#include "../Skill.h"

class CorreWachin : public Skill {
public:
    CorreWachin() {
        id = 6;
        name = "Corre Wachin";
    }

    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
};
