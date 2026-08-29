#include "ShieldSystem.h"
#include "entities/Entity.h"
#include "core/engine/ResourceManager.h"
#include <iostream>

ShieldSystem* ShieldSystem::sInstance = nullptr;

ShieldSystem::ShieldSystem() {
    sInstance = this;
}

ShieldSystem::~ShieldSystem() {
    if (sInstance == this) {
        sInstance = nullptr;
    }
}

bool ShieldSystem::canUseShield(const Entity* entity) const {
    if (!entity) return false;
    return entity->hasShieldEquipped();
}

bool ShieldSystem::toggleGuard(Entity* entity) {
    if (!entity) return false;
    if (!canUseShield(entity)) {
        // If shield is unequipped, ensure guard is turned off
        setGuardActive(entity, false);
        return false;
    }
    bool newState = !isGuardActive(entity);
    setGuardActive(entity, newState);
    return newState;
}

void ShieldSystem::setGuardActive(Entity* entity, bool active) {
    if (!entity) return;
    if (active) {
        if (!canUseShield(entity)) return;
        mActiveGuards.insert(entity);
        entity->setGuardActive(true);
        std::cout << "[ShieldSystem] Guard MODE ACTIVATED for entity.\n";
    } else {
        mActiveGuards.erase(entity);
        entity->setGuardActive(false);
        std::cout << "[ShieldSystem] Guard MODE DEACTIVATED for entity.\n";
    }
}

bool ShieldSystem::isGuardActive(const Entity* entity) const {
    if (!entity) return false;
    return mActiveGuards.find(entity) != mActiveGuards.end() || entity->isGuardActive();
}

float ShieldSystem::getGuardDamageReductionBonus(const Entity* entity) const {
    return isGuardActive(entity) ? 30.0f : 0.0f;
}

float ShieldSystem::getGuardBlockChanceBonus(const Entity* entity) const {
    return isGuardActive(entity) ? 25.0f : 0.0f;
}

const sf::Texture* ShieldSystem::getShieldTexture(ResourceManager& res, const Entity* entity) const {
    try {
        return &res.getTexture("assets/items/shields/16x16x10_escudos.png");
    } catch (...) {
        return nullptr;
    }
}
