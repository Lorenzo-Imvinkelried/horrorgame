#include "Animation.h"
#include "AnimationCurves.h"
#include "Config.h"
#include <algorithm>

float Animation::getHitDelayFactor(const std::string& clipName) {
    ensureSpeedCurvesLoaded();

    if (clipName.empty()) return cfg::Combat::PLAYER_ATTACK_DELAY_FACTOR;

    const auto& curves = getAnimationSpeedCurves();
    auto it = curves.find(clipName);
    if (it == curves.end() || it->second.phases.empty()) {
        return cfg::Combat::PLAYER_ATTACK_DELAY_FACTOR;
    }

    const auto& curve = it->second;
    float targetHitProg = curve.hitProgress;

    float totalRealTime = 0.f;
    float hitRealTime = 0.f;

    for (const auto& phase : curve.phases) {
        float pStart = phase.startProgress;
        float pEnd = phase.endProgress;
        float speed = (phase.speedMultiplier > 0.001f) ? phase.speedMultiplier : 1.0f;

        float dP = pEnd - pStart;
        if (dP <= 0.f) continue;

        float phaseRealTime = dP / speed;
        totalRealTime += phaseRealTime;

        if (targetHitProg >= pEnd) {
            hitRealTime += phaseRealTime;
        } else if (targetHitProg > pStart) {
            float dPHit = targetHitProg - pStart;
            hitRealTime += dPHit / speed;
        }
    }

    if (totalRealTime <= 0.0001f) return cfg::Combat::PLAYER_ATTACK_DELAY_FACTOR;

    float factor = hitRealTime / totalRealTime;
    return std::clamp(factor, 0.1f, 0.95f);
}

void Animation::reset() {
    mAnimTimer = 0.f;
    mPrevTimer = 0.f;
    mBlendStartStates.clear();
    mBlendTimer = -1.f;
}

void Animation::setColor(sf::Color color) {
    if (!mIsLoaded) return;
    for (auto& node : mNodes) {
        for (int i = 0; i < 6; ++i) {
            node.quad[i].color = color;
        }
    }
    if (mWeapon) {
        sf::Color wColor = color;
        wColor.a = 255;
        mWeapon->setColor(wColor);
    }
    if (mWeaponSecondary) {
        sf::Color wColor = color;
        wColor.a = 255;
        mWeaponSecondary->setColor(wColor);
    }
}

sf::FloatRect Animation::getBodyBounds() const {
    if (!mIsLoaded) return sf::FloatRect();
    
    float minX = 999999.f, minY = 999999.f;
    float maxX = -999999.f, maxY = -999999.f;

    for (const auto& node : mNodes) {
        if (node.name != "body") continue;
        for (int i = 0; i < 6; ++i) {
            float x = node.quad[i].position.x;
            float y = node.quad[i].position.y;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
        break;
    }

    if (minX == 999999.f) return sf::FloatRect();
    return sf::FloatRect(sf::Vector2f(minX, minY), sf::Vector2f(maxX - minX, maxY - minY));
}

sf::FloatRect Animation::getGlobalBounds() const {
    sf::FloatRect r = getNodeGlobalBounds("body");
    sf::FloatRect h = getNodeGlobalBounds("head");
    if (r.size.x == 0.f) return h;
    if (h.size.x == 0.f) return r;
    
    float top = std::min(r.position.y, h.position.y);
    float left = std::min(r.position.x, h.position.x);
    float bottom = std::max(r.position.y + r.size.y, h.position.y + h.size.y);
    float right = std::max(r.position.x + r.size.x, h.position.x + h.size.x);
    return sf::FloatRect(sf::Vector2f(left, top), sf::Vector2f(right - left, bottom - top));
}

sf::Vector2f Animation::getNodePosition(const std::string& name) const {
    if (name == "weapon" && mWeapon) {
        return mWeapon->getPosition();
    }
    if (!mIsLoaded) return {};
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        const auto& fn = mNodes[it->second];
        return (fn.quad[0].position + fn.quad[4].position) * 0.5f;
    }
    return {};
}

float Animation::getNodeRotation(const std::string& name) const {
    if (name == "weapon" && mWeapon) {
        return mWeapon->getRotation().asDegrees();
    }
    if (!mIsLoaded) return 0.f;
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        return mNodes[it->second].currentRot;
    }
    return 0.f;
}

sf::Vector2f Animation::getNodeScale(const std::string& name) const {
    if (!mIsLoaded) return {1.f, 1.f};
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        return mNodes[it->second].currentScale;
    }
    return {1.f, 1.f};
}

sf::FloatRect Animation::getNodeLocalBounds(const std::string& name) const {
    if (!mIsLoaded) return {};
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        return mNodes[it->second].localBounds;
    }
    return {};
}

float Animation::getNodeCustomRestY(const std::string& name) const {
    if (!mIsLoaded) return 0.f;
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        return mNodes[it->second].customRestPos.y;
    }
    return 0.f;
}

float Animation::getNodeCurrentY(const std::string& name) const {
    if (!mIsLoaded) return 0.f;
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        return mNodes[it->second].currentPos.y;
    }
    return 0.f;
}

std::vector<sf::Vertex> Animation::getNodeVertices(const std::string& name) const {
    if (!mIsLoaded) return {};
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        const auto& fn = mNodes[it->second];
        return std::vector<sf::Vertex>(fn.quad.begin(), fn.quad.end());
    }
    return {};
}

const sf::Texture* Animation::getAtlasTexture() const {
    return mAtlasTexture;
}

sf::FloatRect Animation::getNodeGlobalBounds(const std::string& name) const {
    if (name == "weapon" && mWeapon) {
        return mWeapon->getGlobalBounds();
    }
    if (!mIsLoaded) return sf::FloatRect();
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        const auto& fn = mNodes[it->second];
        float minX = 999999.f, minY = 999999.f;
        float maxX = -999999.f, maxY = -999999.f;
        for (int i = 0; i < 6; ++i) {
            float x = fn.quad[i].position.x;
            float y = fn.quad[i].position.y;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
        return sf::FloatRect(sf::Vector2f(minX, minY), sf::Vector2f(maxX - minX, maxY - minY));
    }
    return sf::FloatRect();
}

void Animation::applyTerrainPhysics(float lDepth, float lRot, float rDepth, float rRot, float bodyDepth) {
    mIK.applyTerrainPhysics(lDepth, lRot, rDepth, rRot, bodyDepth);
}

float Animation::getFootLDepth() const { return mIK.getFootLDepth(); }
float Animation::getFootRDepth() const { return mIK.getFootRDepth(); }
float Animation::getFootLRotIK() const { return mIK.getFootLRotIK(); }
float Animation::getFootRRotIK() const { return mIK.getFootRRotIK(); }
float Animation::getBodyDepth() const { return mIK.getBodyDepth(); }
void Animation::setEquippedWeightFactor(float weightFactor) { mIK.setEquippedWeightFactor(weightFactor); }
float Animation::getEquippedWeightFactor() const { return mIK.getEquippedWeightFactor(); }

void Animation::applyHitRecoil(sf::Vector2f hitDir, float forceMultiplier) {
    mIK.applyHitRecoil(hitDir, forceMultiplier);
}

void Animation::applyAttackImpulse(sf::Vector2f attackDir, float forceMultiplier) {
    mIK.applyAttackImpulse(attackDir, forceMultiplier);
}

void Animation::triggerHitStop(float durationSeconds) {
    mIK.triggerHitStop(durationSeconds);
}
