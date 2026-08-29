#include "Charge.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/SoundSystem.h" // [NEW]
#include "utils/CombatCalculator.h"
#include <iostream>
#include <algorithm>

void Charge::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    // Intentionally empty: Embestida does not play swing/strike animation on cast
}

void Charge::onQueue(Entity* caster, ParticleSystem* particles) {
    if (!caster) return;
    caster->setCharging(true);
    std::cout << "[SKILL] Embestida iniciada. Corriendo a velocidad extrema.\n";
}

void Charge::onRemoveEffects(Entity* caster, ParticleSystem* particles) {
    if (!caster) return;
    caster->setCharging(false);
}

void Charge::updateChargeVisuals(Entity* caster, ParticleSystem* particles) {
    if (!caster || !particles) return;
    
    // Rastro dorado continuo durante la embestida
    sf::Vector2f pos = caster->getVisualPoint("hand_right");
    particles->emitOwned("GOLD_CHARGE_EMIT", pos, caster, 0.f, 4);
}

void Charge::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target) return;

    // Frenar la embestida al impactar
    onRemoveEffects(caster, particles);

    // Girar al caster hacia el objetivo
    float dx = target->getPosition().x - caster->getPosition().x;
    if (std::abs(dx) > 1.f) {
        if (auto* player = dynamic_cast<Player*>(caster)) {
            player->setFacingDir((dx > 0) ? 1 : -1);
        }
    }

    // Mini-lunge / Desplazamiento de impacto de unos pocos píxeles hacia el mob
    sf::Vector2f pPos = caster->getPosition();
    sf::Vector2f tPos = target->getPosition();
    sf::Vector2f diff = tPos - pPos;
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (dist > 0.1f) {
        sf::Vector2f dir = diff / dist;
        float slideDist = std::min(dist * 0.5f, 15.f);
        caster->setPosition(pPos + dir * slideDist);
    }

    // 1. Calcular daño (Daño Base + Daño de la Habilidad)
    float inputRaw = (float)caster->getAttack() + (float)this->getEffectiveDamageFlat();
    auto res = CombatCalculator::calculateSkillDamage(caster, target, inputRaw, true);

    // 2. Aplicar daño
    target->takeDamage(res.totalDamage, caster, res.isCrit);

    if (!res.isBlocked && res.totalDamage > 0 && caster) {
        target->triggerHitEffect(caster->getPosition());
    }

    if (auto* ss = SoundSystem::getInstance()) {
        if (res.isBlocked) {
            ss->playSound("assets/sounds/block.wav", 100.f);
        } else {
            ss->playSound("assets/sounds/player/charge.wav", 100.f);
        }
    }

    // 3. Aplicar Stun con 100% de probabilidad (duración configurable, default 2s)
    float baseDur = (stunDuration > 0.f) ? stunDuration : 2.0f;
    float duration = getEffectiveStunDuration();
    if (duration <= 0.f) duration = baseDur;
    target->applyStatusEffect(StatusEffect::Stun, duration);
    feedback.onStun(target, duration);

    // 4. Mostrar feedback de combate e impacto visual
    feedback.onHit(target, res.totalDamage, res.isCrit, false, res.isBlocked);

    if (particles) {
        sf::FloatRect bounds = target->getHitImpactBounds();
        sf::Vector2f center(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f);
        // Usar POWER_STRIKE (destello dorado) en el impacto
        particles->emitOwned("POWER_STRIKE", center, target, 0.f, 50);
    }

    std::cout << "[SKILL] Embestida impactó a " << target->getName() << ". ¡Stun aplicado!\n";
}
