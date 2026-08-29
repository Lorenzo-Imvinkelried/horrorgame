#include "AtkSpeedDebuff.h"
#include "entities/Entity.h"

void AtkSpeedDebuff::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster || !target) return;
    for (const auto& eff : effects) {
        if (eff.type == EffectType::BUFF_STAT) {
            target->applyBuff(eff.statToBuff, eff.value, eff.duration, this->id, this->statusEffectId);
        }
    }
}
