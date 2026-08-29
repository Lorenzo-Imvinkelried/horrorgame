#include "Mob.h"
#include "entities/player/Player.h"
#include "core/items/ItemManager.h"
#include "core/engine/ResourceManager.h"
#include "core/skills/SkillManager.h"
#include "core/skills/Skill.h"
#include "utils/Random.h"
#include "Config.h"

std::function<void(Entity* player, int xp)> Mob::s_onExperienceCallback = nullptr;

void Mob::reset(sf::Vector2f startPos, bool startWithFadeIn, int levelOverride) {
    // 1. Reset Positional Data
    mSpawnPoint = startPos;
    mPatrolTarget = startPos;
    setPosition(startPos);

    if (levelOverride != -1) {
        mLevel = levelOverride;
    } else {
        mLevel = mBlueprint.level;
    }
    
    // 2. Reset AI State
    mAiDecision = AiDecision::Idle;
    mAiTimer = Random::Float(cfg::Mob::IDLE_TIME_MIN, cfg::Mob::IDLE_TIME_MAX);
    mMobLeashTimer.restart();
    mAttackClock.restart();
    mAiUpdateClock.restart();
    mAiUpdateThreshold = Random::Float(0.0f, AI_UPDATE_RATE);
    
    mAggroTarget = nullptr;
    mMoveDir = {0.f, 0.f};
    
    mDirectMaxHp = 0;
    mDirectMaxMp = 0;
    mDirectAttack = 0;
    mDirectDefense = 0;
    mMoving = false;
    mVelocity = {0.f, 0.f};
    
    // 3. Reset Physics/Combat State
    mCurrentHp = mMaxHp; // Full HP
    mCurrentMp = mMaxMp;
    clearDebuffs();

    mDamagePending = false;
    mPendingTarget = nullptr;
    mPendingMiss = false;

    mFlashTimer = 0.f;
    mShakeTimer = 0.f;
    mVisualOffset = {0.f, 0.f};
    
    // 4. Reset Visual State
    mCurrentState = State::Idle;
    if (startWithFadeIn) {
        mCurrentState = State::Spawning;
    }
    
    mSkin.reset();
    mSkin.setScale(mBaseScale);
    mSkin.setColor(sf::Color::White);
    if (mIdleClip) {
        mSkin.playAnimation(mIdleClip);
    }
    
    if (mResourceManager) {
        setupEquipment();
    }
    recalculateStats();
    
    mCurrentHp = mMaxHp;
    mCurrentMp = mMaxMp;
    
    mSkin.update(sf::Time::Zero, false, getVisualPosition(), (mAiDir == AiDir::Left ? -1 : 1), 0.f);
}

