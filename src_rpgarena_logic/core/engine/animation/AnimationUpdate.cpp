#include "Animation.h"
#include "AnimationCurves.h"
#include "Config.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include <cmath>
#include <algorithm>

bool Animation::hasPositionTrack(const AnimationClip* clip, const std::string& nodeName) {
    if (!clip) return false;
    auto it = clip->positionTracks.find(nodeName);
    return (it != clip->positionTracks.end() && !it->second.frames.empty());
}

bool Animation::hasRotationTrack(const AnimationClip* clip, const std::string& nodeName) {
    if (!clip) return false;
    auto it = clip->rotationTracks.find(nodeName);
    return (it != clip->rotationTracks.end() && !it->second.frames.empty());
}

bool Animation::hasScaleTrack(const AnimationClip* clip, const std::string& nodeName) {
    if (!clip) return false;
    auto it = clip->scaleTracks.find(nodeName);
    return (it != clip->scaleTracks.end() && !it->second.frames.empty());
}

void Animation::playBase(const AnimationClip* clip, float startTime, float customBlendDuration) {
    if (mBaseClip != clip || (clip && !clip->isLoop)) {
        if (mBaseClip && mIsLoaded && !mNodes.empty()) {
            mBlendStartStates.clear();
            for (const auto& node : mNodes) {
                mBlendStartStates[node.name] = { node.basePos, node.baseRot, node.baseScale };
            }
            mBlendTimer = 0.f;
            if (customBlendDuration > 0.f) {
                mBlendDuration = customBlendDuration;
            } else {
                mBlendDuration = 0.15f;
            }
        } else {
            mBlendTimer = -1.f;
        }
        mBaseClip = clip;
        mCurrentClip = clip;
        mBaseTimer = startTime;
        mPrevTimer = startTime;
    }
}

void Animation::playAction(const AnimationClip* clip, float startTime, float speedMultiplier) {
    mActionClip = clip;
    mActionTimer = startTime;
    mActionSpeedMultiplier = speedMultiplier;
}

