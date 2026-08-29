#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <array>
#include "core/engine/ResourceManager.h"
#include "core/engine/AnimCore.h"
#include "core/engine/ProceduralIK.h"

#include "core/items/Item.h"
#include "core/items/WeaponSprite.h"

class TerrainDeformSystem;

struct SkeletonNode {
    std::string name;
    sf::FloatRect localBounds;
    std::array<sf::Vertex, 6> quad;
    
    // Transformaciones base de la animación (keyframes + blend de clips)
    sf::Vector2f basePos = {0.f, 0.f};
    float baseRot = 0.f;
    sf::Vector2f baseScale = {1.f, 1.f};

    // Transformaciones actuales finales (incluyendo IK procedimental y overlays)
    sf::Vector2f currentPos;
    float currentRot = 0.f;
    sf::Vector2f currentScale = {1.f, 1.f};
    
    sf::Vector2f defaultRestPos = {0.f, 0.f};
    sf::Vector2f customRestPos = {0.f, 0.f};
    sf::Transform currentTransform;
};

class Animation {
public:
    Animation() = default;

    bool loadDynamicParts(ResourceManager& res, const std::string& mobType, const std::vector<std::string>& partNames);
    void setCustomRestOffsets(sf::Vector2f head, sf::Vector2f handL, sf::Vector2f handR, sf::Vector2f footL, sf::Vector2f footR);
    bool loadSkeleton(ResourceManager& res, const std::string& path);
    
