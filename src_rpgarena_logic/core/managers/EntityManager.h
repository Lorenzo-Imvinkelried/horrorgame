#pragma once
#include <functional> // [ADDED] For std::function
#include <vector>
#include <memory>
#include <unordered_set>
#include <string>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "core/engine/ResourceManager.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "entities/mob/Mob.h"
#include "core/systems/RespawnSystem.h"
#include "core/engine/WorldRegistry.h"

// Forward declaration to avoid circular dependency
class CombatSystem;

class EntityManager {
public:
    EntityManager(ResourceManager& res, class ItemManager& itemMgr);
    ~EntityManager();

    // Init Logic
    void loadMobBlueprints(); 
    void setSkillManager(class SkillManager* sm) { mSkillManager = sm; }
    void loadEntitiesFromFile(ResourceManager& res, const std::string& filename, const WorldData& worldData);

    // Update Logic
    void update(sf::Time dt, sf::Vector2f playerPos, sf::Vector2f viewSize = {1200.f, 900.f}, const class TerrainDeformSystem* terrain = nullptr); // Added viewSize and terrain
    void updateActivationRanges(sf::Vector2f viewSize);
    
    // Render Logic
    // Returns a list of entities that are visible/renderable if needed, 
    // or we can passing a batcher here. For now, PlayingState handles drawing via iteration.
    // Ideally EntityManager should just say "Here are the visible entities".
    // But to minimize drawing refactor now, we expose accessors.

    // Accessors
    std::vector<std::unique_ptr<Entity>>& getActiveEntities();
    const std::vector<std::unique_ptr<Entity>>& getActiveEntities() const;
    const std::vector<std::unique_ptr<Entity>>& getSleepingEntities() const; // [DEBUG]
    
    Player* getPlayer() const; 

    // Spawning Helper
    void spawnPlayer(std::unique_ptr<Player> player);
    void spawnMob(const std::string& blueprintName, sf::Vector2f pos, CombatSystem* cs = nullptr, int levelOverride = -1, bool isBoss = false); // Helper

    // Cleanup Logic (with callback for external cleanup)
    void cleanupDeadEntities(std::function<void(Entity*)> onDeath);
    
    // Inject dependencies into entities
    void setCombatSystem(CombatSystem* combatSystem);

    // Cleanup
    void clear();

    // Pool Debugging
    size_t getMobPoolSize() const { return mMobPool.size(); }

    // Optimized Lookup
    bool isValid(Entity* entity) const;

    // [SPATIAL GRID]
    void updateSpatialGrid();
    std::vector<Entity*> querySpatialGrid(sf::Vector2f pos, float radius) const;

private:
    // internal helpers
    void spawnMobFromTicket(const RespawnTicket& ticket, ResourceManager& res);

private:
    // Entities
    std::vector<std::unique_ptr<Entity>> mActiveEntities;
    std::vector<std::unique_ptr<Entity>> mSleepingEntities;
    std::vector<std::unique_ptr<Entity>> mMobPool; // [OBJECT POOLING]
    std::vector<std::unique_ptr<Entity>> mEntitiesToSpawn;
    
    // [OPTIMIZATION] Registry for O(1) existence checks.
    // Contains raw pointers to ALL allocated entities (Active + Sleeping).
    std::unordered_set<Entity*> mEntityRegistry;

    sf::Clock mEntityActivationTimer; // [OPTIMIZATION]


    // Systems owned by Manager
    RespawnSystem mRespawnSystem; 
    ResourceManager& mResourceManager; // [ADDED]
    class ItemManager& mItemManager;
    
    // Blueprints
    std::map<std::string, MobBlueprint> mMobBlueprints;

    Player* mPlayerPtr = nullptr;
    CombatSystem* mCombatSystem = nullptr;
    class SkillManager* mSkillManager = nullptr;

    // Hysteresis constants
    float mActivationRangeSq = 0.f;
    float mDeactivationRangeSq = 0.f;

    // [SPATIAL GRID]
    struct SpatialGrid {
        std::vector<std::vector<Entity*>> cells;
        std::vector<int> usedIndices; // [OPTIMIZATION] Track active cells
        int width = 0;
        int height = 0;
        int cellSize = 256;
    } mGrid;
};