Mob::Mob(sf::Vector2f startPos, const MobBlueprint& bp, ResourceManager& res, ItemManager& itemMgr, CombatSystem* cs, bool startWithFadeIn, int levelOverride, SkillManager* skillMgr)
    : mSpawnPoint(startPos),
      mPatrolTarget(startPos),
      mAiDecision(AiDecision::Idle),
      mCombatSystem(cs),
      mItemManager(itemMgr),
      mResourceManager(&res)
{
    mDirectMaxHp = 0;
    mDirectMaxMp = 0;
    mDirectAttack = 0;
    mDirectDefense = 0;
    mLevel = (levelOverride != -1 ? levelOverride : bp.level);
    setPosition(startPos);
    setStatsFromBlueprint(bp); 
    
    mCurrentState = State::Idle; 
    
    if (startWithFadeIn) {
        mCurrentState = State::Spawning;
    }

    mAiTimer = Random::Float(cfg::Mob::IDLE_TIME_MIN, cfg::Mob::IDLE_TIME_MAX);
    const SkeletonData* sk = res.getSkeleton("assets/textures/mobs/" + bp.type + "/esqueleto.json");
    std::vector<std::string> parts;
    if (sk && !sk->parts.empty()) {
        parts = sk->parts;
    } else {
        parts = {"foot_l", "hand_l", "body", "head", "foot_r", "hand_r"};
    }
    mSkin.loadDynamicParts(res, bp.type, parts);
    if (sk) {
        mSkin.loadSkeleton(res, "assets/textures/mobs/" + bp.type + "/esqueleto.json");
        mGroundOffsetY = mSkin.getGroundOffsetY();
    } else {
        mSkin.setCustomRestOffsets(bp.headOffset, bp.handLOffset, bp.handROffset, bp.footLOffset, bp.footROffset);
    }
    mSkin.setScale(mBaseScale);

    mIdleClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/idle.json");
    mWalkClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/walk.json");
    mAttackClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/attack.json");
    mIdleTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/idle_2h.json");
    mWalkTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/walk_2h.json");
    mAttackTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" + bp.type + "/attack_2h.json");

    if (mAttackClip) {
        const_cast<AnimationClip*>(mAttackClip)->isLoop = false;
    }
    if (mAttackTwoHandedClip) {
        const_cast<AnimationClip*>(mAttackTwoHandedClip)->isLoop = false;
    }

    if (mIdleClip) {
        mSkin.playAnimation(mIdleClip);
    }

    if (startWithFadeIn) {
        mSkin.setColor(sf::Color(255, 255, 255, 0));
    }

    mMobLeashTimer.restart();
    
    mAiUpdateThreshold = Random::Float(0.0f, AI_UPDATE_RATE);
    
    mSkin.reset();

    setupEquipment();

    mCurrentHp = mMaxHp;
    mCurrentMp = mMaxMp;

    if (skillMgr) {
        for (const auto& skillBp : bp.skills) {
            auto skillInstance = skillMgr->cloneSkill(skillBp.id);
            if (skillInstance) {
                if (skillBp.cooldown.has_value()) skillInstance->cooldown = skillBp.cooldown.value();
                if (skillBp.manaCost.has_value()) skillInstance->manaCost = skillBp.manaCost.value();
                if (skillBp.damageFlat.has_value()) skillInstance->damageFlat = skillBp.damageFlat.value();
                if (skillBp.damagePercent.has_value()) skillInstance->damagePercent = skillBp.damagePercent.value();
                if (skillBp.range.has_value()) skillInstance->range = skillBp.range.value();
                if (skillBp.buffDuration.has_value()) skillInstance->buffDuration = skillBp.buffDuration.value();
                if (skillBp.castTime.has_value()) skillInstance->castTime = skillBp.castTime.value();
                if (skillBp.stunDuration.has_value()) skillInstance->stunDuration = skillBp.stunDuration.value();
                
                mSkills.push_back(std::move(skillInstance));
            }
        }
    }
    
    mSkin.update(sf::Time::Zero, false, getVisualPosition(), 1, 0.f);
}

Mob::~Mob() {
    if (mAggroTarget) {
        if (auto* p = dynamic_cast<Player*>(mAggroTarget)) {
            p->removeFromAggro(this);
        }
    }
}

void Mob::setStatsFromBlueprint(const MobBlueprint& bp) {
    mBlueprint = bp;
    mBlueprintName = bp.type;
    
    recalculateStats();

    mCurrentHp = mMaxHp; 
    mCurrentMp = mMaxMp;

    mBaseMovementSpeed = bp.speed > 0.f ? bp.speed : 20.f;
    mSkin.setCustomRestOffsets(bp.headOffset, bp.handLOffset, bp.handROffset, bp.footLOffset, bp.footROffset);

    setSpeed(bp.speed);
    setScale(bp.scale);
    setWeightKg(bp.weightKg);
    mLeashRadius = bp.leashRadius > 0.f ? bp.leashRadius : 450.0f;
    mRangeViolent = bp.rangeViolent > 0.f ? bp.rangeViolent : 180.0f;
    mStance = bp.stance;
    mMalice = bp.malice;
    mXp = bp.xp;
}

void Mob::onWake() {
    if (mCurrentState == State::Idle || mCurrentState == State::Walking) {
        if (mAiTimer > 0.05f) {
            mAiTimer = 0.05f;
        }
    }
    mSkin.reset();
}

void Mob::heal(int amount) {
    if (!isAlive()) return;
    if (amount <= 0) return;

    mCurrentHp += amount;
    if (mCurrentHp > mMaxHp) mCurrentHp = mMaxHp;
    notifyStatsChanged();
}

void Mob::restoreMana(int amount) {
    if (!isAlive()) return;

    mCurrentMp += amount;
    if (mCurrentMp < 0) mCurrentMp = 0;
    if (mCurrentMp > mMaxMp) mCurrentMp = mMaxMp;
    notifyStatsChanged();
}

void Mob::setCurrentHp(int hp) {
    mCurrentHp = std::max(0, std::min(hp, getMaxHp()));
    notifyStatsChanged();
}

void Mob::setCurrentMp(int mp) {
    mCurrentMp = std::max(0, std::min(mp, getMaxMp()));
    notifyStatsChanged();
}

void Mob::onBuffsChanged() {
    recalculateStats();
    notifyStatsChanged();
}
