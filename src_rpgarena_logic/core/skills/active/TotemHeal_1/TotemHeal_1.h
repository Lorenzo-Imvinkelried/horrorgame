#pragma once
#include "core/skills/Skill.h"
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

struct TotemInstance : public IRenderable, public sf::Drawable {
    sf::Vector2f position = {0.f, 0.f};
    float lifetime = 10.0f;
    float maxLifetime = 10.0f;
    float radius = 140.0f;
    int healAmount = 30;
    float tickTimer = 1.0f;
    float particleTimer = 0.f;
    bool alive = true;
    Entity* caster = nullptr;
    const sf::Texture* texture = nullptr;

    float getY() const { return position.y; }
    RenderType getRenderType() const override { return RenderType::Generic; }
    bool castsShadow() const override { return true; }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const override;
    void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const override;
};

class TotemHeal_1 : public Skill {
public:
    TotemHeal_1();

    void setCastTargetPosition(sf::Vector2f targetPos) { mCastTargetPos = targetPos; }

    void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) override;
    void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) override;

    static void spawnTotem(sf::Vector2f position, Entity* caster, float radius, int healAmount, float duration, const sf::Texture* tex);
    static void updateAll(float dt, Entity* player, CombatFeedback& feedback, ParticleSystem& particleSystem, ResourceManager& res, WorldManager* worldManager = nullptr);
    static void clearAll();

    static std::vector<TotemInstance>& getActiveTotems() { return sActiveTotems; }
    static const sf::Texture* sTotemTexture;
    static const sf::Texture* sParticleTexture;
    static std::vector<sf::IntRect> sParticleRects;

private:
    sf::Vector2f mCastTargetPos = {0.f, 0.f};
    static std::vector<TotemInstance> sActiveTotems;
};
