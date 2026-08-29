#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

class TerrainDeformSystem;
struct SkeletonNode;

struct HitRecoilState {
    sf::Vector2f offset = {0.f, 0.f};
    sf::Vector2f vel = {0.f, 0.f};
    float rot = 0.f;
    float rotVel = 0.f;
};

struct AttackImpulseState {
    sf::Vector2f offset = {0.f, 0.f};
    sf::Vector2f vel = {0.f, 0.f};
    float rot = 0.f;
    float rotVel = 0.f;
};

struct ProceduralSwayState {
    float currentLean = 0.f;
    float targetLean = 0.f;
    float swayPhase = 0.f;
    sf::Vector2f smoothedVelocity = {0.f, 0.f};
    sf::Vector2f prevPos = {0.f, 0.f};
    bool hasPrevPos = false;
};

struct TerrainFootIKState {
    float footLDepth = 0.f;
    float footRDepth = 0.f;
    float footLRotIK = 0.f;
    float footRRotIK = 0.f;
    float bodyDepth = 0.f;
};

struct StepImpactState {
    float yOffset = 0.f;
    float velY = 0.f;
};

class ProceduralIK {
public:
    ProceduralIK() = default;

    void reset();

    void update(float dtSec, sf::Vector2f position, bool isMoving, 
                const TerrainDeformSystem* terrain,
                sf::Vector2f leftFootBase, sf::Vector2f rightFootBase);

    void applyHitRecoil(sf::Vector2f hitDir, float forceMultiplier = 1.0f);
    void applyAttackImpulse(sf::Vector2f attackDir, float forceMultiplier = 1.0f);
    void applyStepImpact(float forceMultiplier = 1.0f);
    void applyEquipWeightImpact(float deltaWeight = 1.0f);
    void triggerHitStop(float durationSeconds = 0.04f);

    void solveTwoHandGripIK(std::vector<SkeletonNode>& nodes, 
                            const std::unordered_map<std::string, size_t>& nodeMap, 
                            bool isTwoHanded);

    void applyTerrainPhysics(float lDepth, float lRot, float rDepth, float rRot, float bodyDepth);
    void setEquippedWeightFactor(float weightFactor) { mEquippedWeightFactor = weightFactor; }
    float getEquippedWeightFactor() const { return mEquippedWeightFactor; }

    sf::Vector2f getCombinedWorldOffset() const { return mHitRecoil.offset + mAttackImpulse.offset; }
    float getCombinedWorldRot() const { return mHitRecoil.rot + mAttackImpulse.rot; }

    float getStepImpactBodyOffsetY() const { return mStepImpact.yOffset; }
    float getStepImpactHeadOffsetY() const;

    float getHitStopTimer() const { return mHitStopTimer; }
    const TerrainFootIKState& getFootIK() const { return mFootIK; }
    TerrainFootIKState& getFootIK() { return mFootIK; }
    const ProceduralSwayState& getSway() const { return mSway; }

    float getFootLDepth() const { return mFootIK.footLDepth; }
    float getFootRDepth() const { return mFootIK.footRDepth; }
    float getFootLRotIK() const { return mFootIK.footLRotIK; }
    float getFootRRotIK() const { return mFootIK.footRRotIK; }
    float getBodyDepth() const { return mFootIK.bodyDepth; }

private:
    HitRecoilState mHitRecoil;
    AttackImpulseState mAttackImpulse;
    ProceduralSwayState mSway;
    TerrainFootIKState mFootIK;
    StepImpactState mStepImpact;
    float mHitStopTimer = 0.f;
    float mEquippedWeightFactor = 1.0f;
};
