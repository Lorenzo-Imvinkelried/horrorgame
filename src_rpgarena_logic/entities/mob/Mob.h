// Mob.h
#pragma once
#include "entities/Entity.h"
#include "core/engine/animation/Animation.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <optional>

class ResourceManager;
class CombatSystem;

enum class MobStance {
  Passive,
  Neutral,
  Violent
};

struct MobEquipmentOption {
  std::string itemId;
  float chance = 0.f;
};

struct MobSkillBlueprint {
  int id = 0;
  std::optional<float> cooldown;
  std::optional<int> manaCost;
  std::optional<int> damageFlat;
  std::optional<float> damagePercent;
  std::optional<int> range;
  std::optional<float> buffDuration;
  std::optional<float> castTime;
  std::optional<float> stunDuration;
};

struct MobBlueprint {
  std::string type;
  std::string name;
  std::vector<MobSkillBlueprint> skills;
  std::map<EquipmentSlot, std::vector<MobEquipmentOption>> equipmentPool;
  float speed = 0.f;
  sf::Vector2f scale = {1.f, 1.f};
  int level = 1, maxHp = 100, maxMp = 0, strength = 0, agility = 0,
      intelligence = 0, vitality = 0, attack = 0, defense = 0,
      armorPenetration = 0;
  int accuracy = 0, evasion = 0;
  float atkSpeed = 1.f, critChance = 0.f, critDamage = 1.5f;
  int xp = 0;
  float weightKg = 100.f;

  float armorPenetrationPercent = 0.f;
  float physicalDamageBonus = 0.f;
  float lifestealPercent = 0.f;

  float stunChance = 0.f;
  float stunDuration = 0.f;

  float slowMovePercent = 0.f;
  float slowMoveDuration = 0.f;
  float slowAttackPercent = 0.f;
  float slowAttackDuration = 0.f;

  float tenacityPercent = 0.f;
  float damageReductionPercent = 0.f;
  float critAvoidancePercent = 0.f;
  float antiArmorPenPercent = 0.f;
  int antiArmorPenFlat = 0;
  float manaStealPercent = 0.f;
  float xpBonusPercent = 0.f;
  float cooldownReductionPercent = 0.f;

  float bleedDurationFlat = 0.f;
  float bleedDurationPercent = 0.f;
  int bleedFlat = 0;
  float bleedPercent = 0.f;

  float doubleStrikeChance = 0.f;
  float tripleStrikeChance = 0.f;

  float enemyMaxHpDamagePercent = 0.f;

  float blockChance = 0.f;
  float blockValuePercent = 0.f;
  float thornsPercent = 0.f;
  float hpRegenPercent = 0.f;
  float mpRegenPercent = 0.f;

  float aoeRadius = 0.f;
  float aoeDamagePercent = 0.f;

  int executeDamagePercent = 0;
  int executeHealthThresholdPercent = 0;
  int trueDamagePercent = 0;
  float attackDelayFactor = 0.35f;
  float attackRange = 0.f;
  float leashRadius = 450.0f;
  float rangeViolent = 180.0f;
  float malice = 0.0f;
  MobStance stance = MobStance::Neutral;

  float groundOffsetY = 0.f;
  float baseAnimSpeed = 1.f;
  sf::Vector2f headOffset = {-10.f, -40.f};
  sf::Vector2f handLOffset = {-12.f, -5.f};
  sf::Vector2f handROffset = {12.f, 5.f};
  sf::Vector2f footLOffset = {-6.f, 25.f};
  sf::Vector2f footROffset = {6.f, 35.f};
};

class Mob : public Entity {
public:
  enum class State {
    Idle,
    Walking,
    Attacking,
    Dead,
    Fading,
    Spawning,
    Removable
  };

  Mob(sf::Vector2f startPos, const MobBlueprint &bp, ResourceManager &res,
      class ItemManager &itemMgr, CombatSystem *cs = nullptr,
      bool startWithFadeIn = false, int levelOverride = -1,
      class SkillManager *skillMgr = nullptr);
  virtual ~Mob();

  virtual std::shared_ptr<class Item>
  getWeapon(int slotIndex = 0) const override;
  virtual void equipWeapon(std::shared_ptr<class Item> item,
                           ResourceManager &res, int slotIndex = 0) override;
  virtual void unequipWeapon(int slotIndex = 0) override;
  virtual bool hasWeaponEquipped() const override;
  virtual bool hasTwoHandedWeaponEquipped() const override {
    return mHasTwoHandedWeapon;
  }

