// Player.h
#pragma once
#include "entities/Entity.h"
#include "entities/mob/Mob.h"
#include "entities/PlayerSkin.h"
#include "core/items/Item.h"
#include "core/systems/TapSystem.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

class ResourceManager;
class InputManager;
class TerrainDeformSystem;

class Player : public Entity {
public:
  Player();
  virtual void startAttackAnimation(Entity *target = nullptr, float speedMultiplier = 1.0f) override;
  virtual void startShieldAttackAnimation(Entity *target = nullptr, float speedMultiplier = 1.0f) override;
  bool isAttacking() const { return mSkin.isAttacking(); }
  void cancelAttackAnimation() { mSkin.cancelAttackAnimation(); }
  void handleInput(const InputManager &input, sf::Time dt);
  virtual void update(sf::Time dt) override;
  virtual sf::Vector2f getVelocity() const override { return mVelocity; }
  virtual void
  draw(sf::RenderTarget &target,
       sf::RenderStates states = sf::RenderStates::Default) override;
  virtual void
  drawLayer(sf::RenderTarget &target, int layer,
            sf::RenderStates states = sf::RenderStates::Default) override;
  virtual float getLayerSortingY(int layer) const override;

  virtual bool isBatchable() const override { return false; }

  void setFollowTarget(Entity *target, float customRange = -1.f);

  void clearFollowTargetIfMatches(Entity *target) {
    if (mFollowTarget == target)
      mFollowTarget = nullptr;
  }
  bool isMovingManual() const {
    return mMovingManual;
  }
  bool isMoving() const { return mIsMoving; }
  void stopMovement() {
    mMovingManual = false;
    mIsMoving = false;
  }

  bool loadSkin(ResourceManager &res);
  void setWorldBounds(sf::FloatRect bounds);

  // Morph methods
  void morphInto(const std::string &mobType, float duration,
                 const MobBlueprint &bp);
  void revertMorph();
  bool isMorphed() const { return mMorphActive; }
  std::string getMorphType() const { return mMorphType; }

  // Aggro list
  void addToAggro(Entity *entity);
  void removeFromAggro(Entity *entity);
  bool isInCombat() const { return !mAggroList.empty(); }
  size_t getAggroCount() const { return mAggroList.size(); }

  void clearAggroList() { mAggroList.clear(); }
  void updateAggro(sf::Time dt);
  void setCombatState(bool inCombat) {}
  void setTerrainDeform(const TerrainDeformSystem *terrain) override {
    mTerrainDeform = terrain;
  }
  void resetIK() { mSkin.resetIK(); }

  // Title system
  virtual std::string getTitleName() const override;
  virtual sf::Color getTitleColor() const override;
  void setActiveTitle(const std::string &titleId) {
    mActiveTitleId = titleId;
    recalculateStats();
  }
  std::string getActiveTitleId() const { return mActiveTitleId; }

  // Getters de stats
  virtual std::string getName() const override { return "Player"; }
  virtual int getLevel() const override { return mLevel; }
  virtual int getCurrentHp() const override { return mCurrentHp; }
  virtual int getMaxHp() const override { return mMaxHp; }
  virtual int getCurrentMp() const override { return mCurrentMp; }
  virtual int getMaxMp() const override { return mMaxMp; }

  int getAttack() const { return mAttack; }

  int getStrength() const override { return std::max(0, mStrength + mBonusStrength + static_cast<int>(getStatModifier(Stat::STR))); }
  int getAgility() const override { return std::max(0, mAgility + mBonusAgility + static_cast<int>(getStatModifier(Stat::DEX))); }
  int getIntelligence() const override { return std::max(0, mIntelligence + mBonusIntelligence + static_cast<int>(getStatModifier(Stat::INT))); }
  int getVitality() const override { return std::max(0, mVitality + mBonusVitality + static_cast<int>(getStatModifier(Stat::VIT))); }

  float getAtkSpeed() const { return mAtkSpeed; }

  float getCritChance() const { return mCritChance; }
  float getCritDamage() const { return mCritDamage; }
  int getArmorPenetration() const { return mArmorPenetration; }

