#include "GhostSlash.h"
#include "CombatSystem.h"
#include "entities/player/Player.h"
#include "core/engine/animation/Animation.h"
#include <algorithm>
#include <cmath>
#include <iostream>

std::vector<GhostSlashInstance> GhostSlashSystem::sActiveSlashes;

void GhostSlashInstance::init(Player* player, Entity* target, int hitIndex, int totalHits, 
                              const AnimationClip* clip, float spawnDelay, float speedMultiplier,
                              const GhostSlashVisualData& visualData, bool isMiss) 
{
    mAttacker = player;
    mDefender = target;
    mHitIndex = hitIndex;
    mTotalHits = totalHits;
    mClip = clip;
    mSpawnDelay = spawnDelay;
    mStarted = (spawnDelay <= 0.f);
    mDamageApplied = false;
    mIsMiss = isMiss;
    mFinished = false;
    mTimer = 0.f;

    if (speedMultiplier <= 0.f) speedMultiplier = 1.0f;
    float baseDuration = (clip && clip->duration > 0.f) ? clip->duration : 0.55f;
    mAnimDuration = baseDuration / speedMultiplier;

    std::string clipName = clip ? clip->name : "";
    float delayFactor = Animation::getHitDelayFactor(clipName);
    mImpactDelay = mAnimDuration * delayFactor;

    if (player) {
        mOriginPos = player->getPosition();
        mFacingDir = player->getFacingDir();
    } else {
        mOriginPos = {0.f, 0.f};
        mFacingDir = 1;
    }

    mVisualData = visualData;
    if (mVisualData.origin.x == 0.f && mVisualData.origin.y == 0.f && mVisualData.baseRect.size.y > 0) {
        mVisualData.origin = {0.f, static_cast<float>(mVisualData.baseRect.size.y)};
    }

    if (mVisualData.baseTexture && mVisualData.layoutTexture) {
        mWeaponSprite.setTextures(*mVisualData.baseTexture, *mVisualData.layoutTexture);
        mWeaponSprite.setVisuals(mVisualData.baseRect, mVisualData.overlayRect, mVisualData.rarityColor);
        mWeaponSprite.setFortificationLevel(mVisualData.fortificationLevel);
        mWeaponSprite.setOrigin(mVisualData.origin);
    }

    updateTransform();
}

void GhostSlashInstance::updateTransform() {
    float progress = (mAnimDuration > 0.f) ? std::clamp(mTimer / mAnimDuration, 0.f, 1.f) : 1.f;
    float clipTime = (mClip && mClip->duration > 0.f) ? (progress * mClip->duration) : progress;

    // Lock position and orientation to the strike origin and original facing direction
    sf::Vector2f attackerPos = mOriginPos;
    int origFacingDir = mFacingDir;

    bool hasWeaponTrack = false;
    sf::Vector2f weaponTrackPos(0.f, 0.f);
    float weaponTrackRot = 0.f;

    if (mClip) {
        std::vector<std::string> trackKeys = {"weapon", "weapon_r", "weapon_main", "weapon_sec", "weapon_l"};
        for (const auto& key : trackKeys) {
            auto itPos = mClip->positionTracks.find(key);
            if (itPos != mClip->positionTracks.end() && !itPos->second.frames.empty()) {
                weaponTrackPos = itPos->second.evaluate(clipTime);
                hasWeaponTrack = true;
            }
            auto itRot = mClip->rotationTracks.find(key);
            if (itRot != mClip->rotationTracks.end() && !itRot->second.frames.empty()) {
                weaponTrackRot = itRot->second.evaluate(clipTime);
                hasWeaponTrack = true;
            }
            if (hasWeaponTrack) break;
        }
    }

    if (hasWeaponTrack) {
        float wx = attackerPos.x + (weaponTrackPos.x) * -origFacingDir * mVisualData.scale.x;
        float wy = attackerPos.y + (weaponTrackPos.y) * mVisualData.scale.y;
        float wRot = weaponTrackRot * -origFacingDir;

        mWeaponSprite.setPosition({ std::round(wx), std::round(wy) });
        mWeaponSprite.setScale({ mVisualData.scale.x * -origFacingDir, mVisualData.scale.y });
        mWeaponSprite.setRotation(sf::degrees(wRot));
    } else {
        std::string targetNode = "hand_r";
        if (mClip) {
            auto itR = mClip->positionTracks.find("hand_r");
            auto itL = mClip->positionTracks.find("hand_l");
            if ((itR == mClip->positionTracks.end() || itR->second.frames.empty()) &&
                (itL != mClip->positionTracks.end() && !itL->second.frames.empty())) {
                targetNode = "hand_l";
            }
        }

        sf::Vector2f handPos = attackerPos;
        float nodeRot = 0.f;
        if (mClip) {
            auto itPos = mClip->positionTracks.find(targetNode);
            if (itPos != mClip->positionTracks.end() && !itPos->second.frames.empty()) {
                sf::Vector2f p = itPos->second.evaluate(clipTime);
                handPos = attackerPos + sf::Vector2f(p.x * -origFacingDir * mVisualData.scale.x, p.y * mVisualData.scale.y);
            }
            auto itRot = mClip->rotationTracks.find(targetNode);
            if (itRot != mClip->rotationTracks.end() && !itRot->second.frames.empty()) {
                nodeRot = itRot->second.evaluate(clipTime) * -origFacingDir;
            }
        }

        // For 1-handed (16x16) weapons where the keyframe clip has 0 rotation on the hand bone,
        // create a dynamic, natural slash arc so the phantom sword swings expressively
        if (nodeRot == 0.f) {
            float swingAngle = 0.f;
            if (progress < 0.25f) {
                float t = progress / 0.25f;
                swingAngle = t * 25.f; // windup tilt back
            } else if (progress < 0.70f) {
                float t = (progress - 0.25f) / 0.45f;
                swingAngle = 25.f - t * 95.f; // fast slash forward down to -70 deg
            } else {
                float t = (progress - 0.70f) / 0.30f;
                swingAngle = -70.f + t * 70.f; // recovery back to 0
            }
            nodeRot = swingAngle * -origFacingDir;
        }

        float rad = nodeRot * (3.14159265f / 180.f);
        sf::Vector2f finalOffset = mVisualData.offset;
        sf::Vector2f localOffset = { finalOffset.x * origFacingDir, finalOffset.y };
        sf::Vector2f rotatedOffset = {
            localOffset.x * std::cos(rad) - localOffset.y * std::sin(rad),
            localOffset.x * std::sin(rad) + localOffset.y * std::cos(rad)
        };

        mWeaponSprite.setPosition({
            std::round(handPos.x + rotatedOffset.x),
            std::round(handPos.y + rotatedOffset.y)
        });
        mWeaponSprite.setScale({ mVisualData.scale.x * -origFacingDir, mVisualData.scale.y });
        mWeaponSprite.setRotation(sf::degrees(nodeRot));
    }

    float alphaFactor = 1.0f;
    if (progress < 0.15f) {
        alphaFactor = progress / 0.15f;
    } else if (progress > 0.70f) {
        alphaFactor = (1.0f - progress) / 0.30f;
    }
    std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(170.f * alphaFactor, 0.f, 255.f));
    sf::Color ghostColor(175, 225, 255, alpha);
    mWeaponSprite.setColor(ghostColor);
}

