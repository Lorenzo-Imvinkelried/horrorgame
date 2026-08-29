#include "CombatFeedback.h"
#include "core/systems/FXSystem.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"
#include "entities/Entity.h"
#include "Config.h"
#include "utils/Random.h"

CombatFeedback::CombatFeedback(FXSystem& fx, ParticleSystem* particles)
    : mFX(fx)
    , mParticles(particles)
{
}

void CombatFeedback::onAttackStart(Entity* attacker, Entity* target) {
    if (attacker && target) {
        attacker->startAttackAnimation(target);
    }
}

void CombatFeedback::onHit(Entity* target, int damage, bool isCrit, bool isTrueDamage, bool isBlocked, bool showParticles, float offsetY, float scale, Entity* attacker) {
    if (!target) return;

    sf::FloatRect bounds = target->getHitImpactBounds();
    
    // 1. Damage Numbers
    if (cfg::Debug::ENABLE_FLOATING_TEXT) {
        if (isBlocked) {
             float finalOffset = (offsetY > 0.1f) ? offsetY : cfg::UI::FloatingText::OFFSET_Y_DAMAGE;
             mFX.addDamageNumber(damage, bounds, finalOffset, isCrit, scale, sf::Color::Cyan, target);
        } else if (isTrueDamage) {
             float finalOffset = (offsetY > 0.1f) ? offsetY : (cfg::UI::FloatingText::OFFSET_Y_DAMAGE + cfg::UI::FloatingText::OFFSET_Y_TRUE_EXTRA);
             mFX.addTrueDamageNumber(damage, bounds, finalOffset, isCrit, 1.0f, target); 
        } else {
             float finalOffset = (offsetY > 0.1f) ? offsetY : cfg::UI::FloatingText::OFFSET_Y_DAMAGE;
             mFX.addDamageNumber(damage, bounds, finalOffset, isCrit, scale, sf::Color::Transparent, target);
        }
    }

    // 2. Particles
    if (showParticles && cfg::Debug::ENABLE_COMBAT_PARTICLES) {
        onHitImpact(target, isCrit);
    }
}

void CombatFeedback::onHitImpact(Entity* target, bool isCrit) {
    if (!target || !mParticles) return;

    sf::FloatRect hitBounds = target->getHitImpactBounds();
    sf::Vector2f center;
    center.x = hitBounds.position.x + hitBounds.size.x * 0.5f;
    center.y = hitBounds.position.y + hitBounds.size.y * 0.5f; 

    mParticles->emitHitImpact(center, 10, sf::Color::Red, target);
    
    sf::Vector2f hitPos = center;
    hitPos.x += Random::Float(-hitBounds.size.x * 0.3f, hitBounds.size.x * 0.3f);
    hitPos.y += Random::Float(-hitBounds.size.y * 0.3f, hitBounds.size.y * 0.3f);
    mFX.addHitRing(hitPos, 60.f);
    
    if (isCrit) {
        mParticles->emitHitImpact(center, 20, sf::Color::Red, target);
    }
}

void CombatFeedback::onSkillHit(Entity* target, int skillId) {
    if (!target || !mParticles) return;

    sf::Vector2f center = target->getPosition();
    sf::FloatRect bounds = target->getHitImpactBounds();
    center.x = bounds.position.x + bounds.size.x * 0.5f;
    center.y = bounds.position.y + bounds.size.y * 0.5f;

    if (skillId == 1) { // Power Strike
        if (cfg::Debug::ENABLE_COMBAT_PARTICLES) {
            mParticles->emitPowerStrike(center, 60); 
        }
    }
}

void CombatFeedback::onHeal(Entity* target, int amount, Entity* source) {
    if (!target) return;
    if (cfg::Debug::ENABLE_FLOATING_TEXT) {
        sf::FloatRect bounds = target->getHitImpactBounds();
        mFX.addHealNumber(amount, bounds, cfg::UI::FloatingText::OFFSET_Y_HEAL, source ? source : target);
    }
}

void CombatFeedback::onMiss(Entity* target, Entity* attacker) {
    if (!target) return;
    if (cfg::Debug::ENABLE_FLOATING_TEXT) {
        sf::FloatRect bounds = target->getHitImpactBounds();
        mFX.addMiss(bounds, target);
    }
    if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound("assets/sounds/miss.wav", 100.f);
    }
}

void CombatFeedback::onExperience(Entity* source, int amount) {
    if (!source) return;
    if (cfg::Debug::ENABLE_FLOATING_TEXT) {
        sf::FloatRect bounds = source->getHitImpactBounds();
        mFX.addExperienceNumber(amount, bounds, source);
    }
}

void CombatFeedback::onBleedTick(Entity* target, int damage) {
    if (!target) return;
    
    if (cfg::Debug::ENABLE_FLOATING_TEXT) {
        sf::FloatRect bounds = target->getHitImpactBounds();
        mFX.addBleedNumber(damage, bounds, target);
    }
    
    if (mParticles && cfg::Debug::ENABLE_COMBAT_PARTICLES) {
        sf::Vector2f center = target->getPosition();
        center.y -= 25.f;
        mParticles->emitBloodDrip(center, 5, sf::Color::Red, target);
    }
}

void CombatFeedback::onStun(Entity* target, float duration) {
    if (!target || !mParticles) return;
    if (cfg::Debug::ENABLE_COMBAT_PARTICLES) {
        mParticles->emitStunStars(target, duration);
    }
}

void CombatFeedback::onAoE(sf::Vector2f center, float radius, Entity* owner) {
    if (!mParticles) return;
    
    int particleCount = static_cast<int>(radius * 0.6f);
    if (particleCount < 10) particleCount = 10;
    if (particleCount > 50) particleCount = 50;

    if (cfg::Debug::ENABLE_COMBAT_PARTICLES) {
        mParticles->emitRockBurst(center, radius, particleCount, owner);
    }
}

void CombatFeedback::onLevelUp(Entity* entity) {
    // Reserved for future level up VFX
}