  float getArmorPenetrationPercent() const { return mArmorPenetrationPercent; }
  float getPhysicalDamageBonus() const { return mPhysicalDamageBonus; }
  float getLifestealPercent() const { return mLifestealPercent; }

  int getAccuracy() const { return mAccuracy + mBonusAccuracy; }
  int getEvasion() const { return mEvasion + mBonusEvasion; }

  float getDoubleStrikeChance() const { return mDoubleStrikeChance; }
  float getTripleStrikeChance() const { return mTripleStrikeChance; }

  float getEnemyMaxHpDamagePercent() const { return mEnemyMaxHpDamagePercent; }

  virtual float getBlockChance() const override {
    float base = mBlockChance;
    if (isGuardActive()) base += 25.0f;
    return base;
  }
  float getBlockValuePercent() const { return mBlockValuePercent; }
  float getThornsPercent() const { return mThornsPercent; }
  float getHpRegenPercent() const { return mHpRegenPercent; }
  float getMpRegenPercent() const { return mMpRegenPercent; }

  virtual float getBleedDurationFlat() const override { return mBleedDurationFlat; }
  virtual float getBleedDurationPercent() const override { return mBleedDurationPercent; }
  virtual int getBleedFlat() const override { return mBleedFlat; }
  virtual float getBleedPercent() const override { return mBleedPercent; }

  virtual float getStunChance() const override { return mStunChance; }
  virtual float getStunDuration() const override { return mStunDuration; }

  float getSlowMovePercent() const override { return mSlowMovePercent; }
  float getSlowMoveDuration() const override { return mSlowMoveDuration; }
  float getSlowAttackPercent() const override { return mSlowAttackPercent; }
  float getSlowAttackDuration() const override { return mSlowAttackDuration; }

  float getAoeRadius() const { return mAoeRadius; }
  float getAoeDamagePercent() const { return mAoeDamagePercent; }

  void heal(int amount) override;
  void restoreMana(int amount) override;
  void setCurrentHp(int hp) override;
  void setCurrentMp(int mp) override;
  bool wasHitRecently(float seconds) const;

  int getExecuteDamagePercent() const { return mExecuteDamagePercent; }
  int getExecuteHealthThresholdPercent() const { return mExecuteHealthThresholdPercent; }

  float getExecuteDamageMultiplier() const { return mExecuteDamageMultiplier; }
  float getExecuteThresholdFactor() const { return mExecuteThresholdFactor; }

  int getTrueDamagePercent() const { return mTrueDamagePercent; }

  virtual float getTenacityPercent() const override { return mTenacityPercent; }
  virtual float getDamageReductionPercent() const override {
    float base = mDamageReductionPercent;
    if (isGuardActive()) base += 30.0f;
    return base;
  }
  virtual float getCritAvoidancePercent() const override { return mCritAvoidancePercent; }
  virtual float getAntiArmorPenPercent() const override { return mAntiArmorPenPercent; }
  virtual int getAntiArmorPenFlat() const override { return mAntiArmorPenFlat; }
  virtual float getManaStealPercent() const override { return mManaStealPercent; }
  virtual float getXpBonusPercent() const override { return mXpBonusPercent; }
  virtual float getMalice() const override { return mMalice; }

  virtual float getMovementSpeed() const { return mSpeed; }

  virtual int getDefense() const override { return mDefense; }
  virtual int takeDamage(int damageAmount, Entity *attacker,
                         bool isCrit = false,
                         bool isTrueDamage = false) override;
  virtual bool isAlive() const override { return mCurrentHp > 0; }

  virtual sf::FloatRect getGlobalBounds() const override {
    return mSkin.getGlobalBounds();
  }
  virtual class Animation* getAnimation() override { return mSkin.getAnimation(); }
  virtual const class Animation* getAnimation() const override { return mSkin.getAnimation(); }
  void getShadowRenderData(std::vector<sf::Vertex> &vertices,
                           const sf::Texture *&texture) const override;
  void getWeaponShadowRenderData(std::vector<sf::Vertex> &vertices,
                                 const sf::Texture *&texture,
                                 int slotIndex) const override;
  void getArmorShadowRenderData(std::vector<sf::Vertex> &vertices,
                                const sf::Texture *&texture,
                                int slotIndex) const override;