  virtual void equipItem(std::shared_ptr<class Item> item, EquipmentSlot slot,
                         ResourceManager &res) override;
  virtual void unequipItem(EquipmentSlot slot) override;
  virtual std::shared_ptr<class Item>
  getEquippedItem(EquipmentSlot slot) const override;
  virtual void emitWeaponGibs(class GoreSystem &gore, float floorY,
                              sf::Vector2f sourcePos = {0.f, 0.f},
                              float forceMultiplier = 1.0f) override;
  virtual std::vector<std::string> getNodeNames() const override {
    std::vector<std::string> names;
    for (const auto &node : mSkin.getNodes()) {
      names.push_back(node.name);
    }
    return names;
  }
  void recalculateStats();
  Animation::WeaponSpawnInfo getWeaponSpawnInfo(int slotIndex) const;
  virtual class Animation *getAnimation() override { return &mSkin; }
  virtual const class Animation *getAnimation() const override {
    return &mSkin;
  }

  void reset(sf::Vector2f startPos, bool startWithFadeIn,
             int levelOverride = -1);

  virtual void update(sf::Time dt) override;
  virtual sf::Vector2f getVelocity() const override { return mVelocity; }
  virtual void
  draw(sf::RenderTarget &target,
       sf::RenderStates states = sf::RenderStates::Default) override;
  virtual void
  drawLayer(sf::RenderTarget &target, int layer,
            sf::RenderStates states = sf::RenderStates::Default) override;
  virtual float getLayerSortingY(int layer) const override;
  void onWake() override;
  void onBuffsChanged() override;

  void setStatsFromBlueprint(const MobBlueprint &bp);
  void setTerrainDeform(const class TerrainDeformSystem *terrain) override {
    mTerrainDeform = terrain;
  }

  std::string getBlueprintName() const { return mBlueprintName; }
  sf::Vector2f getSpawnPoint() const { return mSpawnPoint; }

  virtual sf::Vector2f getGroundPosition() const override {
    return mPos + sf::Vector2f(0.f, mGroundOffsetY);
  }

  virtual float getGoreFloorY() const override {
    float currentFootBottomY = mSkin.getFootCurrentBottomY();
    if (currentFootBottomY != 0.f) {
      return currentFootBottomY;
    }
    float gOffset = (mGroundOffsetY != 0.f ? mGroundOffsetY : 30.f);
    return mPos.y + gOffset;
  }

  virtual std::string getName() const override { return mName; }
  bool debugAddStat(const std::string &statL, float amount,
                    bool isFixed = false) override;
  virtual int getLevel() const override { return mLevel; }
  virtual int getCurrentHp() const override { return mCurrentHp; }
  virtual int getMaxHp() const override { return mMaxHp; }
  virtual int getCurrentMp() const override { return mCurrentMp; }
  virtual int getMaxMp() const override { return mMaxMp; }

  virtual int getStrength() const override { return mStrength; }
  virtual int getAgility() const override { return mAgility; }
  virtual int getIntelligence() const override { return mIntelligence; }
  virtual int getVitality() const override { return mVitality; }

  virtual int getDefense() const override { return mDefense; }
  virtual int takeDamage(int damageAmount, Entity *attacker,
                         bool isCrit = false,
                         bool isTrueDamage = false) override;
  virtual bool isAlive() const override {
    return mCurrentState != State::Dead && mCurrentState != State::Fading;
  }
  virtual bool isRemovable() const override {
    return mCurrentState == State::Removable;
  }

  int getXp() const { return mXp; }
  float getXpBonusPercent() const { return mXpBonusPercent; }
  static std::function<void(Entity *player, int xp)> s_onExperienceCallback;

  virtual int getAccuracy() const override { return mAccuracy; }
  virtual int getEvasion() const override { return mEvasion; }

  virtual bool isBatchable() const override { return !hasWeaponEquipped(); }

  Animation &getSkin() { return mSkin; }
  const Animation &getSkin() const { return mSkin; }

  void getRenderData(std::vector<sf::Vertex> &vertices,
                     const sf::Texture *&texture) const override {
    mSkin.getRenderData(vertices, texture);
  }
  virtual bool castsShadow() const override;
  void getShadowRenderData(std::vector<sf::Vertex> &vertices,
                           const sf::Texture *&texture) const override;
  void getWeaponShadowRenderData(std::vector<sf::Vertex> &vertices,
                                 const sf::Texture *&texture,
                                 int slotIndex) const override;
  void getArmorShadowRenderData(std::vector<sf::Vertex> &vertices,
                                const sf::Texture *&texture,
                                int slotIndex) const override;

  virtual sf::FloatRect getGlobalBounds() const override {
    return mSkin.getGlobalBounds();
  }

  sf::Sprite *getSprite() { return nullptr; }
  const MobBlueprint &getBlueprint() const { return mBlueprint; }

  MobStance getStance() const { return mStance; }
  void setStance(MobStance stance) { mStance = stance; }
  float getRangeViolent() const { return mRangeViolent; }
  void setRangeViolent(float range) { mRangeViolent = range; }

