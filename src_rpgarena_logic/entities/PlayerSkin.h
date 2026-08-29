// PlayerSkin.h
#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <memory>
#include <vector>
#include <string>
#include "core/engine/ResourceManager.h"
#include "core/engine/animation/Animation.h"
#include "core/items/WeaponSprite.h"
#include "core/items/Item.h"

class TerrainDeformSystem;

class PlayerSkin {
public:
    PlayerSkin() = default;

    bool loadParts(ResourceManager& res);
    bool morph(const std::string& mobType, ResourceManager& res);
    bool revert(ResourceManager& res);
    void update(sf::Time dt, bool isMoving, sf::Vector2f position, int facingDir, float currentSpeed = 340.f, const TerrainDeformSystem* terrain = nullptr);
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default);
    void drawLayer(sf::RenderTarget& target, int layer, sf::RenderStates states = sf::RenderStates::Default);
    void attack(float durationSeconds, bool useLeft, bool useRight, bool isTwoHanded = false);
    void shieldAttack(float durationSeconds, bool useLeft = true);

    sf::FloatRect getBodyBounds() const { 
        return mAnim.getBodyBounds();
    }

    bool isAttacking() const { return mIsAttacking; }
    bool isShieldAttacking() const { return mIsShieldAttacking; }
    void cancelAttackAnimation() { mIsAttacking = false; mIsShieldAttacking = false; mAttackTimer = 0.f; mAnim.clearAction(); }
    void playHealAnimation();
    const AnimationClip* getHealClip() const { return mHealClip; }
    
    // [PARTICLES] Exact foot positions
    sf::Vector2f getLeftFootPosition() const { 
        return mAnim.getNodePosition("foot_l");
    }
    sf::Vector2f getRightFootPosition() const { 
        return mAnim.getNodePosition("foot_r");
    }

    // [TERRAIN DEFORM] Detecta el frame exacto en que cada pie toca el suelo.
    bool didLeftFootLand()  const { return mLeftFootDown; }
    bool didRightFootLand() const { return mRightFootDown; }

    // [TERRAIN DEFORM] Transform exacto del sprite del pie en el momento del impacto.
    sf::Vector2f getLandedLeftPos()    const { return mLandedLeftPos; }
    float        getLandedLeftRot()    const { return mLandedLeftRot; }
    sf::Vector2f getLandedLeftScale()  const { return mLandedLeftScale; }
    sf::Vector2f getLandedLeftOrigin() const { return mLandedLeftOrigin; }

    sf::Vector2f getLandedRightPos()    const { return mLandedRightPos; }
    float        getLandedRightRot()    const { return mLandedRightRot; }
    sf::Vector2f getLandedRightScale()  const { return mLandedRightScale; }
    sf::Vector2f getLandedRightOrigin() const { return mLandedRightOrigin; }

    float getFootBottomOffset() const {
        return mAnim.getFootBottomOffset();
    }

    float getFootCurrentBottomY() const {
        return mAnim.getFootCurrentBottomY();
    }

    Animation* getAnimation() { return &mAnim; }
    const Animation* getAnimation() const { return &mAnim; }
    void resetIK() { mAnim.resetIK(); }

    const sf::Texture* getFootprintTexture() const { return mFootprintTexture; }
    const sf::Image*   getFootprintImage()   const { return mFootprintImage; }

    // [SKILL VISUALS]
    sf::Vector2f getRightHandPosition() const {
        return mAnim.getNodePosition("hand_r");
    }

    sf::Vector2f getLeftHandPosition() const {
        return mAnim.getNodePosition("hand_l");
    }

    // [GORE] Export all body part vertices for gib emission on death
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const;
    void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const;
    void getWeaponShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const;
    void getArmorShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const;
    
    // [GORE] Emit gibs for each body part individually (handles multi-texture)
    void emitGibs(class GoreSystem& gore, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f, sf::Vector2f initialVelocity = {0.f, 0.f}, const std::vector<std::shared_ptr<class Item>>& armorItems = {}, float deathSortY = 0.f) const;
    void emitWeaponGibs(class GoreSystem& gore, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f) const;

    // [FIX HEALTH BAR] Retorna el bounds completo (incluyendo cabeza)
    sf::FloatRect getGlobalBounds() const {
        sf::FloatRect r = mAnim.getNodeGlobalBounds("body");
        sf::FloatRect h = mAnim.getNodeGlobalBounds("head");
        if (r.size.x == 0.f) return h;
        if (h.size.x == 0.f) return r;
        
        float top = std::min(r.position.y, h.position.y);
        float left = std::min(r.position.x, h.position.x);
        float bottom = std::max(r.position.y + r.size.y, h.position.y + h.size.y);
        float right = std::max(r.position.x + r.size.x, h.position.x + h.size.x);
        return sf::FloatRect({left, top}, {right - left, bottom - top});
     }
 
     const Animation& getAnim() const { return mAnim; }

     void setWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                          const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                          float scale, sf::Vector2f offset, bool isTwoHanded = false, int fortificationLevel = 0, bool isShield = false, bool shieldOverHand = false);

    void setSecondaryWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                                   const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                                   float scale, sf::Vector2f offset, int fortificationLevel = 0, bool isShield = false, bool shieldOverHand = false);

    void setArmorVisuals(EquipmentSlot slot, const sf::Texture* tex, const sf::IntRect& rect, sf::Vector2f offset, float scale, int fortificationLevel = 0);

    void applyTerrainPhysics(float lDepth, float lRot, float rDepth, float rRot, float bodyDepth) {
        mAnim.applyTerrainPhysics(lDepth, lRot, rDepth, rRot, bodyDepth);
    }

    void setCustomRestOffsets(sf::Vector2f head, sf::Vector2f handL, sf::Vector2f handR, sf::Vector2f footL, sf::Vector2f footR) {
        mAnim.setCustomRestOffsets(head, handL, handR, footL, footR);
    }

    bool loadSkeleton(ResourceManager& res, const std::string& path) {
        return mAnim.loadSkeleton(res, path);
    }
    
    float getWorldStride() const {
        return mAnim.getWorldStride();
    }

    void setGuardState(bool active, bool shieldInLeftHand, bool shieldInRightHand) {
        mIsGuardActive = active;
        mHasShieldLeft = shieldInLeftHand;
        mHasShieldRight = shieldInRightHand;
    }