  virtual float getVisualHeight() const override { return 42.f; }

  virtual sf::Vector2f getLeftFootPosition() const override {
    return mSkin.getLeftFootPosition();
  }
  virtual sf::Vector2f getRightFootPosition() const override {
    return mSkin.getRightFootPosition();
  }
  sf::Vector2f getRightHandPosition() const {
    return mSkin.getRightHandPosition();
  }
  sf::Vector2f getLeftHandPosition() const {
    return mSkin.getLeftHandPosition();
  }

  bool didLeftFootLand() const { return mSkin.didLeftFootLand(); }
  bool didRightFootLand() const { return mSkin.didRightFootLand(); }

  sf::Vector2f getLandedLeftFootPos() const { return mSkin.getLandedLeftPos(); }
  float getLandedLeftFootRot() const { return mSkin.getLandedLeftRot(); }
  sf::Vector2f getLandedLeftFootScale() const { return mSkin.getLandedLeftScale(); }
  sf::Vector2f getLandedLeftFootOrigin() const { return mSkin.getLandedLeftOrigin(); }

  sf::Vector2f getLandedRightFootPos() const { return mSkin.getLandedRightPos(); }
  float getLandedRightFootRot() const { return mSkin.getLandedRightRot(); }
  sf::Vector2f getLandedRightFootScale() const { return mSkin.getLandedRightScale(); }
  sf::Vector2f getLandedRightFootOrigin() const { return mSkin.getLandedRightOrigin(); }

  const sf::Texture *getFootprintTexture() const {
    return mSkin.getFootprintTexture();
  }
  const sf::Image *getFootprintImage() const {
    return mSkin.getFootprintImage();
  }

  sf::Vector2f getVisualPoint(const std::string &pointName) const override;

  virtual sf::Vector2f getGroundPosition() const override {
    return mPos + sf::Vector2f(0.f, cfg::YSorting::PLAYER);
  }

  virtual float getGoreFloorY() const override {
    float currentFootBottomY = mSkin.getFootCurrentBottomY();
    if (currentFootBottomY != 0.f) {
      return currentFootBottomY;
    }
    return getGroundPosition().y;
  }

  // Setters
  void setBaseStrength(int val) { mStrength = val; recalculateStats(); }
  void setBaseAgility(int val) { mAgility = val; recalculateStats(); }
  void setBaseIntelligence(int val) { mIntelligence = val; recalculateStats(); }
  void setBaseVitality(int val) { mVitality = val; recalculateStats(); }

  void setBaseMaxHp(int val) { mBaseMaxHp = val; recalculateStats(); }
  void setBaseMaxMp(int val) { mBaseMaxMp = val; recalculateStats(); }
  void setBaseAttack(int val) { mBaseAttack = val; recalculateStats(); }
  void setBaseDefense(int val) { mBaseDefense = val; recalculateStats(); }

  void setBaseAccuracy(int val) { mBaseAccuracy = val; recalculateStats(); }
  void setBaseEvasion(int val) { mBaseEvasion = val; recalculateStats(); }
  void setHpPerVit(int val) { mHpPerVit = val; recalculateStats(); }
  void setDefPerVit(int val) { mDefPerVit = val; recalculateStats(); }
  void setMpPerInt(int val) { mMpPerInt = val; recalculateStats(); }
  void setAtkPerStr(int val) { mAtkPerStr = val; recalculateStats(); }
  void setAtkSpeedPerAgi(float val) { mAtkSpeedPerAgi = val; recalculateStats(); }
  void setBaseAtkSpeed(float val) { mBaseAtkSpeed = val; recalculateStats(); }
  void setCritChance(float val) { mBaseCritChance = val; recalculateStats(); }
  void setCritDamage(float val) { mBaseCritDamage = val; recalculateStats(); }

