#include "Whirlwind.h"
#include "entities/Entity.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/SoundSystem.h"
#include <iostream>

void Whirlwind::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    caster->startAttackAnimation(nullptr);
}

void Whirlwind::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;

    // 1. Calculate center around the caster's feet.
    // NOTE: You can adjust the offset here. Increasing center.y (e.g. + 10.0f instead of - 5.0f)
    // will shift both the visual particle circle and the damage area lower on the screen.
    sf::FloatRect bounds = caster->getGlobalBounds();
    sf::Vector2f center;
    center.x = bounds.position.x + bounds.size.x * 0.5f;
    center.y = (bounds.position.y + bounds.size.y) + 16.0f; // Shift this offset to align lower or higher

    // 2. Play Sound Effect
    if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound("assets/sounds/player/torbellino.wav", 80.f);
    }

    // 3. Apply damage wave (this internally calls applyAoEAt -> onAoE, triggering both ROCK_BURST and ROCK_DEBRIS)
    int baseDamage = caster->getAttack() + this->getEffectiveDamageFlat();
    if (auto* cs = CombatSystem::getInstance()) {
        cs->applyAoEAt(caster, center, static_cast<float>(this->range), baseDamage);
    }

    std::cout << "[SKILL] Whirlwind (Torbellino) executed by " << caster->getName() << "\n";
}
