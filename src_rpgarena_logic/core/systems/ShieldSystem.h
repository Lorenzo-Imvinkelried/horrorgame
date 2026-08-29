#pragma once
#include <unordered_set>
#include <SFML/Graphics.hpp>

class Entity;
class ResourceManager;

class ShieldSystem {
public:
    ShieldSystem();
    ~ShieldSystem();

    static ShieldSystem* getInstance() { return sInstance; }

    bool canUseShield(const Entity* entity) const;
    bool toggleGuard(Entity* entity);
    void setGuardActive(Entity* entity, bool active);
    bool isGuardActive(const Entity* entity) const;

    float getGuardDamageReductionBonus(const Entity* entity) const;
    float getGuardBlockChanceBonus(const Entity* entity) const;

    const sf::Texture* getShieldTexture(ResourceManager& res, const Entity* entity) const;

private:
    static ShieldSystem* sInstance;
    std::unordered_set<const Entity*> mActiveGuards;
};