  void setSpeed(float val) { mBaseSpeed = val; recalculateStats(); }
  void setArmorPenPercent(float val) { mArmorPenetrationPercentBase = val; recalculateStats(); }
  void setArmorPenFlat(int val) { mArmorPenetrationFlatBase = val; recalculateStats(); }
  void setPhysicalDmgBonus(float val) { mPhysicalDamageBonusBase = val; recalculateStats(); }

  void setLifestealPercent(float val) { mLifestealPercentBase = val; recalculateStats(); }
  void setDoubleStrikeChance(float val) { mDoubleStrikeChanceBase = val; recalculateStats(); }
  void setTripleStrikeChance(float val) { mTripleStrikeChanceBase = val; recalculateStats(); }
  void setEnemyMaxHpDamagePercent(float val) { mEnemyMaxHpDamagePercentBase = val; recalculateStats(); }

  void setBlockChance(float val) { mBlockChanceBase = val; recalculateStats(); }
  void setBlockValuePercent(float val) { mBlockValuePercentBase = val; recalculateStats(); }
  void setThornsPercent(float val) { mThornsPercentBase = val; recalculateStats(); }
  void setHpRegenPercent(float val) { mHpRegenPercentBase = val; recalculateStats(); }
  void setMpRegenPercent(float val) { mMpRegenPercentBase = val; recalculateStats(); }

  void setTenacityPercent(float val) { mTenacityPercentBase = val; recalculateStats(); }
  void setDamageReductionPercent(float val) { mDamageReductionPercentBase = val; recalculateStats(); }
  void setCritAvoidancePercent(float val) { mCritAvoidancePercentBase = val; recalculateStats(); }
  void setAntiArmorPenPercent(float val) { mAntiArmorPenPercentBase = val; recalculateStats(); }
  void setAntiArmorPenFlat(int val) { mAntiArmorPenFlatBase = val; recalculateStats(); }
  void setManaStealPercent(float val) { mManaStealPercentBase = val; recalculateStats(); }
  void setXpBonusPercent(float val) { mXpBonusPercentBase = val; recalculateStats(); }
  void setCooldownReductionPercent(float val) { mCooldownReductionPercentBase = val; recalculateStats(); }
  void setExecuteDamagePercent(int val) { mExecuteDamagePercentBase = val; recalculateStats(); }
  void setExecuteThresholdPercent(int val) { mExecuteHealthThresholdPercentBase = val; recalculateStats(); }
  void setTrueDamagePercent(int val) { mTrueDamagePercentBase = val; recalculateStats(); }

  void setAoeRadius(float val) { mAoeRadiusBase = val; recalculateStats(); }
  void setAoeDamagePercent(float val) { mAoeDamagePercentBase = val; recalculateStats(); }

  void setBleedFlat(int val) { mBleedFlatBase = val; recalculateStats(); }
  void setBleedPercent(float val) { mBleedPercentBase = val; recalculateStats(); }
  void setBleedDurationFlat(float val) { mBleedDurationFlatBase = val; recalculateStats(); }
  void setBleedDurationPercent(float val) { mBleedDurationPercentBase = val; recalculateStats(); }

  void setStunChance(float val) { mStunChanceBase = val; recalculateStats(); }
  void setStunDuration(float val) { mStunDurationBase = val; recalculateStats(); }

  void setSlowMovePercent(float val) { mSlowMovePercentBase = val; recalculateStats(); }
  void setSlowMoveDuration(float val) { mSlowMoveDurationBase = val; recalculateStats(); }
  void setSlowAttackPercent(float val) { mSlowAttackPercentBase = val; recalculateStats(); }
  void setSlowAttackDuration(float val) { mSlowAttackDurationBase = val; recalculateStats(); }

  void recalculateStats();
  void handleEnvironmentCollisions(const std::vector<sf::FloatRect> &obstacles);

  void onBuffsChanged() override;
  void debugBoostStats();
  bool debugAddStat(const std::string &statL, float amount, bool isFixed = false) override;

  int addExperience(int amount, bool applyModifiers = true);

  float getCurrentExp() const { return static_cast<float>(mCurrentExp); }
  float getNextLevelExp() const { return static_cast<float>(mNextLevelExp); }

