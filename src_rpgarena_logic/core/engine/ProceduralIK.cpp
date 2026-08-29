#include "ProceduralIK.h"
#include "animation/Animation.h"
#include "Config.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include <cmath>
#include <algorithm>

void ProceduralIK::reset() {
    mHitRecoil = HitRecoilState{};
    mAttackImpulse = AttackImpulseState{};
    mSway = ProceduralSwayState{};
    mFootIK = TerrainFootIKState{};
    mStepImpact = StepImpactState{};
    mHitStopTimer = 0.f;
}

void ProceduralIK::update(float dtSec, sf::Vector2f position, bool isMoving, 
                          const TerrainDeformSystem* terrain,
                          sf::Vector2f leftFootBase, sf::Vector2f rightFootBase) {
    const float originalDt = dtSec; // Guardar el dt real original para cálculos de velocidad

    if (mHitStopTimer > 0.f) {
        mHitStopTimer -= dtSec;
        dtSec *= cfg::IK::HITSTOP_TIME_SCALE; // Ralentizar animación/física durante el hit-stop
    }
    if (dtSec > 0.1f) dtSec = 0.1f;

    // 1. Integración de Resortes (Hit Recoil, Attack Impulse & Step Impact Spring Dampers)
    float stiffness = cfg::IK::RECOIL_STIFFNESS;
    float damping = cfg::IK::RECOIL_DAMPING;

    sf::Vector2f recoilAccel = -stiffness * mHitRecoil.offset - damping * mHitRecoil.vel;
    mHitRecoil.vel += recoilAccel * dtSec;
    mHitRecoil.offset += mHitRecoil.vel * dtSec;

    float rotAccel = -stiffness * mHitRecoil.rot - damping * mHitRecoil.rotVel;
    mHitRecoil.rotVel += rotAccel * dtSec;
    mHitRecoil.rot += mHitRecoil.rotVel * dtSec;

    float atkStiffness = cfg::IK::ATTACK_STIFFNESS;
    float atkDamping = cfg::IK::ATTACK_DAMPING;
    sf::Vector2f atkAccel = -atkStiffness * mAttackImpulse.offset - atkDamping * mAttackImpulse.vel;
    mAttackImpulse.vel += atkAccel * dtSec;
    mAttackImpulse.offset += mAttackImpulse.vel * dtSec;

    float atkRotAccel = -atkStiffness * mAttackImpulse.rot - atkDamping * mAttackImpulse.rotVel;
    mAttackImpulse.rotVel += atkRotAccel * dtSec;
    mAttackImpulse.rot += mAttackImpulse.rotVel * dtSec;

    float stepStiffness = cfg::IK::STEP_IMPACT_STIFFNESS;
    float stepDamping = cfg::IK::STEP_IMPACT_DAMPING;
    float stepAccel = -stepStiffness * mStepImpact.yOffset - stepDamping * mStepImpact.velY;
    mStepImpact.velY += stepAccel * dtSec;
    mStepImpact.yOffset += mStepImpact.velY * dtSec;

    // 2. Inclinación y Balanceo por Movimiento (Movement Lean & Sway)
    if (mSway.hasPrevPos) {
        sf::Vector2f delta = position - mSway.prevPos;
        float distSq = delta.x * delta.x + delta.y * delta.y;
        if (distSq > (200.f * 200.f)) {
            // Teletransporte / Portal / Spawn: resetear posición previa y velocidad sin tirones de resortes
            mSway.prevPos = position;
            mSway.smoothedVelocity = {0.f, 0.f};
            mSway.currentLean = 0.f;
            mSway.targetLean = 0.f;
        } else {
            sf::Vector2f instVel = delta / std::max(originalDt, 0.001f);
            float smoothFactor = 1.0f - std::exp(-originalDt * cfg::IK::SWAY_SMOOTH_FREQ);
            mSway.smoothedVelocity += (instVel - mSway.smoothedVelocity) * smoothFactor;
        }
    } else {
        mSway.prevPos = position;
        mSway.hasPrevPos = true;
        mSway.smoothedVelocity = {0.f, 0.f};
    }
    mSway.prevPos = position;

    float speed = std::sqrt(mSway.smoothedVelocity.x * mSway.smoothedVelocity.x + mSway.smoothedVelocity.y * mSway.smoothedVelocity.y);
    mSway.targetLean = std::clamp(mSway.smoothedVelocity.x * cfg::IK::SWAY_LEAN_MULT, -cfg::IK::SWAY_MAX_LEAN, cfg::IK::SWAY_MAX_LEAN);
    
    float leanFactor = 1.0f - std::exp(-originalDt * cfg::IK::LEAN_SMOOTH_FREQ);
    mSway.currentLean += (mSway.targetLean - mSway.currentLean) * leanFactor;

    if (isMoving || speed > 15.f) {
        mSway.swayPhase += dtSec * (speed * 0.035f + 4.f);
    } else {
        mSway.swayPhase += dtSec * 2.5f;
    }

    // 3. Terrain Foot IK & Pit Sampling
    if (leftFootBase != sf::Vector2f(0.f, 0.f)) {
        leftFootBase.y -= mFootIK.footLDepth;
    }
    if (rightFootBase != sf::Vector2f(0.f, 0.f)) {
        rightFootBase.y -= mFootIK.footRDepth;
    }

    if (terrain && terrain->isInitialized() && leftFootBase.x != 0.f && rightFootBase.x != 0.f) {
        float targetDepthL = terrain->getDepthAt(leftFootBase);
        float targetDepthR = terrain->getDepthAt(rightFootBase);
        
        float footLerpFactor = 1.0f - std::exp(-dtSec * cfg::IK::FOOT_LERP_FREQ);
        mFootIK.footLDepth += (targetDepthL - mFootIK.footLDepth) * footLerpFactor;
        mFootIK.footRDepth += (targetDepthR - mFootIK.footRDepth) * footLerpFactor;
        
        float sampleOffset = cfg::IK::FOOT_SAMPLE_OFFSET;
        float dL_north = terrain->getDepthAt(leftFootBase  + sf::Vector2f(0.f, -sampleOffset));
        float dL_south = terrain->getDepthAt(leftFootBase  + sf::Vector2f(0.f,  sampleOffset));
        float dR_north = terrain->getDepthAt(rightFootBase + sf::Vector2f(0.f, -sampleOffset));
        float dR_south = terrain->getDepthAt(rightFootBase + sf::Vector2f(0.f,  sampleOffset));
        
        float dL_west  = terrain->getDepthAt(leftFootBase  + sf::Vector2f(-sampleOffset, 0.f));
        float dL_east  = terrain->getDepthAt(leftFootBase  + sf::Vector2f( sampleOffset, 0.f));
        float dR_west  = terrain->getDepthAt(rightFootBase + sf::Vector2f(-sampleOffset, 0.f));
        float dR_east  = terrain->getDepthAt(rightFootBase + sf::Vector2f( sampleOffset, 0.f));

        int pointsInPitL = 0;
        if (targetDepthL > 0.f) pointsInPitL++;
        if (dL_north > 0.f) pointsInPitL++;
        if (dL_south > 0.f) pointsInPitL++;
        if (dL_west > 0.f) pointsInPitL++;
        if (dL_east > 0.f) pointsInPitL++;

        float slopeL = 0.f;
        if (pointsInPitL < 4) {
            slopeL = dL_east - dL_west;
        }

        int pointsInPitR = 0;
        if (targetDepthR > 0.f) pointsInPitR++;
        if (dR_north > 0.f) pointsInPitR++;
        if (dR_south > 0.f) pointsInPitR++;
        if (dR_west > 0.f) pointsInPitR++;
        if (dR_east > 0.f) pointsInPitR++;

        float slopeR = 0.f;
        if (pointsInPitR < 4) {
            slopeR = dR_east - dR_west;
        }

        float maxRot = cfg::IK::FOOT_MAX_ROT;
        float targetRotL = std::atan2(slopeL, 10.f) * (180.f / 3.14159265f);
        float targetRotR = std::atan2(slopeR, 10.f) * (180.f / 3.14159265f);
        targetRotL = std::clamp(targetRotL, -maxRot, maxRot);
        targetRotR = std::clamp(targetRotR, -maxRot, maxRot);
        
        mFootIK.footLRotIK = 0.f;
        mFootIK.footRRotIK = 0.f;
        
        mFootIK.bodyDepth = std::min(mFootIK.footLDepth, mFootIK.footRDepth) * cfg::IK::BODY_DEPTH_MULT;
    } else if (terrain) {
        mFootIK.footLDepth = 0.f;
        mFootIK.footRDepth = 0.f;
        mFootIK.footLRotIK = 0.f;
        mFootIK.footRRotIK = 0.f;
        mFootIK.bodyDepth  = 0.f;
    }
}

