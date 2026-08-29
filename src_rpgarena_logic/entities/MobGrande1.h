#pragma once
#include "mob/Mob.h"

class MobGrande1 : public Mob {
public:
    using Mob::Mob; // Inherit constructors from Mob

protected:
    void updateAI(sf::Time dt) override;
};
