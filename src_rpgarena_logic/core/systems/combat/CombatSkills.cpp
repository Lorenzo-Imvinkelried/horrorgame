#include "CombatSystem.h"
#include "core/skills/SkillManager.h"
#include "core/engine/animation/Animation.h"

void CombatSystem::requestSkillAttack(const Skill* skill, Entity* target) {
    if (!mPlayer || !skill || !target) return;
    if (!isEntityAllocated(target)) return;
    
    Skill* mutableSkill = const_cast<Skill*>(skill);
    mutableSkill->onCastStart(mPlayer, target, mParticleSystem);
    
    mCombatTimer.restart();
    
    float speedMult = skill ? skill->getSpeedMultiplier() : 1.0f;
    if (speedMult <= 0.f) speedMult = 1.0f;

    float attackDuration = (1.f / mPlayer->getAtkSpeed()) / speedMult; 
    if (skill->castTime > 0.f) {
        mDamageDelay = skill->castTime / speedMult;
        if (mPlayer) {
            mPlayer->setCasting(true);
        }
    } else {
        std::string clipName = "";
        if (mPlayer) {
            const AnimationClip* clip = mPlayer->getSkin().getAnimation()->getCurrentClip();
            if (clip) clipName = clip->name;
        }
        float delayFactor = Animation::getHitDelayFactor(clipName);
        mDamageDelay = attackDuration * delayFactor;
    }
    
    mDamagePending = true;
    mDamageTimer.restart();
    mPendingTarget = target;
    mPendingSkill = skill;
    mPendingMiss = false;
}

void CombatSystem::performSkillAttack(const Skill* skill, Entity* target) {
    if (!skill || !mPlayer || !target) return;
    if (mPlayer) {
        mPlayer->setCasting(false);
    }
    if (!target->isAlive()) return;
    
    if (target->isReturningToSpawn()) return;

    if (target != mPlayer) {
        mPlayer->addToAggro(target);
        target->onAggroedBy(mPlayer);

        if (mInCombat && mPlayer && !mPlayer->isInCombat()) {
             disengageCombat();
        }
        if (!mInCombat && mPlayer && mPlayer->isInCombat()) {
            engageCombat();
        }
    }

    Skill* mutableSkill = const_cast<Skill*>(skill);
    mutableSkill->onExecute(mPlayer, target, mParticleSystem);

    if (target != mPlayer || skill->targetType == "ENEMY") {
        mutableSkill->onHit(mPlayer, target, mFeedback, mParticleSystem, this);
        registerDebuff(target);
    }
}

bool CombatSystem::isCastingSkill() const {
    return mDamagePending && mPendingSkill && mPendingSkill->castTime > 0.f;
}

float CombatSystem::getCastProgress() const {
    if (!isCastingSkill() || mDamageDelay <= 0.f) return 0.f;
    return mDamageTimer.getElapsedTime().asSeconds() / mDamageDelay;
}
