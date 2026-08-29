#include "ShieldSlam.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "entities/mob/Mob.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/FXSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "utils/CombatCalculator.h"
#include <algorithm>
#include <iostream>

ShieldSlam::ShieldSlam() {
    id = 31;
    name = "Golpe de Escudo";
    description = "Golpea con el escudo. Si el objetivo esta casteando, interrumpe el hechizo, lo aturde 1.5s e inflige dano critico de castigo.";
    iconPath = "assets/ui/skills/atlas_skills_18x18x10.png";
    atlasX = 1 * 18 + 1;
    atlasY = 1 * 18 + 1;
    cooldown = 5.0f;
    manaCost = 20;
    damageFlat = 50;
    range = 0; // Mismo rango que ataque basico cuerpo a cuerpo
    castTime = 0.0f;
    stunDuration = 1.5f;
    requiresShield = true;
    type = SkillType::Active;
    targetType = "ENEMY";
    defaultSlot = 3;
    chargesGranted = 1;
}

void ShieldSlam::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;

    // Disparar animacion de golpe frontal con la mano del escudo
    caster->startShieldAttackAnimation(target, getSpeedMultiplier());

    // Particulas sutiles en la mano del escudo al iniciar
    if (particles) {
        bool isRightHand = false;
        if (auto* player = dynamic_cast<Player*>(caster)) {
            auto mainWp = player->getWeapon(0);
            auto offWp = player->getWeapon(1);
            if (mainWp && mainWp->isShield() && (!offWp || !offWp->isShield())) {
                isRightHand = true;
            }
        }
        sf::Vector2f handPos = caster->getVisualPoint(isRightHand ? "hand_right" : "hand_left");
        particles->emitOwned("GOLD_CHARGE_EMIT", handPos, caster, 0.f, 5);
    }
}

void ShieldSlam::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target || !target->isAlive()) return;

    // TAP SYSTEM
    if (this->chargesGranted > 0 && caster) {
        if (auto* p = dynamic_cast<Player*>(caster)) {
            p->getTapSystem().addCharges(this->chargesGranted);
        }
    }

    // 1. Detectar si el objetivo estaba casteando / preparando una habilidad
    bool wasCasting = target->isCasting();
    if (!wasCasting) {
        if (auto* mob = dynamic_cast<Mob*>(target)) {
            wasCasting = mob->isCasting() || (mob->isAlive() && mob->getPendingSkill() != nullptr);
        }
    }

    // 2. Calculo de dano
    // Base Attack + Skill Flat Damage
    float baseRaw = (float)caster->getAttack() + (float)this->getEffectiveDamageFlat();
    
    // Si estaba casteando, bono del 50% por castigo / interrupcion
    if (wasCasting) {
        baseRaw *= 1.5f;
    }

    auto res = CombatCalculator::calculateSkillDamage(caster, target, baseRaw, true);

    // 3. Mecanica de Interrupcion y Stun
    if (wasCasting) {
        // Cancelar casteo y limpiar estados
        target->setCasting(false);
        if (auto* mob = dynamic_cast<Mob*>(target)) {
            mob->cancelPendingSkill();
        } else if (auto* player = dynamic_cast<Player*>(target)) {
            if (combatSystem) combatSystem->cancelPendingAttack();
        }

        // Aplicar Stun y Efectos Visuales (Estrellitas de Stun)
        float stunDur = (this->stunDuration > 0.f) ? this->getEffectiveStunDuration() : 1.5f;
        target->applyStatusEffect(StatusEffect::Stun, stunDur);
        feedback.onStun(target, stunDur);
        if (particles) {
            particles->emitStunStars(target, stunDur);
        }

        // Feedback de texto flotante
        if (FXSystem::getInstance()) {
            FXSystem::getInstance()->createFloatingText(
                target->getPosition(),
                "¡INTERRUMPIDO!",
                sf::Color(255, 220, 40),
                13,
                1.3f,
                -40.f
            );
        }

        std::cout << "[SKILL] Golpe de Escudo: ¡INTERRUPCION EXITOSA sobre " << target->getName() << "! Stun: " << stunDur << "s\n";
    }

    // 4. Aplicar dano
    target->takeDamage(res.totalDamage, caster, res.isCrit);

    if (!res.isBlocked && res.totalDamage > 0 && caster) {
        target->triggerHitEffect(caster->getPosition());
    }

    // 5. Sonidos
    if (auto* ss = SoundSystem::getInstance()) {
        if (res.isBlocked) {
            ss->playSound("assets/sounds/block.wav", 100.f);
        } else if (wasCasting) {
            // Sonido de golpe contundente pesado
            ss->playSound("assets/sounds/efecto_sonido_ia/shield1.wav", 100.f);
        } else {
            ss->playSound("assets/sounds/efecto_sonido_ia/shield2.wav", 90.f);
        }
    }

    // 6. Visuals & Particulas
    feedback.onHit(target, res.totalDamage, res.isCrit, false, res.isBlocked);

    if (particles) {
        sf::FloatRect bounds = target->getHitImpactBounds();
        sf::Vector2f center;
        center.x = bounds.position.x + bounds.size.x * 0.5f;
        center.y = bounds.position.y + bounds.size.y * 0.5f;

        if (wasCasting) {
            // Impacto dorado / chispas pesadas por stun
            particles->emitHitImpact(center, 25, sf::Color(255, 215, 0), target);
            particles->emitOwned("POWER_STRIKE", center, target, 0.f, 40);
        } else {
            particles->emitHitImpact(center, 12, sf::Color(200, 200, 220), target);
        }
    }
}