void ProceduralIK::solveTwoHandGripIK(std::vector<SkeletonNode>& nodes, 
                                      const std::unordered_map<std::string, size_t>& nodeMap, 
                                      bool isTwoHanded) {
    if (!isTwoHanded || nodes.empty()) return;
    auto itR = nodeMap.find("hand_r");
    auto itL = nodeMap.find("hand_l");
    if (itR == nodeMap.end() || itL == nodeMap.end()) return;

    auto& handR = nodes[itR->second];
    auto& handL = nodes[itL->second];

    float angleRad = handR.currentRot * (3.14159265f / 180.f);
    sf::Vector2f gripOffset = {
        std::cos(angleRad) * cfg::IK::GRIP_OFFSET_X - std::sin(angleRad) * cfg::IK::GRIP_OFFSET_Y,
        std::sin(angleRad) * cfg::IK::GRIP_OFFSET_X + std::cos(angleRad) * cfg::IK::GRIP_OFFSET_Y
    };

    sf::Vector2f ikTarget = handR.currentPos + gripOffset;
    float distSq = (ikTarget.x - handL.currentPos.x) * (ikTarget.x - handL.currentPos.x) + 
                   (ikTarget.y - handL.currentPos.y) * (ikTarget.y - handL.currentPos.y);
    if (distSq > (40.f * 40.f)) {
        handL.currentPos = ikTarget;
    } else {
        handL.currentPos += (ikTarget - handL.currentPos) * cfg::IK::GRIP_LERP_FACTOR;
    }
    handL.currentRot = 0.f;
}

