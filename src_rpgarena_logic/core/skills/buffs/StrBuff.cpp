#include "StrBuff.h"
#include "entities/Entity.h"

void StrBuff::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    for (const auto& eff : effects) {
        if (eff.type == EffectType::BUFF_STAT) {
            float val = getEffectiveValue(eff.value);
            caster->applyBuff(eff.statToBuff, val, eff.duration, this->id, this->statusEffectId);
        }
    }
}