  virtual void equipWeapon(std::shared_ptr<Item> item, ResourceManager &res,
                           int slotIndex = 0) override;
  virtual void unequipWeapon(int slotIndex = 0) override;

  virtual std::shared_ptr<Item> getWeapon(int slotIndex = 0) const override {
    if (slotIndex == 1)
      return getEquippedItem(EquipmentSlot::OffHand);
    return getEquippedItem(EquipmentSlot::MainHand);
  }

  virtual bool hasWeaponEquipped() const override {
    auto mh = getEquippedItem(EquipmentSlot::MainHand);
    auto oh = getEquippedItem(EquipmentSlot::OffHand);
    auto isActualWeapon = [](const std::shared_ptr<Item>& item) {
      if (!item) return false;
      if (item->stats.blockChance > 0.f || item->stats.blockValuePercent > 0.f) return false;
      std::string id = item->id;
      std::string name = item->name;
      std::string tex = item->texturePath;
      for (auto& c : id) c = std::tolower(c);
      for (auto& c : name) c = std::tolower(c);
      for (auto& c : tex) c = std::tolower(c);
      if (id.find("escudo") != std::string::npos || id.find("shield") != std::string::npos ||
          name.find("escudo") != std::string::npos || name.find("shield") != std::string::npos ||
          tex.find("escudo") != std::string::npos || tex.find("shield") != std::string::npos) {
        return false;
      }
      return true;
    };
    return isActualWeapon(mh) || isActualWeapon(oh);
  }

  virtual bool hasTwoHandedWeaponEquipped() const override {
    auto mh = getEquippedItem(EquipmentSlot::MainHand);
    auto oh = getEquippedItem(EquipmentSlot::OffHand);
    return (mh && mh->gripType == GripType::TwoHanded) ||
           (oh && oh->gripType == GripType::TwoHanded);
  }

  virtual bool hasShieldEquipped() const override {
    auto mh = getEquippedItem(EquipmentSlot::MainHand);
    auto oh = getEquippedItem(EquipmentSlot::OffHand);
    return (mh && mh->isShield()) || (oh && oh->isShield());
  }

  virtual void setGuardActive(bool active) override;

  virtual std::shared_ptr<Item> getEquippedItem(EquipmentSlot slot) const {
    int idx = static_cast<int>(slot);
    if (idx >= 0 && idx < static_cast<int>(EquipmentSlot::Count)) {
      return mEquipment[idx];
    }
    return nullptr;
  }

  bool isEquipped(const std::shared_ptr<Item>& item) const;
  virtual void equipItem(std::shared_ptr<Item> item, EquipmentSlot slot, ResourceManager &res);
  virtual void unequipItem(EquipmentSlot slot);
  virtual void emitWeaponGibs(class GoreSystem &gore, float floorY,
                              sf::Vector2f sourcePos = {0.f, 0.f},
                              float forceMultiplier = 1.0f) override;
  virtual std::vector<std::string> getNodeNames() const override {
    std::vector<std::string> names;
    for (const auto &node : mSkin.getAnim().getNodes()) {
      names.push_back(node.name);
    }
    return names;
  }

  void equipSkill(int slotIndex, int skillId);
  int getEquippedSkill(int slotIndex) const;

  float getAttackRange() const override;

  PlayerSkin &getSkin() { return mSkin; }
  const PlayerSkin &getSkin() const { return mSkin; }

  void setCharging(bool charging) override;

private:
  void updateFollowTarget(sf::Time dt);
  enum class Dir { Left, Right };
  void applyFrame();

  void checkLevelUp();

  std::map<int, int> mEquippedSkills;

  long long mCurrentExp = 0;
  long long mNextLevelExp = 100;

private:
  PlayerSkin mSkin;
  ResourceManager *mResourceManager = nullptr;
  void updateWeaponVisuals(ResourceManager &res);
  void updateArmorVisuals(ResourceManager &res);

  bool mMorphActive = false;
  std::string mMorphType = "";
  float mMorphTimer = 0.f;
  MobBlueprint mMorphBlueprint;