void Animation::update(sf::Time dt, bool isMoving, sf::Vector2f position, int facingDir, float speedMultiplier, const TerrainDeformSystem* terrain) {
    if (!mIsLoaded) return;
    
    mLastBasePos = position;
    mLastFacingDir = facingDir;
    
    // Si no hay ningún clip cargado, aplicamos la posición de reposo
    if (!mBaseClip && !mActionClip) {
        for (auto& node : mNodes) {
            node.currentPos = node.customRestPos;
            node.currentRot = 0.f;
            node.currentScale = {1.f, 1.f};
            updateNodeQuad(node, position, facingDir);
        }
        return;
    }

    // --- CÁLCULO DE IK Y FÍSICA PROCEDIMENTAL / TERRENO ---
    sf::Vector2f leftFootBase = getNodePosition("foot_l");
    sf::Vector2f rightFootBase = getNodePosition("foot_r");
    mIK.update(dt.asSeconds(), position, isMoving, terrain, leftFootBase, rightFootBase);

    mFiredEvents.clear();

    // 1. Avanzar Temporizador del Clip BASE
    ensureSpeedCurvesLoaded();

    float dynamicSpeedMult = 1.0f;
    if (mBaseClip && !mBaseClip->name.empty() && mBaseClip->duration > 0.f) {
        const auto& curves = getAnimationSpeedCurves();
        auto itCurve = curves.find(mBaseClip->name);
        if (itCurve != curves.end()) {
            float progress = mBaseTimer / mBaseClip->duration;
            for (const auto& phase : itCurve->second.phases) {
                if (progress >= phase.startProgress && progress <= phase.endProgress) {
                    dynamicSpeedMult = phase.speedMultiplier / itCurve->second.totalRealTime;
                    break;
                }
            }
        }
    }

    if (mBaseClip) {
        mPrevTimer = mBaseTimer;
        mBaseTimer += dt.asSeconds() * speedMultiplier * dynamicSpeedMult;

        bool wrapped = false;
        if (mBaseClip->isLoop) {
            float lStart = mBaseClip->getEffectiveLoopStart();
            float lEnd = mBaseClip->getEffectiveLoopEnd();
            if (mBaseTimer >= lEnd) {
                wrapped = true;
                float loopLen = lEnd - lStart;
                if (loopLen > 0.001f) {
                    float excess = mBaseTimer - lEnd;
                    mBaseTimer = lStart + std::fmod(excess, loopLen);
                } else {
                    mBaseTimer = lStart;
                }
            }
        } else if (mBaseTimer > mBaseClip->duration) {
            mBaseTimer = mBaseClip->duration;
        }

        for (const auto& ev : mBaseClip->events) {
            if (wrapped) {
                if ((ev.time > mPrevTimer && ev.time <= mBaseClip->duration) ||
                    (ev.time >= 0.f && ev.time <= mBaseTimer)) {
                    mFiredEvents.push_back(ev.name);
                }
            } else {
                if (ev.time > mPrevTimer && ev.time <= mBaseTimer) {
                    mFiredEvents.push_back(ev.name);
                }
            }
        }
    }

    // 2. Avanzar Temporizador del Clip ACTION (Habilidad / Gesto / Ataque)
    if (mActionClip) {
        float prevActionT = mActionTimer;
        mActionTimer += dt.asSeconds() * mActionSpeedMultiplier;

        for (const auto& ev : mActionClip->events) {
            if (ev.time > prevActionT && ev.time <= mActionTimer) {
                mFiredEvents.push_back(ev.name);
            }
        }

        // Desenganchar la acción automáticamente al expirar si no es cíclica
        if (!mActionClip->isLoop && mActionTimer >= mActionClip->duration) {
            mActionClip = nullptr;
        } else if (mActionClip->isLoop && mActionClip->duration > 0.f) {
            float lStart = mActionClip->getEffectiveLoopStart();
            float lEnd = mActionClip->getEffectiveLoopEnd();
            if (mActionTimer >= lEnd) {
                float loopLen = lEnd - lStart;
                if (loopLen > 0.001f) {
                    float excess = mActionTimer - lEnd;
                    mActionTimer = lStart + std::fmod(excess, loopLen);
                } else {
                    mActionTimer = lStart;
                }
            }
        }
    }

    // Actualizar alias para compatibilidad con código legacy
    mCurrentClip = mActionClip ? mActionClip : mBaseClip;
    mAnimTimer = mActionClip ? mActionTimer : mBaseTimer;

    // 3. Blending de Locomoción Base
    float effectiveBlendDuration = mBlendDuration;
    if (mBaseClip) {
        effectiveBlendDuration = std::min(mBlendDuration, mBaseClip->duration * 0.3f);
    }

    if (mBlendTimer >= 0.f) {
        mBlendTimer += dt.asSeconds() * speedMultiplier;
        if (mBlendTimer >= effectiveBlendDuration) {
            mBlendTimer = -1.f;
        }
    }

    float blendFactor = 1.0f;
    if (mBlendTimer >= 0.f && effectiveBlendDuration > 0.f) {
        blendFactor = mBlendTimer / effectiveBlendDuration;
        if (blendFactor > 1.f) blendFactor = 1.f;
    }

    // Advance shield guard timer if active
    if (mShieldGuardClip && mShieldGuardClip->duration > 0.f) {
        mGuardTimer += dt.asSeconds();
        if (mGuardTimer >= mShieldGuardClip->duration) {
            mGuardTimer = mShieldGuardClip->isLoop ? std::fmod(mGuardTimer, mShieldGuardClip->duration) : mShieldGuardClip->duration;
        }
    } else {
        mGuardTimer = 0.f;
    }

    // PASO 1: Construir Pose Base
    for (auto& node : mNodes) {
        sf::Vector2f targetPos = node.customRestPos;
        float targetRot = 0.f;
        sf::Vector2f targetScale = {1.f, 1.f};

        bool isGuardedNode = (mShieldGuardClip != nullptr && node.name == mShieldedBoneName);
        const AnimationClip* clipToEvaluate = (isGuardedNode && mShieldGuardClip) ? mShieldGuardClip : mBaseClip;
        float timeToEvaluate = isGuardedNode ? mGuardTimer : mBaseTimer;

        if (clipToEvaluate) {
            std::string evalTrack = node.name;
            std::string restRef = node.name;
            if (facingDir == -1 && !isGuardedNode) {
                if (node.name == "hand_l") { evalTrack = "hand_r"; restRef = "hand_r"; }
                else if (node.name == "hand_r") { evalTrack = "hand_l"; restRef = "hand_l"; }
                else if (node.name == "foot_l") { evalTrack = "foot_r"; restRef = "foot_r"; }
                else if (node.name == "foot_r") { evalTrack = "foot_l"; restRef = "foot_l"; }
            }

            auto itPos = clipToEvaluate->positionTracks.find(evalTrack);
            if (itPos != clipToEvaluate->positionTracks.end() && !itPos->second.frames.empty()) {
                sf::Vector2f keyPos = itPos->second.evaluate(timeToEvaluate);
                sf::Vector2f defRest = node.defaultRestPos;
                sf::Vector2f custRest = node.customRestPos;
                if (restRef != node.name && mNodeMap.count(restRef)) {
                    defRest = mNodes[mNodeMap.at(restRef)].defaultRestPos;
                    custRest = mNodes[mNodeMap.at(restRef)].customRestPos;
                }
                sf::Vector2f clipDelta = keyPos - defRest;
                targetPos = custRest + clipDelta;
            }

            auto itRot = clipToEvaluate->rotationTracks.find(evalTrack);
            if (itRot != clipToEvaluate->rotationTracks.end() && !itRot->second.frames.empty()) {
                targetRot = itRot->second.evaluate(timeToEvaluate);
            }
            
            auto itScale = clipToEvaluate->scaleTracks.find(evalTrack);
            if (itScale != clipToEvaluate->scaleTracks.end() && !itScale->second.frames.empty()) {
                targetScale = itScale->second.evaluate(timeToEvaluate);
            }
        }

        if (mBlendTimer >= 0.f) {
            auto itBlend = mBlendStartStates.find(node.name);
            if (itBlend != mBlendStartStates.end()) {
                node.basePos = itBlend->second.pos + blendFactor * (targetPos - itBlend->second.pos);
                
                float startRot = itBlend->second.rot;
                float diff = targetRot - startRot;
                while (diff < -180.f) diff += 360.f;
                while (diff > 180.f) diff -= 360.f;
                node.baseRot = startRot + blendFactor * diff;

                node.baseScale = itBlend->second.scale + blendFactor * (targetScale - itBlend->second.scale);
            } else {
                node.basePos = targetPos;
                node.baseRot = targetRot;
                node.baseScale = targetScale;
            }
        } else {
            node.basePos = targetPos;
            node.baseRot = targetRot;
            node.baseScale = targetScale;
        }
    }

    // PASO 2: Action Override por canal (Sobrescribe solo las propiedades que Action define)
    if (mActionClip) {
        for (auto& node : mNodes) {
            auto itPos = mActionClip->positionTracks.find(node.name);
            if (itPos != mActionClip->positionTracks.end() && !itPos->second.frames.empty()) {
                sf::Vector2f keyPos = itPos->second.evaluate(mActionTimer);
                sf::Vector2f clipDelta = keyPos - node.defaultRestPos;
                node.basePos = node.customRestPos + clipDelta;
            }

            auto itRot = mActionClip->rotationTracks.find(node.name);
            if (itRot != mActionClip->rotationTracks.end() && !itRot->second.frames.empty()) {
                node.baseRot = itRot->second.evaluate(mActionTimer);
            }

            auto itScale = mActionClip->scaleTracks.find(node.name);
            if (itScale != mActionClip->scaleTracks.end() && !itScale->second.frames.empty()) {
                node.baseScale = itScale->second.evaluate(mActionTimer);
            }
        }
    }

    // PASO 3: Procedural IK & Physics
    for (auto& node : mNodes) {
        node.currentPos = node.basePos;
        node.currentRot = node.baseRot;
        node.currentScale = node.baseScale;

        // Capa de animación secundaria procedimental (Overlays & Springs)
        const auto& sway = mIK.getSway();
        float bodyBob = std::sin(sway.swayPhase) * (isMoving ? 1.5f : 0.6f);

        sf::Vector2f combinedOffsetWorld = mIK.getCombinedWorldOffset();
        sf::Vector2f localRecoil = { combinedOffsetWorld.x * facingDir, combinedOffsetWorld.y };

        sf::Vector2f weightLagWorld = -sway.smoothedVelocity * (cfg::IK::WEIGHT_LAG_MULT * mIK.getEquippedWeightFactor());
        weightLagWorld.x = std::clamp(weightLagWorld.x, -20.f, 20.f);
        weightLagWorld.y = std::clamp(weightLagWorld.y, -20.f, 20.f);
        sf::Vector2f weightLagLocal = { weightLagWorld.x * facingDir, weightLagWorld.y };

        float eqWeight = mIK.getEquippedWeightFactor();
        float weightStanceSink = (eqWeight > 1.0f) ? (eqWeight - 1.0f) * cfg::IK::WEIGHT_STANCE_SINK : 0.f;

        if (mEnableProceduralIK) {
            if (node.name == "body") {
                node.currentPos += localRecoil;
                node.currentPos.y += bodyBob;
                node.currentPos.y += mIK.getStepImpactBodyOffsetY();
                node.currentPos.y += weightStanceSink;
            } else if (node.name == "head") {
                node.currentPos += localRecoil * 0.85f;
                node.currentPos.y += bodyBob * 1.2f;
                node.currentPos.y += mIK.getStepImpactHeadOffsetY();
                node.currentPos.y += weightStanceSink * 1.2f;
            } else if (node.name == "hand_r" || node.name == "hand_l") {
                node.currentPos += localRecoil * 0.7f;
                node.currentPos += weightLagLocal;
                node.currentPos.y += mIK.getStepImpactBodyOffsetY() * 0.5f;
                node.currentPos.y += weightStanceSink * 1.4f;
            }

            if (node.name == "foot_l") {
                node.currentPos.y += (facingDir == -1) ? mIK.getFootRDepth() : mIK.getFootLDepth();
            } else if (node.name == "foot_r") {
                node.currentPos.y += (facingDir == -1) ? mIK.getFootLDepth() : mIK.getFootRDepth();
            } else if (node.name == "body" || node.name == "head" || node.name == "hand_l" || node.name == "hand_r") {
                node.currentPos.y += mIK.getBodyDepth();
            }
        }
    }

    // 4. IK Solver para Manos / Armas de 2 Manos (Two-Hand Grip IK)
    bool actionAnimatesHandL = (mActionClip != nullptr && mActionClip->positionTracks.count("hand_l"));
    if (mEnableTwoHandGripIK && !actionAnimatesHandL) {
        solveTwoHandGripIK();
    }

    // 5. Actualizar los Quads de todos los nodos
    for (auto& node : mNodes) {
        updateNodeQuad(node, position, facingDir);
    }
    
    // Emulate old mLeftFootDown logic for TerrainDeformSystem
    mLeftFootDown = popEvent("footstep_l");
    mRightFootDown = popEvent("footstep_r");
    
    if (mLeftFootDown || mRightFootDown) {
        mIK.applyStepImpact(1.0f);
    }

    if (mLeftFootDown && mNodeMap.count("foot_l")) {
        auto& fn = mNodes[mNodeMap["foot_l"]];
        mLandedLeftPos = (fn.quad[0].position + fn.quad[4].position) * 0.5f; // Center of the foot quad
        mLandedLeftOrigin = {fn.localBounds.size.x * 0.5f, fn.localBounds.size.y * 0.5f};
        mLandedLeftScale = { fn.currentScale.x * mBaseScale.x * -facingDir, fn.currentScale.y * mBaseScale.y };
        mLandedLeftRot = fn.currentRot * -facingDir;
    }
    if (mRightFootDown && mNodeMap.count("foot_r")) {
        auto& fn = mNodes[mNodeMap["foot_r"]];
        mLandedRightPos = (fn.quad[0].position + fn.quad[4].position) * 0.5f; // Center of the foot quad
        mLandedRightOrigin = {fn.localBounds.size.x * 0.5f, fn.localBounds.size.y * 0.5f};
        mLandedRightScale = { fn.currentScale.x * mBaseScale.x * -facingDir, fn.currentScale.y * mBaseScale.y };
        mLandedRightRot = fn.currentRot * -facingDir;
    }

    int origFacingDir = facingDir;
    bool usePixelSnap = false;
    auto snap = [usePixelSnap](float v) -> float {
        return usePixelSnap ? std::round(v) : v;
    };
    if (mWeapon) {
        bool hasWeaponTrack = false;
        sf::Vector2f weaponTrackPos(15.f, 5.f);
        float weaponTrackRot = 0.f;

        if (mCurrentClip) {
            std::vector<std::string> trackKeys = {"weapon", "weapon_main", "weapon_r"};
            for (const auto& key : trackKeys) {
                auto itPos = mCurrentClip->positionTracks.find(key);
                auto itRot = mCurrentClip->rotationTracks.find(key);
                if (itPos != mCurrentClip->positionTracks.end() && !itPos->second.frames.empty()) {
                    weaponTrackPos = itPos->second.evaluate(mAnimTimer);
                    hasWeaponTrack = true;
                }
                if (itRot != mCurrentClip->rotationTracks.end() && !itRot->second.frames.empty()) {
                    weaponTrackRot = itRot->second.evaluate(mAnimTimer);
                    hasWeaponTrack = true;
                }
                if (hasWeaponTrack) break;
            }
        }

        if (hasWeaponTrack) {
            float wx = position.x + (weaponTrackPos.x) * -origFacingDir * mBaseScale.x;
            float wy = position.y + (weaponTrackPos.y) * mBaseScale.y;
            float wRot = weaponTrackRot * -origFacingDir;

            mWeapon->setPosition({ snap(wx), snap(wy) });
            mWeapon->setScale({ mBaseScale.x * -origFacingDir, mBaseScale.y });
            mWeapon->setRotation(sf::degrees(wRot));
        } else {
            std::string targetNode = "hand_r";
            sf::Vector2f handPos = getNodePosition(targetNode);
            float nodeRot = getNodeRotation(targetNode) * -origFacingDir;
            float rad = nodeRot * (3.14159265f / 180.f);

            sf::Vector2f finalOffset = mWeaponOffset;
            sf::Vector2f localOffset = { finalOffset.x * origFacingDir, finalOffset.y };
            sf::Vector2f rotatedOffset = {
                localOffset.x * std::cos(rad) - localOffset.y * std::sin(rad),
                localOffset.x * std::sin(rad) + localOffset.y * std::cos(rad)
            };

            mWeapon->setPosition({
                snap(handPos.x + rotatedOffset.x),
                snap(handPos.y + rotatedOffset.y)
            });
            mWeapon->setScale({ mBaseScale.x * -origFacingDir, mBaseScale.y });
            mWeapon->setRotation(sf::degrees(nodeRot));
        }
    }
    if (mWeaponSecondary) {
        std::string targetNodeSec = "hand_l";
        sf::Vector2f handPos = getNodePosition(targetNodeSec);

        sf::Vector2f finalOffsetSec = mSecondaryWeaponOffset;

        float weaponTrackRotSec = 0.f;
        sf::Vector2f weaponPosDeltaSec(0.f, 0.f);

        if (mCurrentClip) {
            std::vector<std::string> trackKeysSec = {"weapon_sec", "weapon_l", "shield"};
            for (const auto& key : trackKeysSec) {
                auto itRot = mCurrentClip->rotationTracks.find(key);
                if (itRot != mCurrentClip->rotationTracks.end() && !itRot->second.frames.empty()) {
                    weaponTrackRotSec = itRot->second.evaluate(mAnimTimer);
                }
                auto itPos = mCurrentClip->positionTracks.find(key);
                if (itPos != mCurrentClip->positionTracks.end() && !itPos->second.frames.empty()) {
                    sf::Vector2f currentPos = itPos->second.evaluate(mAnimTimer);
                    sf::Vector2f firstPos = itPos->second.frames[0].value;

                    sf::Vector2f handTrackPosSec(0.f, 0.f);
                    sf::Vector2f handFirstPosSec(0.f, 0.f);
                    auto itHandPosSec = mCurrentClip->positionTracks.find(targetNodeSec);
                    if (itHandPosSec != mCurrentClip->positionTracks.end() && !itHandPosSec->second.frames.empty()) {
                        handTrackPosSec = itHandPosSec->second.evaluate(mAnimTimer);
                        handFirstPosSec = itHandPosSec->second.frames[0].value;
                    }

                    sf::Vector2f currentRelSec = currentPos - handTrackPosSec;
                    sf::Vector2f firstRelSec = firstPos - handFirstPosSec;
                    weaponPosDeltaSec = currentRelSec - firstRelSec;
                }
                if (itRot != mCurrentClip->rotationTracks.end() || itPos != mCurrentClip->positionTracks.end()) {
                    break;
                }
            }
        }

        float nodeRot = (getNodeRotation(targetNodeSec) - weaponTrackRotSec) * -origFacingDir;
        float rad = nodeRot * (3.14159265f / 180.f);

        sf::Vector2f totalOffsetSec = finalOffsetSec + weaponPosDeltaSec;
        sf::Vector2f localOffset = { totalOffsetSec.x * origFacingDir, totalOffsetSec.y };
        sf::Vector2f rotatedOffset = {
            localOffset.x * std::cos(rad) - localOffset.y * std::sin(rad),
            localOffset.x * std::sin(rad) + localOffset.y * std::cos(rad)
        };

        mWeaponSecondary->setPosition({
            snap(handPos.x + rotatedOffset.x),
            snap(handPos.y + rotatedOffset.y)
        });
        mWeaponSecondary->setScale({ mBaseScale.x * -origFacingDir, mBaseScale.y });
        mWeaponSecondary->setRotation(sf::degrees(nodeRot));
    }
}

