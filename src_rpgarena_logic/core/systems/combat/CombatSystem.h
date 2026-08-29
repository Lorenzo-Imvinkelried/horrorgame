#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "core/ui/hud/Hud.h"
#include "CombatFeedback.h"

class EntityManager; 
struct Skill;

class CombatSystem {
public:
    CombatSystem(Hud& hud, FXSystem& fxSystem, ParticleSystem* ps); 
    static CombatSystem* getInstance() { return sInstance; }

    void setParticleSystem(ParticleSystem* ps);
    ParticleSystem* getParticleSystem() const { return mParticleSystem; }
    CombatFeedback& getFeedback() { return mFeedback; }
    void setSoundSystem(class SoundSystem* ss) { mSoundSystem = ss; }

    void update(sf::Time dt);
    
    void setPlayer(Player* player);
    Player* getPlayer() const { return mPlayer; } 
    Entity* getCurrentTarget() const { return mCombatTarget; }
    bool isInCombat() const { return mInCombat; } 
    bool isCastingSkill() const;
    float getCastProgress() const;
    const struct Skill* getPendingSkill() const { return mPendingSkill; } 

    void setCombatTarget(Entity* target);

    void requestAutoAttack();
    void notifyPlayerMoved();

    void setEntityManager(EntityManager* em); 

    // AOE Helpers
    void applyAoE(Entity* attacker, Entity* primaryTarget, int damageDealt);
    void applyAoEAt(Entity* attacker, sf::Vector2f center, float radius, int damageDealt, float aoePercent = 100.f);

    // Helpers
    void engageCombat();
    void disengageCombat();
    
    // Attack Logic
    void initiateAttack(Entity* target);
    void applyDamageImpact(Entity* target);
    
    // Skills
    void requestSkillAttack(const struct Skill* skill, Entity* target);
    void performSkillAttack(const struct Skill* skill, Entity* target);
    
    // Unified Combat
    void performAttack(Entity* attacker, Entity* defender);
    void performSingleStrike(Entity* attacker, Entity* defender, int hitIndex, int totalHits);
    
    // Visuals Wrapper
    void createMissEffect(Entity* target, Entity* attacker = nullptr);

    // Debuff List Management
    void registerDebuff(Entity* target);
    void onEntityDeath(Entity* target);

    // Auto-Attack Reset
    void resetAttackTimer(); 
    void cancelPendingAttack();

    bool isEntityAllocated(Entity* candidate) const;

private:
    void updateDebuffsAndBleeds(sf::Time dt);
    void updateAoEWaves(sf::Time dt);
    void spawnGhostSlashes(Entity* target, int totalHits);

    static CombatSystem* sInstance;

    Hud& mHud;
    CombatFeedback mFeedback; 
    
    Player*    mPlayer = nullptr;

    Entity*    mCombatTarget = nullptr;
    Entity*    mActualTarget  = nullptr;
    Entity*    mPendingTarget = nullptr;

    bool       mInCombat = false;
    bool       mAutoAttackRequested = false;
    bool       mPlayerMovedFlag    = false;
    bool       mAwaitingFirstHit = false;
    
    // Delay Damage (Skills)
    bool       mDamagePending = false;
    bool       mPendingMiss   = false; 
    sf::Clock  mDamageTimer;
    float      mDamageDelay = 0.f;

    // Sequential Multi-Strike (Auto-Attacks)
    struct MultiStrikeState {
        int totalStrikes = 1;
        int currentStrike = 0;
        float strikeDuration = 0.f;
        float strikeImpactDelay = 0.f;
        float speedMultiplier = 1.0f;
        Entity* target = nullptr;
        bool isMiss = false;
        sf::Clock strikeClock;
        bool strikeDamageApplied = false;
        bool active = false;
    } mMultiStrike;

    sf::Clock mCombatTimer;
    sf::Clock mLeashTimer;
    const float mLeashTime = 5;

    EntityManager* mEntityManager = nullptr;

    std::vector<Entity*> mDebuffedEntities;

    bool mAttackReset = false;
    
    const struct Skill* mPendingSkill = nullptr;
    
    ParticleSystem* mParticleSystem = nullptr; 
    class SoundSystem* mSoundSystem = nullptr;

    // Wave AOE
    struct AoEWave {
        Entity* attacker;
        sf::Vector2f center;
        float currentRadius;
        float maxRadius;
        float speed;
        int damageDealt;
        float aoePercent;
        
        struct Target {
            Entity* entity;
            float distSq;
        };
        std::vector<Target> targets; 
    };
    std::vector<AoEWave> mActiveWaves;
};