  float mAttackRange = 50.f;
  float mBonusAttackRange = 0.f;

  Entity *mFollowTarget = nullptr;
  float mFollowRangeOverride = -1.f;

  int mFacingDir = 1;

public:
  int getFacingDir() const override { return mFacingDir; }
  void setFacingDir(int dir) override {
    int prev = mFacingDir;
    mFacingDir = dir;
    if (prev != mFacingDir && mResourceManager && hasShieldEquipped()) {
      updateWeaponVisuals(*mResourceManager);
    }
  }

private:
  sf::FloatRect mWorldBounds;

  bool mMovingManual = false;
  bool mIsMoving = false;
  sf::Vector2f mVelocity = {0.f, 0.f};

  int mBonusStrength = 0;
  int mBonusAgility = 0;
  int mBonusIntelligence = 0;
  int mBonusVitality = 0;

  int mBonusAccuracy = 0;
  int mBonusEvasion = 0;

  float mHpRegenAccumulator = 0.0f;
  float mMpRegenAccumulator = 0.0f;

  std::string mActiveTitleId = "";

  sf::Clock mLastHitTimer;

  std::shared_ptr<Item> mEquipment[12] = { nullptr };

  int mBaseAccuracy, mBaseEvasion;
  int mHpPerVit, mDefPerVit, mMpPerInt, mAtkPerStr;
  float mAtkSpeedPerAgi, mBaseAtkSpeed;
  float mBaseCritChance;
  int mBaseMaxHp = 0;
  int mBaseMaxMp = 0;
  int mBaseAttack = 0;
  int mBaseDefense = 0;

  float mLifestealPercentBase;
  float mBlockChanceBase;
  float mTenacityPercentBase;
  float mDamageReductionPercentBase;
  float mCritAvoidancePercentBase;
  float mAntiArmorPenPercentBase;
  int mAntiArmorPenFlatBase;
  float mManaStealPercentBase;
  float mXpBonusPercentBase;
  float mCooldownReductionPercentBase = 0.f;
  int mExecuteDamagePercentBase;
  int mExecuteHealthThresholdPercentBase;
  int mTrueDamagePercentBase;

  int mBleedFlatBase;
  float mBleedPercentBase;
  float mBleedDurationFlatBase;
  float mBleedDurationPercentBase;
  float mStunChanceBase;
  float mStunDurationBase;
  float mSlowMovePercentBase;
  float mSlowMoveDurationBase;
  float mSlowAttackPercentBase;
  float mSlowAttackDurationBase;

  float mBaseSpeed;
  float mPhysicalDamageBonusBase;
  float mBaseCritDamage;
  int mArmorPenetrationFlatBase;
  float mArmorPenetrationPercentBase;
  float mDoubleStrikeChanceBase;
  float mTripleStrikeChanceBase;
  float mEnemyMaxHpDamagePercentBase;
  float mBlockValuePercentBase;
  float mThornsPercentBase;
  float mHpRegenPercentBase;
  float mMpRegenPercentBase;
  float mAoeRadiusBase;
  float mAoeDamagePercentBase;

  std::set<Entity *> mAggroList;
  const TerrainDeformSystem *mTerrainDeform = nullptr;

public:
  uint64_t getBronzeCoins() const { return mBronzeCoins; }
  void setBronzeCoins(uint64_t coins) { mBronzeCoins = coins; }
  void addBronzeCoins(int64_t amount) {
    if (amount < 0 && static_cast<uint64_t>(-amount) > mBronzeCoins) {
      mBronzeCoins = 0;
    } else {
      mBronzeCoins = static_cast<uint64_t>(static_cast<int64_t>(mBronzeCoins) + amount);
    }
  }
  bool removeBronzeCoins(uint64_t amount) {
    if (mBronzeCoins < amount) return false;
    mBronzeCoins -= amount;
    return true;
  }

  TapSystem& getTapSystem() { return mTapSystem; }
  const TapSystem& getTapSystem() const { return mTapSystem; }

private:
  uint64_t mBronzeCoins = 0;
  TapSystem mTapSystem;
};