  float getLeashRadius() const { return mLeashRadius; }
  float getLeashRadiusSq() const { return mLeashRadius * mLeashRadius; }
  virtual float getAttackRange() const override { return mAttackRange; }
  virtual int getAttack() const override { return mAttack; }
  virtual float getAtkSpeed() const override { return mAtkSpeed; }
  virtual float getCritChance() const override { return mCritChance; }
  virtual float getCritDamage() const override { return mCritDamage; }
  virtual int getArmorPenetration() const override { return mArmorPenetration; }

  virtual float getBlockChance() const override { return mBlockChance; }
  virtual float getBlockValuePercent() const override {
    return mBlockValuePercent;
  }
  virtual float getThornsPercent() const override { return mThornsPercent; }
  virtual float getHpRegenPercent() const override { return mHpRegenPercent; }
  virtual float getMpRegenPercent() const override { return mMpRegenPercent; }

  virtual bool isAggro() const override { return mAggroTarget != nullptr; }
  Entity *getAggroTarget() const { return mAggroTarget; }

  void die();

  virtual void returnToSpawn() override;
  virtual bool isReturningToSpawn() const override;
  virtual float getVisualHeight() const override;
  void onAggroedBy(Entity *attacker) override;

  virtual void heal(int amount) override;
  void restoreMana(int amount) override;
  void setCurrentHp(int hp) override;
  void setCurrentMp(int mp) override;
  virtual void startAttackAnimation(Entity *target = nullptr,
                                    float speedMultiplier = 1.0f) override;

  void resetAggro(Entity *requester);
  const class Skill *getPendingSkill() const { return mPendingSkill; }
  void cancelPendingSkill();

  virtual sf::Vector2f getLeftFootPosition() const override {
    return mSkin.getLastFacingDir() == -1 ? mSkin.getNodePosition("foot_r")
                                          : mSkin.getNodePosition("foot_l");
  }
  virtual sf::Vector2f getRightFootPosition() const override {
    return mSkin.getLastFacingDir() == -1 ? mSkin.getNodePosition("foot_l")
                                          : mSkin.getNodePosition("foot_r");
  }

  bool didLeftFootLand() const { return mSkin.mLeftFootDown; }
  bool didRightFootLand() const { return mSkin.mRightFootDown; }

  sf::Vector2f getLandedLeftFootPos() const { return mSkin.mLandedLeftPos; }
  float getLandedLeftFootRot() const { return mSkin.mLandedLeftRot; }
  sf::Vector2f getLandedLeftFootScale() const { return mSkin.mLandedLeftScale; }
  sf::Vector2f getLandedLeftFootOrigin() const {
    return mSkin.mLandedLeftOrigin;
  }

  sf::Vector2f getLandedRightFootPos() const { return mSkin.mLandedRightPos; }
  float getLandedRightFootRot() const { return mSkin.mLandedRightRot; }
  sf::Vector2f getLandedRightFootScale() const {
    return mSkin.mLandedRightScale;
  }
  sf::Vector2f getLandedRightFootOrigin() const {
    return mSkin.mLandedRightOrigin;
  }

  const sf::Texture *getFootprintTexture() const {
    return mSkin.mFootprintTexture;
  }
  const sf::Image *getFootprintImage() const { return mSkin.mFootprintImage; }

protected:
  MobBlueprint mBlueprint;
  virtual void updateAI(sf::Time dt);

  State mCurrentState;

  Animation mSkin;
  const AnimationClip *mIdleClip = nullptr;
  const AnimationClip *mWalkClip = nullptr;
  const AnimationClip *mAttackClip = nullptr;
  const AnimationClip *mIdleTwoHandedClip = nullptr;
  const AnimationClip *mWalkTwoHandedClip = nullptr;
  const AnimationClip *mAttackTwoHandedClip = nullptr;
  bool mHasTwoHandedWeapon = false;

  bool mMoving = false;

  sf::Clock mFadeClock;
  float mFadeDuration = 1.0f;

  sf::Clock mAttackClock;

  bool mDamagePending = false;
  float mDamageDelay = 0.f;
  sf::Clock mDamageTimer;
  Entity *mPendingTarget = nullptr;
  bool mPendingMiss = false;

  int mBaseAttack = 0;
  int mDirectMaxHp = 0;
  int mDirectMaxMp = 0;
  int mDirectAttack = 0;
  int mDirectDefense = 0;
  float mAttackRange = 50.f;
  float mLeashRadius = 450.f;
  float mRangeViolent = 180.f;
  MobStance mStance = MobStance::Neutral;

  float mGroundOffsetY = 0.f;
  float mBaseAnimSpeed = 1.f;
  float mBaseMovementSpeed = 20.f;