    void update(sf::Time dt, bool isMoving, sf::Vector2f position, int facingDir, float speedMultiplier = 1.0f, const TerrainDeformSystem* terrain = nullptr);
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default);
    void drawLayer(sf::RenderTarget& target, int layer, sf::RenderStates states = sf::RenderStates::Default);
    
    void playBase(const AnimationClip* clip, float startTime = 0.f, float customBlendDuration = -1.f);
    void playAction(const AnimationClip* clip, float startTime = 0.f, float speedMultiplier = 1.0f);
    void clearAction() { mActionClip = nullptr; }
    bool hasAction() const { return mActionClip != nullptr; }
    bool isActionFinished() const {
        if (!mActionClip) return true;
        if (mActionClip->isLoop) return false;
        return mActionTimer >= mActionClip->duration;
    }

    // Backward-compatibility wrapper for legacy single-clip callers
    void playAnimation(const AnimationClip* clip, float startTime = 0.f, float customBlendDuration = -1.f) {
        playBase(clip, startTime, customBlendDuration);
    }

    static bool hasPositionTrack(const AnimationClip* clip, const std::string& nodeName);
    static bool hasRotationTrack(const AnimationClip* clip, const std::string& nodeName);
    static bool hasScaleTrack(const AnimationClip* clip, const std::string& nodeName);

    void setScale(sf::Vector2f scale) { mBaseScale = scale; }
    void attack(float durationSeconds, bool useLeft, bool useRight) { /* Stub for now */ }
    static float getHitDelayFactor(const std::string& clipName);
    
    struct WeaponSpawnInfo {
        sf::Vector2f position;
        float rotation = 0.f;
        sf::Vector2f scale = {1.f, 1.f};
        sf::Vector2f origin = {0.f, 0.f};
        bool exists = false;
    };
    WeaponSpawnInfo getWeaponSpawnInfo(int slotIndex) const;

    struct ArmorVisualInfo {
        const sf::Texture* texture = nullptr;
        sf::IntRect textureRect;
        sf::Vector2f offset;
        float scale = 1.0f;
        bool exists = false;
        int fortificationLevel = 0;
    };
    const ArmorVisualInfo& getArmorVisual(EquipmentSlot slot) const { return mArmorVisuals[static_cast<int>(slot)]; }
    const std::unordered_map<std::string, size_t>& getNodeMap() const { return mNodeMap; }
    sf::Vector2f getBaseScale() const { return mBaseScale; }
    void applyEquipWeightImpact(float deltaWeight = 1.0f) { mIK.applyEquipWeightImpact(deltaWeight); }

    const WeaponSprite* getWeapon() const { return mWeapon.get(); }
    const WeaponSprite* getWeaponSecondary() const { return mWeaponSecondary.get(); }
    bool isWeaponTwoHanded() const { return mWeaponIsTwoHanded; }
    sf::Vector2f getWeaponOffset() const { return mWeaponOffset; }
    sf::Vector2f getSecondaryWeaponOffset() const { return mSecondaryWeaponOffset; }
 
    void setWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                          const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                          sf::Vector2f offset, bool isTwoHanded = false, int fortificationLevel = 0, bool isShield = false, bool shieldOverHand = false);
    void setSecondaryWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                                   const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                                   sf::Vector2f offset, int fortificationLevel = 0, bool isShield = false, bool shieldOverHand = false);
    void setArmorVisuals(EquipmentSlot slot, const sf::Texture* tex, const sf::IntRect& rect, sf::Vector2f offset, float scale, int fortificationLevel = 0);
    
    bool isFinished() const {
        if (mActionClip) {
            if (mActionClip->isLoop) return false;
            return mActionTimer >= mActionClip->duration;
        }
        if (!mBaseClip) return true;
        if (mBaseClip->isLoop) return false;
        return mBaseTimer >= mBaseClip->duration;
    }

    void setAnimTimer(float time) {
        mBaseTimer = time;
        mActionTimer = time;
        mAnimTimer = time;
        mPrevTimer = time;
        mIdleTimer = time;
    }
    float getAnimTimer() const { return mActionClip ? mActionTimer : mBaseTimer; }
    
    void setEnableProceduralIK(bool enable) { mEnableProceduralIK = enable; }
    bool isEnableProceduralIK() const { return mEnableProceduralIK; }

    void setEnableTwoHandGripIK(bool enable) { mEnableTwoHandGripIK = enable; }
    bool isEnableTwoHandGripIK() const { return mEnableTwoHandGripIK; }

    void setEnableIdleLayer(bool enable) { mEnableIdleLayer = enable; }
    bool isEnableIdleLayer() const { return mEnableIdleLayer; }
    
    void setBaseIdleClip(const AnimationClip* clip) { mBaseIdleClip = clip; }
    const AnimationClip* getBaseIdleClip() const { return mBaseIdleClip; }

    void setShieldGuardClip(const AnimationClip* clip, const std::string& shieldedBoneName = "") {
        mShieldGuardClip = clip;
        mShieldedBoneName = shieldedBoneName;
    }
    const AnimationClip* getShieldGuardClip() const { return mShieldGuardClip; }
    const std::string& getShieldedBoneName() const { return mShieldedBoneName; }

    const AnimationClip* getCurrentClip() const { return mActionClip ? mActionClip : mBaseClip; }
    const AnimationClip* getBaseClip() const { return mBaseClip; }
    const AnimationClip* getActionClip() const { return mActionClip; }
    float getBaseTimer() const { return mBaseTimer; }
    float getActionTimer() const { return mActionTimer; }
    float getGroundOffsetY() const { return mGroundOffsetY; }
    float getStride() const { return mStride; }
    float getWorldStride() const { return mStride * std::abs(mBaseScale.x); }
    int getLastFacingDir() const { return mLastFacingDir; }
    
    bool popEvent(const std::string& eventName); 

    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const;
    void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const;
    void getWeaponShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const;
    void getArmorShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const;
    void emitWeaponGibs(class GoreSystem& gore, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f) const;

    // Getters for specific nodes / parts
    sf::Vector2f getNodePosition(const std::string& name) const;
    float getLayerSortingY(int layer, float entityBaseY) const;
    float getNodeRotation(const std::string& name) const;
    sf::Vector2f getNodeScale(const std::string& name) const;
    sf::FloatRect getNodeLocalBounds(const std::string& name) const;
    float getNodeCustomRestY(const std::string& name) const;
    float getNodeCurrentY(const std::string& name) const;
    std::vector<sf::Vertex> getNodeVertices(const std::string& name) const;
    const sf::Texture* getAtlasTexture() const;
    void drawNode(const std::string& name, sf::RenderTarget& target, sf::RenderStates states) const;
    void drawSingleNodeAndArmor(const SkeletonNode& node, sf::RenderTarget& target, sf::RenderStates states) const;
    sf::FloatRect getNodeGlobalBounds(const std::string& name) const;
    const std::vector<SkeletonNode>& getNodes() const { return mNodes; }

    // Terrain physics & IK
    void applyTerrainPhysics(float lDepth, float lRot, float rDepth, float rRot, float bodyDepth);

    // [PROCEDURAL IK & RECOIL & ATTACK IMPULSE]
    void applyHitRecoil(sf::Vector2f hitDir, float forceMultiplier = 1.0f);
    void applyAttackImpulse(sf::Vector2f attackDir, float forceMultiplier = 1.0f);
    void triggerHitStop(float durationSeconds = 0.04f);
    void setEquippedWeightFactor(float weightFactor);
    float getEquippedWeightFactor() const;

    float getFootLDepth() const;
    float getFootRDepth() const;
    float getFootLRotIK() const;
    float getFootRRotIK() const;
    float getBodyDepth() const;

    ProceduralIK& getIK() { return mIK; }
    const ProceduralIK& getIK() const { return mIK; }
    void resetIK() { mIK.reset(); }

    // Métodos heredados/compatibilidad
    void reset();
    void setColor(sf::Color color);
    sf::FloatRect getBodyBounds() const;
    sf::FloatRect getGlobalBounds() const;
    
    float getFootBottomOffset() const {
        if (!mIsLoaded) return 0.f;
        auto it = mNodeMap.find("foot_l");
        if (it != mNodeMap.end()) {
            const auto& fn = mNodes[it->second];
            return fn.localBounds.size.y * fn.currentScale.y * mBaseScale.y;
        }
        return 0.f;
    }

    float getFootCurrentBottomY() const {
        if (!mIsLoaded || mNodes.empty()) return 0.f;
        
        float leftFootBottom = -999999.f;
        float rightFootBottom = -999999.f;

        auto itL = mNodeMap.find("foot_l");
        if (itL != mNodeMap.end()) {
            const auto& fn = mNodes[itL->second];
            for (int i = 0; i < 6; ++i) {
                if (fn.quad[i].position.y > leftFootBottom) {
                    leftFootBottom = fn.quad[i].position.y;
                }
            }
        }

        auto itR = mNodeMap.find("foot_r");
        if (itR != mNodeMap.end()) {
            const auto& fn = mNodes[itR->second];
            for (int i = 0; i < 6; ++i) {
                if (fn.quad[i].position.y > rightFootBottom) {
                    rightFootBottom = fn.quad[i].position.y;
                }
            }
        }

        if (leftFootBottom != -999999.f || rightFootBottom != -999999.f) {
            return (leftFootBottom > rightFootBottom) ? leftFootBottom : rightFootBottom;
        }

        // Fallback: bottom-most Y of any node
        float overallBottom = -999999.f;
        for (const auto& node : mNodes) {
            for (int i = 0; i < 6; ++i) {
                if (node.quad[i].position.y > overallBottom) {
                    overallBottom = node.quad[i].position.y;
                }
            }
        }

        if (overallBottom != -999999.f) {
            return overallBottom;
        }

        return 0.f;
    }

    // Footprints logic
    sf::Vector2f mLandedLeftPos;
    float mLandedLeftRot = 0.f;
    sf::Vector2f mLandedLeftScale;
    sf::Vector2f mLandedLeftOrigin;
    
    sf::Vector2f mLandedRightPos;
    float mLandedRightRot = 0.f;
    sf::Vector2f mLandedRightScale;
    sf::Vector2f mLandedRightOrigin;
    
    bool mLeftFootDown = false;
    bool mRightFootDown = false;

    const sf::Texture* mFootprintTexture = nullptr;
    sf::Image* mFootprintImage = nullptr;

