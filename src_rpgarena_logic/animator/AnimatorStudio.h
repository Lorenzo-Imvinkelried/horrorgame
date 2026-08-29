#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../core/engine/animation/Animation.h"
#include "../core/engine/ResourceManager.h"
#include "../core/engine/AnimCore.h"

enum class StudioWeaponType {
    None,
    OneHandedSword,
    TwoHandedSword,
    DualWield,
    SwordAndShield,    // Espada en Mano Der, Escudo en Mano Izq
    ShieldAndSword,    // Escudo en Mano Der, Espada en Mano Izq
    ShieldRightOnly,   // Solo Escudo en Mano Der
    ShieldLeftOnly     // Solo Escudo en Mano Izq
};

struct MotionTrailPoint {
    sf::Vector2f pos;
    float time;
    float alpha;
};

class AnimatorStudio {
public:
    explicit AnimatorStudio(ResourceManager& res);
    ~AnimatorStudio() = default;

    void update(sf::Time dt);
    void drawWorld(sf::RenderTarget& target);
    void drawOverlays(sf::RenderTarget& target, float zoomFactor);

    // Playback control
    void togglePlay() { mIsPlaying = !mIsPlaying; }
    void play() { mIsPlaying = true; }
    void pause() { mIsPlaying = false; }
    bool isPlaying() const { return mIsPlaying; }
    
    void setTime(float t);
    float getTime() const { return mCurrentTime; }
    float getDuration() const;
    
    void stepForward(float dt = 1.0f / 60.0f);
    void stepBackward(float dt = 1.0f / 60.0f);
    
    void setSpeedMultiplier(float speed) { mSpeedMultiplier = speed; }
    float getSpeedMultiplier() const { return mSpeedMultiplier; }
    
    void toggleLoop() { mIsLooping = !mIsLooping; }
    bool isLooping() const { return mIsLooping; }
    
    void toggleFacingDir() { mFacingDir *= -1; }
    int getFacingDir() const { return mFacingDir; }
    void setFacingDir(int dir) { mFacingDir = (dir >= 0) ? 1 : -1; }

    // Clip management
    struct ClipInfo {
        std::string displayName;
        std::string filePath;
        bool isTwoHanded = false;
    };
    
    const std::vector<ClipInfo>& getAvailableClips() const { return mClips; }
    size_t getActiveClipIndex() const { return mActiveClipIndex; }
    void selectClip(size_t index);
    void selectClipByName(const std::string& name);
    AnimationClip* getActiveClip() { return mActiveClip; }
    const AnimationClip* getActiveClip() const { return mActiveClip; }
    
    void reloadAllClips();
    bool saveCurrentClip();
    bool createNewClip(const std::string& clipName, int baseClipIndex = -1, bool isTwoHanded = false, bool includeWeapon = true);
    bool deleteCurrentClip();

    // Interactive Bone Dragging & Keyframing
    std::string getHoveredNode(sf::Vector2f worldMousePos) const;
    void moveSelectedNode(sf::Vector2f worldDelta);
    void rotateSelectedNode(float deltaDegrees);
    void setNodeAbsoluteRotation(float absoluteDegrees);
    void insertKeyframeAtCurrentTime(const std::string& nodeName);
    void removeKeyframeAtCurrentTime(const std::string& nodeName);
    void clearNodeTracks(const std::string& nodeName);
    const std::vector<std::string>& getLayerOrder();
    void moveSelectedNodeLayerUp();
    void moveSelectedNodeLayerDown();
    void resetLayerOrder();
    std::vector<float> getKeyframeTimes(const std::string& nodeName) const;
    void selectKeyframe(float time);
    void jumpToPrevKeyframe();
    void jumpToNextKeyframe();
    bool isAtKeyframe(const std::string& nodeName, float* outExactTime = nullptr, float tolerance = 0.015f) const;
    float findNearestKeyframeTime(float queryTime, float maxDistance = 0.03f) const;

    // Loop Range Management (Intro / Channeling / Outro)
    void setLoopStartAtCurrentTime();
    void setLoopEndAtCurrentTime();
    void resetLoopRange();
    float getLoopStart() const;
    float getLoopEnd() const;

    // Weapon & entity configuration
    void setWeaponType(StudioWeaponType type);
    StudioWeaponType getWeaponType() const { return mWeaponType; }
    