void ProceduralIK::applyHitRecoil(sf::Vector2f hitDir, float forceMultiplier) {
    float len = std::sqrt(hitDir.x * hitDir.x + hitDir.y * hitDir.y);
    if (len > 0.001f) {
        hitDir /= len;
    } else {
        hitDir = {1.f, 0.f};
    }
    mHitRecoil.vel += hitDir * (cfg::IK::RECOIL_FORCE_LIN * forceMultiplier);
    mHitRecoil.rotVel += (hitDir.x > 0.f ? 1.f : -1.f) * (cfg::IK::RECOIL_FORCE_ROT * forceMultiplier);
}

void ProceduralIK::applyAttackImpulse(sf::Vector2f attackDir, float forceMultiplier) {
    float len = std::sqrt(attackDir.x * attackDir.x + attackDir.y * attackDir.y);
    if (len > 0.001f) {
        attackDir /= len;
    } else {
        attackDir = {1.f, 0.f};
    }
    mAttackImpulse.vel += attackDir * (cfg::IK::ATTACK_FORCE_LIN * forceMultiplier);
    mAttackImpulse.rotVel += (attackDir.x > 0.f ? 1.f : -1.f) * (cfg::IK::ATTACK_FORCE_ROT * forceMultiplier);
}

void ProceduralIK::applyStepImpact(float forceMultiplier) {
    mStepImpact.yOffset += 1.0f * forceMultiplier;
    mStepImpact.velY += cfg::IK::STEP_IMPACT_FORCE * forceMultiplier;
}

void ProceduralIK::applyEquipWeightImpact(float deltaWeight) {
    float force = std::clamp(deltaWeight * 12.0f, 5.0f, 20.0f);
    mStepImpact.yOffset += force * 0.1f;
    mStepImpact.velY += force * 2.0f;
}

float ProceduralIK::getStepImpactHeadOffsetY() const {
    return mStepImpact.yOffset * cfg::IK::HEAD_IMPACT_LAG;
}

void ProceduralIK::triggerHitStop(float durationSeconds) {
    mHitStopTimer = durationSeconds;
}

void ProceduralIK::applyTerrainPhysics(float lDepth, float lRot, float rDepth, float rRot, float bodyDepth) {
    mFootIK.footLDepth = lDepth;
    mFootIK.footLRotIK = lRot;
    mFootIK.footRDepth = rDepth;
    mFootIK.footRRotIK = rRot;
    mFootIK.bodyDepth  = bodyDepth;
}
