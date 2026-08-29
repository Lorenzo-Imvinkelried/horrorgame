#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// Forward Declarations
class Game;
class Player;
class Entity;
class CombatSystem;
class SkillManager;
class ParticleSystem;
class EntityManager;
class UIManager;
class ResourceManager;

class PlayerController {
public:
    PlayerController(CombatSystem& combat, SkillManager& skills, ParticleSystem& particles, EntityManager& entities, UIManager& ui);

    void setPlayer(Player* player);
    
    // [REFACTOR] View is now passed per-frame to ensure it's always sync with Render
    // void setView(const sf::View& view); 

    void handleInput(Game& game, sf::Time dt, const sf::View& view);
    void update(sf::Time dt);

    void setTargetedEntity(Entity* entity);
    Entity* getTargetedEntity() const;

    bool hasExplosionClick() const { return mHasExplosionClick; }
    sf::Vector2f consumeExplosionClick() { mHasExplosionClick = false; return mExplosionPos; }
    bool isExplosionMode() const { return mExplosionMode; }

    void reset(); // [FIX] Clear state on level change

    bool isTargetingGroundSkill() const { return mTargetingGroundSkillId != -1; }
    void cancelGroundTargeting() { mTargetingGroundSkillId = -1; }
    void drawGroundTargeting(sf::RenderTarget& target, SkillManager& skillManager, ResourceManager& res);

private:
    void tryCastSkill(int slotIndex);
    void handleMouseInput(Game& game, const sf::View& view);
    
    // Skill Queue Logic
    void checkPendingSkill();

    // References to external systems
    CombatSystem& mCombatSystem;
    SkillManager& mSkillManager;
    ParticleSystem& mParticleSystem;
    EntityManager& mEntityManager;
    UIManager& mUIManager;

    Player* mPlayerPtr = nullptr;
    // const sf::View* mView = nullptr; // [REMOVED]

    // State
    Entity* mTargetedEntity = nullptr;
    
    // Skill Queue (Moved from PlayingState)
    int mPendingSkillId = -1;
    Entity* mPendingSkillTarget = nullptr;

    // Ground Skill Targeting
    int mTargetingGroundSkillId = -1;

    // Debug Mode (Terrain Explosion)
    bool mExplosionMode = false;
    bool mHasExplosionClick = false;
    sf::Vector2f mExplosionPos;

    // Tab targeting cycle state
    std::vector<Entity*> mTabTargetCandidates;
    int mTabTargetIndex = -1;
    float mTabResetTimer = 0.f;

    sf::Vector2f mLastMouseWorldPos = {0.f, 0.f};
};