void Animation::updateNodeQuad(SkeletonNode& node, sf::Vector2f basePos, int facingDir) {
    sf::Transform t;
    
    float flipX = static_cast<float>(-facingDir);
    sf::Vector2f mirroredPos = {node.currentPos.x * flipX, node.currentPos.y};
    t.translate(basePos + mirroredPos);
    
    if (node.currentRot != 0.f) t.rotate(sf::degrees(node.currentRot * flipX));
    
    sf::Vector2f finalScale = {node.currentScale.x * mBaseScale.x * flipX, node.currentScale.y * mBaseScale.y};
    t.scale(finalScale);

    sf::Vector2f p1 = t.transformPoint({node.localBounds.position.x, node.localBounds.position.y});
    sf::Vector2f p2 = t.transformPoint({node.localBounds.position.x + node.localBounds.size.x, node.localBounds.position.y});
    sf::Vector2f p3 = t.transformPoint({node.localBounds.position.x + node.localBounds.size.x, node.localBounds.position.y + node.localBounds.size.y});
    sf::Vector2f p4 = t.transformPoint({node.localBounds.position.x, node.localBounds.position.y + node.localBounds.size.y});

    node.quad[0].position = p1; node.quad[1].position = p2; node.quad[2].position = p4;
    node.quad[3].position = p2; node.quad[4].position = p3; node.quad[5].position = p4;
    node.currentTransform = t;
}

bool Animation::popEvent(const std::string& eventName) {
    for (const auto& ev : mFiredEvents) {
        if (ev == eventName) return true;
    }
    return false;
}

void Animation::solveTwoHandGripIK() {
    mIK.solveTwoHandGripIK(mNodes, mNodeMap, mWeaponIsTwoHanded);
}