private:
    std::unique_ptr<WeaponSprite> mWeapon;
    std::unique_ptr<WeaponSprite> mWeaponSecondary;
    sf::Vector2f mWeaponOffset = {0.f, 0.f};
    sf::Vector2f mSecondaryWeaponOffset = {0.f, 0.f};
    sf::Vector2f mBaseWeaponOffsetMain = {-25.f, 14.f};
    sf::Vector2f mBaseWeaponOffsetSec = {-25.f, 14.f};
    sf::Vector2f mBaseWeaponOffsetTwoHanded = {-25.f, 14.f};
    bool mWeaponIsTwoHanded = false;
    bool mWeaponIsShield = false;
    bool mSecondaryIsShield = false;
    bool mWeaponShieldOverHand = false;
    bool mSecondaryShieldOverHand = false;
    bool mEnableProceduralIK = true;
    bool mEnableTwoHandGripIK = false;
    bool mEnableIdleLayer = true;
    ArmorVisualInfo mArmorVisuals[12];

    void updateNodeQuad(SkeletonNode& node, sf::Vector2f basePos, int facingDir);

    bool mIsLoaded = false;
    
    std::vector<SkeletonNode> mNodes;
    std::unordered_map<std::string, size_t> mNodeMap;

    const sf::Texture* mAtlasTexture = nullptr;
    sf::Vector2f mBaseScale = {1.f, 1.f};
    float mGroundOffsetY = 0.f;
    sf::Vector2f mLastBasePos = {0.f, 0.f};
    int mLastFacingDir = 1;
    float mStride = 12.f;

    const AnimationClip* mBaseClip = nullptr;
    const AnimationClip* mActionClip = nullptr;
    const AnimationClip* mCurrentClip = nullptr; // Legacy compatibility alias
    const AnimationClip* mBaseIdleClip = nullptr;
    const AnimationClip* mShieldGuardClip = nullptr;
    std::string mShieldedBoneName = "";
    float mBaseTimer = 0.f;
    float mActionTimer = 0.f;
    float mActionSpeedMultiplier = 1.0f;
    float mAnimTimer = 0.f;
    float mIdleTimer = 0.f;
    float mGuardTimer = 0.f;
    
    std::vector<std::string> mFiredEvents;
    float mPrevTimer = 0.f;

    struct BlendState {
        sf::Vector2f pos;
        float rot = 0.f;
        sf::Vector2f scale = {1.f, 1.f};
    };
    std::unordered_map<std::string, BlendState> mBlendStartStates;
    float mBlendTimer = -1.f;
    float mBlendDuration = 0.15f;

    // [PROCEDURAL IK]
    ProceduralIK mIK;
    void solveTwoHandGripIK();
};