  virtual float getArmorPenetrationPercent() const override {
    return mArmorPenetrationPercent;
  }
  virtual float getPhysicalDamageBonus() const override {
    return mPhysicalDamageBonus;
  }
  virtual float getLifestealPercent() const override {
    return mLifestealPercent;
  }
  virtual float getManaStealPercent() const override {
    return mManaStealPercent;
  }
  virtual float getCooldownReductionPercent() const override {
    return mCooldownReductionPercent;
  }

  virtual float getStunChance() const override { return mStunChance; }
  virtual float getStunDuration() const override { return mStunDuration; }

  float getDistToPlayerSq() const { return mDistToPlayerSq; }

  virtual float getSlowMovePercent() const override { return mSlowMovePercent; }
  virtual float getSlowMoveDuration() const override {
    return mSlowMoveDuration;
  }
  virtual float getSlowAttackPercent() const override {
    return mSlowAttackPercent;
  }
  virtual float getSlowAttackDuration() const override {
    return mSlowAttackDuration;
  }

  virtual float getTenacityPercent() const override { return mTenacityPercent; }
  virtual float getDamageReductionPercent() const override {
    return mDamageReductionPercent;
  }
  virtual float getCritAvoidancePercent() const override {
    return mCritAvoidancePercent;
  }
  virtual float getAntiArmorPenPercent() const override {
    return mAntiArmorPenPercent;
  }
  virtual int getAntiArmorPenFlat() const override { return mAntiArmorPenFlat; }
  virtual float getBleedDurationFlat() const override {
    return mBleedDurationFlat;
  }
  virtual float getBleedDurationPercent() const override {
    return mBleedDurationPercent;
  }
  virtual int getBleedFlat() const override { return mBleedFlat; }
  virtual float getBleedPercent() const override { return mBleedPercent; }

  virtual float getDoubleStrikeChance() const override {
    return mDoubleStrikeChance;
  }
  virtual float getTripleStrikeChance() const override {
    return mTripleStrikeChance;
  }

  virtual float getEnemyMaxHpDamagePercent() const override {
    return mEnemyMaxHpDamagePercent;
  }

  virtual float getAoeRadius() const override { return mAoeRadius; }
  virtual float getAoeDamagePercent() const override {
    return mAoeDamagePercent;
  }

  virtual float getExecuteThresholdFactor() const override {
    return (float)mExecuteHealthThresholdPercent / 100.0f;
  }
  virtual float getExecuteDamageMultiplier() const override {
    return 1.0f + ((float)mExecuteDamagePercent / 100.0f);
  }
  virtual int getTrueDamagePercent() const override {
    return mTrueDamagePercent;
  }

  int getExecuteDamagePercent() const {
    return mExecuteDamagePercent;
  }
  int getExecuteHealthThresholdPercent() const {
    return mExecuteHealthThresholdPercent;
  }

  int mXp;
  std::string mName;
  std::string mBlueprintName;

  enum class AiDir { Left, Right };
  AiDir mAiDir = AiDir::Left;
  float mAiTimer = 0.f;

  enum class AiDecision {
    Idle,
    Patrolling,
    Chasing,
    Returning
  };
  AiDecision mAiDecision = AiDecision::Idle;
  sf::Vector2f mSpawnPoint;
  sf::Vector2f mPatrolTarget;

  Entity *mAggroTarget = nullptr;

  virtual sf::Vector2f
  getVisualPoint(const std::string &pointName) const override;

  sf::Clock mMobLeashTimer;

  sf::Clock mAiUpdateClock;
  sf::Vector2f mMoveDir{0.f, 0.f};
  static constexpr float AI_UPDATE_RATE = 0.2f;
  float mAiUpdateThreshold = AI_UPDATE_RATE;
  float mDistToPlayerSq = 0.f;
  bool mForceAiUpdate = false;

  CombatSystem *mCombatSystem = nullptr;

  const class TerrainDeformSystem *mTerrainDeform = nullptr;

  class ItemManager &mItemManager;
  class ResourceManager *mResourceManager = nullptr;
  std::shared_ptr<class Item> mEquippedWeapon;
  std::shared_ptr<class Item> mWeaponSecondary;
  std::shared_ptr<class Item> mEquipment[12] = {nullptr};
  float mHpRegenAccumulator = 0.f;
  float mMpRegenAccumulator = 0.f;
  sf::Vector2f mVelocity = {0.f, 0.f};
  std::vector<std::unique_ptr<class Skill>> mSkills;
  const class Skill *mPendingSkill = nullptr;
  void setupEquipment();
  void setupRandomWeapons();
  void setupRandomArmor();
  void updateWeaponVisuals(class ResourceManager &res);
  void updateArmorVisuals(class ResourceManager &res);
};
