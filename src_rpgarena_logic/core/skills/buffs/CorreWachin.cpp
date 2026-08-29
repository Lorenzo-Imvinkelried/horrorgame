#include "CorreWachin.h"
#include "entities/Entity.h"
#include <iostream>
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"

void CorreWachin::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    // Intentionally empty or just cast start visuals
}

void CorreWachin::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;

    std::cout << "[CorreWachin] Activando velocidad de movimiento!\n";

    if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound("assets/sounds/player/charge.wav", 100.f);
    }

    // 1. Visuals: Emit speed trail/particles around caster (cyan/green/yellow)
    if (particles) {
        particles->emitHitImpact(caster->getPosition(), 25, sf::Color::Cyan, caster);
    }

    // 2. Logic (Buffs)
    bool hasBuffs = false;
    for (const auto& eff : effects) {
        if (eff.type == EffectType::BUFF_STAT) {
            caster->applyBuff(eff.statToBuff, eff.value, eff.duration);
            hasBuffs = true;
        }
    }

    // Fallback if JSON was empty (Safety)
    if (!hasBuffs) {
        // Defaults: +150 movement speed for 5 seconds
        caster->applyBuff(Stat::MOVE_SPEED, 150.0f, 5.0f);
    }
}
