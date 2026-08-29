#include "Metamorphosis.h"
#include "entities/player/Player.h"
#include "core/managers/EntityManager.h"
#include "core/systems/ParticleSystem.h"
#include <iostream>

void Metamorphosis::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    std::cout << "[Metamorphosis] Iniciando casteo de Metamorphosis...\n";
    if (particles && caster) {
        // Emit purple/magenta charging particles around caster
        particles->emitPowerStrike(caster->getPosition(), 30);
    }
}

void Metamorphosis::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target) return;

    auto* player = dynamic_cast<Player*>(caster);
    auto* mob = dynamic_cast<Mob*>(target);

    if (player && mob && mob->isAlive()) {
        std::cout << "[Metamorphosis] Transformándose en: " << mob->getBlueprint().type << "\n";

        // Emitir explosión de partículas color púrpura/magenta
        if (particles) {
            particles->emitHitImpact(caster->getPosition(), 45, sf::Color::Magenta, caster);
        }

        // Ejecutar la transformación
        player->morphInto(mob->getBlueprint().type, buffDuration, mob->getBlueprint());
    } else {
        std::cout << "[Metamorphosis] Target inválido o no es un Mob vivo.\n";
    }
}
