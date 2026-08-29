#include "PowerStrike.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"

#include "core/systems/ParticleSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/SoundSystem.h" // [NEW]
#include "utils/CombatCalculator.h"
#include <iostream>

void PowerStrike::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    
    // 1. Play Animation faster based on skill speed multiplier (LoL AA-Reset feel)
    caster->startAttackAnimation(target, getSpeedMultiplier());
    
    // 2. [NEW] Hand Particles (Gold Charge)
    if (particles) {
        // We assume Hand is near Center/Chest for now, or use a specific Offset
        sf::Vector2f handPos = caster->getPosition();
        handPos.y -= 30.f; // Approx chest/hand height
        // onCast only handles Animation now. Visuals started at onQueue.
    }
}


void PowerStrike::onQueue(Entity* caster, ParticleSystem* particles) {
    if (!caster || !particles) return;
    
    // Default: Right Hand (Standard for Mobs/Unarmed)
    bool useRight = true;
    bool useLeft = false;

    if (auto* player = dynamic_cast<Player*>(caster)) {
        bool hasMain = (player->getWeapon(0) != nullptr);
        bool hasOff  = (player->getWeapon(1) != nullptr);

        if (hasMain && hasOff) {
            useRight = true; useLeft = true;
        } else if (hasOff && !hasMain) {
            useRight = false; useLeft = true;
        } else if (hasMain) {
            useRight = true; useLeft = false;
        } else {
            // Unarmed -> "Screen Right" consistency
            if (player->getFacingDir() == 1) {
                useRight = true; useLeft = false;
            } else {
                useRight = false; useLeft = true;
            }
        }
    }

    if (useRight) {
        sf::Vector2f pos = caster->getVisualPoint("hand_right");
        particles->emit("GOLD_CHARGE_EMIT", pos); 
    }
    if (useLeft) {
         sf::Vector2f pos = caster->getVisualPoint("hand_left");
         particles->emit("GOLD_CHARGE_EMIT", pos); 
    }
}

void PowerStrike::onRemoveEffects(Entity* caster, ParticleSystem* particles) {
    // No explicit cleanup needed for continuous emission
    // (It stops when updateChargeVisuals stops being called)
}

void PowerStrike::updateChargeVisuals(Entity* caster, ParticleSystem* particles) {
    if (!caster || !particles) return;

    // Default: Right Hand (Standard for Mobs/Unarmed)
    bool useRight = true;
    bool useLeft = false;

    if (auto* player = dynamic_cast<Player*>(caster)) {
        bool hasMain = (player->getWeapon(0) != nullptr);
        bool hasOff  = (player->getWeapon(1) != nullptr);

        if (hasMain && hasOff) {
            useRight = true; useLeft = true;
        } else if (hasOff && !hasMain) {
            useRight = false; useLeft = true;
        } else if (hasMain) {
            useRight = true; useLeft = false;
        } else {
            // Unarmed -> "Screen Right" consistency
            if (player->getFacingDir() == 1) {
                useRight = true; useLeft = false;
            } else {
                useRight = false; useLeft = true;
            }
        }
    }

    if (useRight) {
        sf::Vector2f pos = caster->getVisualPoint("hand_right");
        particles->emitOwned("GOLD_CHARGE_EMIT", pos, caster);
    }
    if (useLeft) {
         sf::Vector2f pos = caster->getVisualPoint("hand_left");
         particles->emitOwned("GOLD_CHARGE_EMIT", pos, caster);
     }
}

void PowerStrike::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target) return;

    // [VISUALS] Stop Charge
    onRemoveEffects(caster, particles);

    // [TAP SYSTEM] Grant charges to caster if configured
    if (this->chargesGranted > 0 && caster) {
        if (auto* p = dynamic_cast<Player*>(caster)) {
            p->getTapSystem().addCharges(this->chargesGranted);
        }
    }

    // 1. Damage Calculation
    // Base Attack + Skill Flat Damage
    float inputRaw = (float)caster->getAttack() + (float)this->getEffectiveDamageFlat();
    
    // Use Calculator
    auto res = CombatCalculator::calculateSkillDamage(caster, target, inputRaw, true);

    // 2. Apply
    target->takeDamage(res.totalDamage, caster, res.isCrit);
    
    if (!res.isBlocked && res.totalDamage > 0 && caster) {
        target->triggerHitEffect(caster->getPosition());
    }
    
    if (auto* ss = SoundSystem::getInstance()) {
        if (res.isBlocked) {
            ss->playSound("assets/sounds/block.wav", 100.f);
        } else if (res.isCrit && caster && caster->hasTwoHandedWeaponEquipped()) {
            ss->playSound("assets/sounds/crit_2h.wav", 100.f);
        } else if (caster && caster->hasTwoHandedWeaponEquipped()) {
            ss->playSound("assets/sounds/efecto_sonido_ia/2h_power_strike.wav", 100.f);
        } else {
            ss->playSound("assets/sounds/player/power_strike.wav", 100.f);
        }
    }
         
    // 3. Visuals
    // Pass isCrit from result
    feedback.onHit(target, res.totalDamage, res.isCrit, false, res.isBlocked);
    
    // Explicit Particle Call for Power Strike
    if (particles) {
        // [VISUAL FIX] Use Bounds Center instead of Feet (getPosition)
        sf::FloatRect bounds = target->getHitImpactBounds();
        sf::Vector2f center;
        center.x = bounds.position.x + bounds.size.x * 0.5f;
        center.y = bounds.position.y + bounds.size.y * 0.5f;

        // [Z-SORTING] Use emitOwned to sort with target
        // [VISUAL FIX] Reduced count to 60 (from 100) to fix "Tirones" (Lag) and "Too Many Particles"
        particles->emitOwned("POWER_STRIKE", center, target, 0.f, 60);
    }

    if (res.isCrit) std::cout << "[SKILL] Power Strike CRITICAL!\n";
    else std::cout << "[SKILL] Power Strike executed.\n";
    
    // 4. Aggro handled naturally by takeDamage events or virtuals
    // if (auto player = dynamic_cast<Player*>(caster)) ...
}
