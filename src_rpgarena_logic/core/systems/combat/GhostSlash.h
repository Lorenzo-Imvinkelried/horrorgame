#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "core/engine/IRenderable.h"
#include "core/items/WeaponSprite.h"
#include "core/engine/AnimCore.h"

class Entity;
class Player;
class CombatSystem;

struct GhostSlashVisualData {
    const sf::Texture* baseTexture = nullptr;
    const sf::Texture* layoutTexture = nullptr;
    sf::IntRect baseRect;
    sf::IntRect overlayRect;
    sf::Color rarityColor = sf::Color::Transparent;
    int fortificationLevel = 0;
    sf::Vector2f origin = {0.f, 0.f};
    sf::Vector2f scale = {1.f, 1.f};
    sf::Vector2f offset = {0.f, 0.f};
    bool isTwoHanded = false;
};

class GhostSlashInstance : public IRenderable, public sf::Drawable {
public:
    GhostSlashInstance() = default;

    void init(Player* player, Entity* target, int hitIndex, int totalHits, 
              const AnimationClip* clip, float spawnDelay, float speedMultiplier,
              const GhostSlashVisualData& visualData, bool isMiss);

    void update(float dt, CombatSystem* combatSystem);

    bool isFinished() const { return mFinished; }
    bool isStarted() const { return mStarted; }

    float getY() const;
    RenderType getRenderType() const override { return RenderType::Generic; }
    const sf::Drawable* getDrawable() const override { return this; }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const override {
        outTex = nullptr;
    }

private:
    void updateTransform();

    Player* mAttacker = nullptr;
    Entity* mDefender = nullptr;
    int mHitIndex = 1;
    int mTotalHits = 2;
    const AnimationClip* mClip = nullptr;
    float mSpawnDelay = 0.f;
    float mAnimDuration = 0.2f;
    float mTimer = 0.f;
    float mImpactDelay = 0.1f;
    bool mStarted = false;
    bool mDamageApplied = false;
    bool mIsMiss = false;
    bool mFinished = false;

    sf::Vector2f mOriginPos;
    int mFacingDir = 1;
    GhostSlashVisualData mVisualData;

    mutable WeaponSprite mWeaponSprite;
};

class GhostSlashSystem {
public:
    GhostSlashSystem() = default;

    static void spawn(Player* player, Entity* target, int hitIndex, int totalHits,
                      const AnimationClip* clip, float spawnDelay, float speedMultiplier,
                      const GhostSlashVisualData& visualData, bool isMiss);

    static void updateAll(float dt, CombatSystem* combatSystem);
    static void clearAll();

    static const std::vector<GhostSlashInstance>& getActiveSlashes() { return sActiveSlashes; }
    static std::vector<GhostSlashInstance>& getActiveSlashesMutable() { return sActiveSlashes; }

private:
    static std::vector<GhostSlashInstance> sActiveSlashes;
};