    void setEntityType(const std::string& mobType);
    const std::string& getEntityType() const { return mEntityType; }

    // Diagnostic toggles
    bool toggleIK() {
        mEnableIK = !mEnableIK;
        mAnim.setEnableProceduralIK(mEnableIK);
        return mEnableIK;
    }
    bool isIKEnabled() const { return mEnableIK; }
    void setIKEnabled(bool enable) {
        mEnableIK = enable;
        mAnim.setEnableProceduralIK(mEnableIK);
    }

    bool toggleSpeedCurves() { mEnableSpeedCurves = !mEnableSpeedCurves; return mEnableSpeedCurves; }
    bool isSpeedCurvesEnabled() const { return mEnableSpeedCurves; }
    void setSpeedCurvesEnabled(bool enable) { mEnableSpeedCurves = enable; }

    bool toggleGripIK() {
        mEnableGripIK = !mEnableGripIK;
        mAnim.setEnableTwoHandGripIK(mEnableGripIK);
        return mEnableGripIK;
    }
    bool isGripIKEnabled() const { return mEnableGripIK; }
    void setGripIKEnabled(bool enable) {
        mEnableGripIK = enable;
        mAnim.setEnableTwoHandGripIK(mEnableGripIK);
    }

    bool toggleShowBones() { mShowBones = !mShowBones; return mShowBones; }
    bool isShowBones() const { return mShowBones; }
    void setShowBones(bool show) { mShowBones = show; }

    bool toggleShowMotionTrail() { mShowMotionTrail = !mShowMotionTrail; return mShowMotionTrail; }
    bool isShowMotionTrail() const { return mShowMotionTrail; }
    void setShowMotionTrail(bool show) { mShowMotionTrail = show; }

    bool toggleOnionSkin() { mShowOnionSkin = !mShowOnionSkin; return mShowOnionSkin; }
    bool isShowOnionSkin() const { return mShowOnionSkin; }
    void setShowOnionSkin(bool show) { mShowOnionSkin = show; }

    // Bone / Node Inspector
    const std::vector<SkeletonNode>& getNodes() const { return mAnim.getNodes(); }
    const std::string& getSelectedNode() const { return mSelectedNode; }
    void setSelectedNode(const std::string& name) { mSelectedNode = name; }
    
    // Offset Calibration
    sf::Vector2f getWeaponOffset() const { return mWeaponOffset; }
    void setWeaponOffset(sf::Vector2f offset);
    void adjustWeaponOffset(sf::Vector2f delta) { setWeaponOffset(mWeaponOffset + delta); }

    sf::Vector2f getWeapon2HOffset() const { return mWeapon2HOffset; }
    void setWeapon2HOffset(sf::Vector2f offset);
    void adjustWeapon2HOffset(sf::Vector2f delta) { setWeapon2HOffset(mWeapon2HOffset + delta); }

    Animation& getAnimation() { return mAnim; }
    const std::string& getStatusMessage() const { return mStatusMessage; }

private:
    void initClips();
    void loadCurrentEntity();
    void updateWeaponVisuals();
    void updateTrail();

private:
    ResourceManager& mRes;
    Animation mAnim;
    std::string mEntityType = "player";

    std::vector<ClipInfo> mClips;
    size_t mActiveClipIndex = 0;
    AnimationClip* mActiveClip = nullptr;

    bool mIsPlaying = false; // Start paused for precise editing
    float mCurrentTime = 0.f;
    float mSpeedMultiplier = 1.0f;
    bool mIsLooping = true;
    int mFacingDir = 1; // 1 = Right, -1 = Left

    StudioWeaponType mWeaponType = StudioWeaponType::None;
    sf::Vector2f mWeaponOffset = { 15.f, 2.f };
    sf::Vector2f mWeapon2HOffset = { 28.f, 5.f };

    // Diagnostic Toggles
    bool mEnableIK = false; // Default false in editor to show pure keyframes!
    bool mEnableSpeedCurves = false;
    bool mEnableGripIK = false;
    bool mShowBones = true;
    bool mShowMotionTrail = true;
    bool mShowOnionSkin = false;

    std::string mSelectedNode = "hand_r";
    std::vector<MotionTrailPoint> mTrailPoints;
    std::string mStatusMessage = "Listo. [Espacio] Play/Pausa  [Arrastrar Pieza] Mover  [Shift / Der] Rotar";
};