void GhostSlashInstance::update(float dt, CombatSystem* combatSystem) {
    if (mFinished) return;

    if (mSpawnDelay > 0.f) {
        mSpawnDelay -= dt;
        if (mSpawnDelay > 0.f) return;
        mStarted = true;
    } else {
        mStarted = true;
    }

    mTimer += dt;
    updateTransform();

    if (!mDamageApplied && mTimer >= mImpactDelay) {
        mDamageApplied = true;
        if (combatSystem && mDefender) {
            if (combatSystem->isEntityAllocated(mDefender) && mDefender->isAlive() && !mDefender->isReturningToSpawn()) {
                if (mAttacker) {
                    mAttacker->addToAggro(mDefender);
                    mDefender->onAggroedBy(mAttacker);
                }
                if (mIsMiss) {
                    combatSystem->createMissEffect(mDefender, mAttacker);
                } else {
                    combatSystem->performSingleStrike(mAttacker, mDefender, mHitIndex, mTotalHits);
                }
            }
        }
    }

    if (mTimer >= mAnimDuration) {
        mFinished = true;
    }
}

float GhostSlashInstance::getY() const {
    return mOriginPos.y + 8.f;
}

void GhostSlashInstance::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!mStarted || mFinished) return;
    if (!mVisualData.baseTexture) return;

    target.draw(mWeaponSprite, states);
}

void GhostSlashSystem::spawn(Player* player, Entity* target, int hitIndex, int totalHits,
                            const AnimationClip* clip, float spawnDelay, float speedMultiplier,
                            const GhostSlashVisualData& visualData, bool isMiss) 
{
    GhostSlashInstance slash;
    slash.init(player, target, hitIndex, totalHits, clip, spawnDelay, speedMultiplier, visualData, isMiss);
    sActiveSlashes.push_back(slash);
}

void GhostSlashSystem::updateAll(float dt, CombatSystem* combatSystem) {
    for (auto& slash : sActiveSlashes) {
        slash.update(dt, combatSystem);
    }
    sActiveSlashes.erase(
        std::remove_if(sActiveSlashes.begin(), sActiveSlashes.end(),
                       [](const GhostSlashInstance& s) { return s.isFinished(); }),
        sActiveSlashes.end()
    );
}

void GhostSlashSystem::clearAll() {
    sActiveSlashes.clear();
}
