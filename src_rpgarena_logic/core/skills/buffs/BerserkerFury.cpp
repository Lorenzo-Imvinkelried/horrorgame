#include "BerserkerFury.h"
#include "entities/Entity.h"
#include <iostream>
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h" // [NEW]

void BerserkerFury::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    // Intentionally empty or just SFX/Particle for shout start
}

void BerserkerFury::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;

    std::cout << "[BerserkerFury] Unleashing rage!\n";

    if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound("assets/sounds/player/berserker_fury.wav", 100.f);
    }

    // 1. Visuals
    if (particles) {
        particles->emitBerserkerFury(caster);
    }

    // 2. Logic (Buffs)
    // We apply the effects defined in JSON (stored in base 'effects' vector)
    // This allows data-driven tuning while satisfying "New Class" structure.
    bool hasBuffs = false;
    for (const auto& eff : effects) {
        if (eff.type == EffectType::BUFF_STAT) {
            float effVal = getEffectiveValue(eff.statToBuff, eff.value);
            caster->applyBuff(eff.statToBuff, effVal, eff.duration, this->id, this->statusEffectId);
            hasBuffs = true;
        }
    }

    // Fallback if JSON was empty (Safety)
    if (!hasBuffs) {
        // Defaults: +50% AtkSpeed, +20 Dmg, -20 Def for 5s
        caster->applyBuff(Stat::ATK_SPEED, 0.5f, 5.0f, this->id, this->statusEffectId);
        caster->applyBuff(Stat::ATTACK, 20.0f, 5.0f, this->id, this->statusEffectId);
        caster->applyBuff(Stat::DEFENSE, -20.0f, 5.0f, this->id, this->statusEffectId);
    }
}