private:
    // Animation variables
    Animation mAnim;
    const AnimationClip* mIdleClip = nullptr;
    const AnimationClip* mWalkClip = nullptr;
    const AnimationClip* mIdleTwoHandedClip = nullptr; // [NEW]
    const AnimationClip* mWalkTwoHandedClip = nullptr; // [NEW]
    const AnimationClip* mAttackRClip = nullptr;
    const AnimationClip* mAttackLClip = nullptr;
    const AnimationClip* mAttackDualClip = nullptr;
    const AnimationClip* mAttackTwoHandedClip = nullptr; // [NEW]
    const AnimationClip* mShieldActiveLeftClip = nullptr;  // escudo_activo_d_i.json
    const AnimationClip* mShieldActiveRightClip = nullptr; // escudo_activo_D_d.json
    const AnimationClip* mHealClip = nullptr;              // heal.json

    bool mIsGuardActive = false;
    bool mIsShieldAttacking = false;
    bool mHasShieldLeft = false;
    bool mHasShieldRight = false;

    // [TERRAIN DEFORM] Edge-detection para pisada: true solo el frame del impacto
    bool mLeftFootDown  = false;
    bool mRightFootDown = false;
    bool mPrevLeftAirborne  = false; // true si el pie estaba levantado el frame anterior
    bool mPrevRightAirborne = false;

    // [TERRAIN DEFORM] Transform del sprite de cada pie, capturado en el frame del impacto.
    sf::Vector2f mLandedLeftPos,   mLandedRightPos;
    float        mLandedLeftRot  = 0.f, mLandedRightRot  = 0.f;
    sf::Vector2f mLandedLeftScale = {1.f,1.f}, mLandedRightScale = {1.f,1.f};
    sf::Vector2f mLandedLeftOrigin, mLandedRightOrigin;
    
    // [FOOTPRINT]
    const sf::Texture* mFootprintTexture = nullptr;
    const sf::Image*   mFootprintImage   = nullptr;
    sf::Image          mFootprintImageStorage; // To store the copy
    


    // Variables de ataque
    bool  mIsAttacking = false;
    bool  mAttackLeft = false;
    bool  mAttackRight = false;
    bool  mAttackTwoHanded = false; // [NEW]
    bool  mHasTwoHandedWeapon = false; // [NEW]
    float mAttackTimer = 0.f;
    float mAttackDuration = 0.f;

    // [GAIT PHASE TRACKING] Alternancia de pasos en taps cortos
    float mNextWalkPhase = 0.0f;
    bool  mWasMoving = false;
};