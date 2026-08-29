#pragma once
#include "../Skill.h"
#include "core/engine/IRenderable.h"
#include <SFML/Graphics.hpp>
#include <vector>

class Entity;
class ParticleSystem;
class CombatFeedback;
class CombatSystem;
class EntityManager;
class ResourceManager;
class WorldManager;

struct OrbProjectile : public IRenderable, public sf::Drawable {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float distanceTraveled = 0.f;
    float maxDistance = 600.f;
    float hitRadius = 18.f;
    int damage = 60;
    Entity* caster = nullptr;
    bool alive = true;
    float animTimer = 0.f;
    const sf::Texture* texture = nullptr;

    float getY() const { return position.y + 15.f; }
    RenderType getRenderType() const override { return RenderType::Generic; }
    const sf::Drawable* getDrawable() const override { return this; }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const override {
        outTex = nullptr;
    }
};

class BasicOrb : public Skill {
public:
    BasicOrb();

    void setCastTargetPosition(sf::Vector2f targetPos) { mCastTargetPos = targetPos; }

    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void updateChargeVisuals(Entity* caster, ParticleSystem* particles) override;
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, CombatSystem* combatSystem = nullptr) override {}

    static void updateAll(float dt, EntityManager& entityManager, CombatSystem& combatSystem, ParticleSystem& particleSystem, ResourceManager& res, WorldManager* worldManager = nullptr);
    static void clearAll();

    static std::vector<OrbProjectile>& getActiveOrbs() { return sActiveOrbs; }
    static const sf::Texture* sOrbTexture;

private:
    sf::Vector2f mCastTargetPos = {0.f, 0.f};
    static std::vector<OrbProjectile> sActiveOrbs;
};
