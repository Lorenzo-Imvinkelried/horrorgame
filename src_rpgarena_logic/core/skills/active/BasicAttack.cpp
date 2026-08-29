#include "BasicAttack.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/combat/GhostSlash.h"
#include "core/engine/animation/Animation.h"
#include "core/systems/SoundSystem.h"
#include "utils/CombatCalculator.h"
#include "utils/Random.h"
#include <iostream>

void BasicAttack::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    caster->startAttackAnimation(target);
}

void BasicAttack::onQueue(Entity* caster, ParticleSystem* particles) {
    // Basic attack has no queuing charge visuals
}

void BasicAttack::onRemoveEffects(Entity* caster, ParticleSystem* particles) {
    // Basic attack has no queuing charge effects to remove
}

void BasicAttack::updateChargeVisuals(Entity* caster, ParticleSystem* particles) {
    // Basic attack has no continuous charge visuals
}

void BasicAttack::onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem) {
    if (!caster || !target) return;
    if (!target->isAlive() || target->isReturningToSpawn()) return;

    // Multi Strike Logic (if caster is Player or supports it)
    int numHits = 1;
    float tripleChance = caster->getTripleStrikeChance();
    float doubleChance = caster->getDoubleStrikeChance();

    if (Random::Roll(tripleChance)) {
        numHits = 3;
    } else if (Random::Roll(doubleChance)) {
        numHits = 2;
    }

    Player* playerCaster = dynamic_cast<Player*>(caster);
    if (numHits > 1 && playerCaster) {
        std::cout << "[COMBAT] MULTI-STRIKE ACTIVADO (SKILL BASIC ATTACK): " << numHits << " hits!\n";
    }

    float currentOffsetY = cfg::UI::DAMAGE_OFFSET_BASE;

    // Perform Base Strike (Hit #0)
    CombatCalculator::DamageResult result = CombatCalculator::calculateDamage(caster, target, true, 0.f);

    bool isPureTrue = (result.trueDamage > 0 && result.physicalDamage == 0);

    target->setLastHitDirect(true);
    target->takeDamage(result.totalDamage, caster, result.isCrit, isPureTrue);

    if (auto* ss = SoundSystem::getInstance()) {
        if (result.isBlocked) {
            ss->playSound("assets/sounds/block.wav", 100.f);
        } else if (result.totalDamage > 0) {
            target->triggerHitEffect(caster->getPosition());
            if (result.isCrit && caster && caster->hasTwoHandedWeaponEquipped()) {
                ss->playSound("assets/sounds/crit_2h.wav", 100.f);
            } else if (caster && caster->hasTwoHandedWeaponEquipped()) {
                ss->playSound("assets/sounds/efecto_sonido_ia/2h_basic_attack.wav", 100.f);
            } else if (result.isCrit && caster && !caster->hasWeaponEquipped()) {
                ss->playSound("assets/sounds/player/crit_hand.wav", 100.f);
            } else {
                ss->playSound("assets/sounds/barehand_hit.wav", 100.f);
                ss->playSound("assets/sounds/barehand_hit.wav", 100.f);
            }
        }
    }

    // Visual Feedback (Red/Silver/Blocked text)
    if (result.physicalDamage > 0 && result.trueDamage > 0) {
        feedback.onHit(target, result.physicalDamage, result.isCrit, false, result.isBlocked, true, currentOffsetY, 1.0f, caster);
        feedback.onHit(target, result.trueDamage, result.isCrit, true, false, false, currentOffsetY + 10.f, 1.0f, caster);
    } else if (result.trueDamage > 0) {
        feedback.onHit(target, result.trueDamage, result.isCrit, true, result.isBlocked, true, currentOffsetY, 1.0f, caster);
    } else {
        feedback.onHit(target, result.physicalDamage, result.isCrit, false, result.isBlocked, true, currentOffsetY, 1.0f, caster);
    }

    // Multi-strike ghost slashes
    if (numHits > 1 && playerCaster && combatSystem) {
        GhostSlashVisualData vData;
        const Animation* playerAnim = playerCaster->getSkin().getAnimation();
        const WeaponSprite* ws = nullptr;
        if (playerAnim) {
            if (playerAnim->getWeapon()) {
                ws = playerAnim->getWeapon();
                vData.offset = playerAnim->getWeaponOffset();
            } else if (playerAnim->getWeaponSecondary()) {
                ws = playerAnim->getWeaponSecondary();
                vData.offset = playerAnim->getSecondaryWeaponOffset();
            }
        }
        if (ws && playerAnim) {
            vData.baseTexture = ws->getBaseTexture();
            vData.layoutTexture = ws->getLayoutTexture();
            vData.baseRect = ws->getBaseRect();
            vData.overlayRect = ws->getOverlayRect();
            vData.rarityColor = ws->getRarityColor();
            vData.fortificationLevel = ws->getFortificationLevel();
            vData.origin = ws->getOrigin();
            vData.scale = playerAnim->getBaseScale();
            vData.isTwoHanded = playerAnim->isWeaponTwoHanded();
        }

        const AnimationClip* clip = playerAnim ? playerAnim->getCurrentClip() : nullptr;
        float ghostSpeedMult = 3.5f;
        float ghost1Delay = 0.06f;
        bool ghost1Miss = !CombatCalculator::tryToHit(caster, target);
        GhostSlashSystem::spawn(playerCaster, target, 1, numHits, clip, ghost1Delay, ghostSpeedMult, vData, ghost1Miss);

        if (numHits >= 3) {
            float ghost2Delay = ghost1Delay + 0.09f;
            bool ghost2Miss = !CombatCalculator::tryToHit(caster, target);
            GhostSlashSystem::spawn(playerCaster, target, 2, numHits, clip, ghost2Delay, ghostSpeedMult, vData, ghost2Miss);
        }
    }

    std::cout << "[SKILL] Basic Attack executed.\n";
}
