#include "Mob.h"
#include "entities/player/Player.h"
#include "core/skills/Skill.h"
#include "core/systems/combat/CombatSystem.h"
#include "utils/CombatCalculator.h"
#include "utils/Random.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

// [OPTIMIZATION] Fast Inverse Sqrt from Quake III Arena
float Q_rsqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = * ( long * ) &y;
	i = 0x5f3759df - ( i >> 1 );
	y = * ( float * ) &i;
	y = y * ( threehalfs - ( x2 * y * y ) );

	return y;
}

void Mob::update(sf::Time dt) {
    updateBuffs(dt);
    updateCooldowns(dt);

    // [HP/MP REGEN LOGIC]
    bool inCombat = (mAggroTarget != nullptr);
    if (!inCombat && mHpRegenPercent > 0.f && mCurrentHp < mMaxHp && isAlive()) {
        float regenPerSec = (float)mMaxHp * (mHpRegenPercent / 100.f);
        mHpRegenAccumulator += regenPerSec * dt.asSeconds();
        if (mHpRegenAccumulator >= 1.f) {
            int healAmount = static_cast<int>(mHpRegenAccumulator);
            heal(healAmount);
            mHpRegenAccumulator -= healAmount;
        }
    }

    if (!inCombat && mMpRegenPercent > 0.f && mCurrentMp < mMaxMp && isAlive()) {
        float regenPerSec = (float)mMaxMp * (mMpRegenPercent / 100.f);
        mMpRegenAccumulator += regenPerSec * dt.asSeconds();
        if (mMpRegenAccumulator >= 1.f) {
            int restoreAmount = static_cast<int>(mMpRegenAccumulator);
            restoreMana(restoreAmount);
            mMpRegenAccumulator -= restoreAmount;
        }
    }

    // --- Attack Delay Logic ---
    if (mDamagePending) {
        if (!mPendingTarget || !mPendingTarget->isAlive() || mPendingTarget->isReturningToSpawn()) {
            mDamagePending = false;
            mPendingTarget = nullptr;
            mPendingSkill = nullptr;
            setCasting(false);
        } else {
            sf::Vector2f pPos = getPosition();
            sf::Vector2f tPos = mPendingTarget->getPosition();
            float distSq = (pPos.x - tPos.x)*(pPos.x - tPos.x) + (pPos.y - tPos.y)*(pPos.y - tPos.y);
            float baseRange = getAttackRange();
            float rangeTolerance = baseRange + 15.f;

            if (distSq > rangeTolerance * rangeTolerance) {
                mDamagePending = false;
                mPendingTarget = nullptr;
                mPendingSkill = nullptr;
                setCasting(false);
                mCurrentState = State::Idle;
                const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
                if (idleClipToUse) {
                    mSkin.playAnimation(idleClipToUse);
                }
                mForceAiUpdate = true;
            }
            else if (mDamageTimer.getElapsedTime().asSeconds() >= mDamageDelay) {
                if (mPendingSkill) {
                    bool hit = CombatCalculator::tryToHit(this, mPendingTarget);
                    if (!hit) {
                        if (mCombatSystem) mCombatSystem->createMissEffect(mPendingTarget, this);
                    } else {
                        Skill* mutableSkill = const_cast<Skill*>(mPendingSkill);
                        if (mCombatSystem) {
                            mutableSkill->onHit(this, mPendingTarget, mCombatSystem->getFeedback(), mCombatSystem->getParticleSystem(), mCombatSystem);
                            mCombatSystem->registerDebuff(mPendingTarget);
                        }
                    }
                    mPendingSkill = nullptr;
                } else {
                    bool hit = CombatCalculator::tryToHit(this, mPendingTarget);
                    if (!hit) {
                        if (mCombatSystem) mCombatSystem->createMissEffect(mPendingTarget, this);
                    } else {
                        if (mCombatSystem) mCombatSystem->performAttack(this, mPendingTarget);
                    }
                }
                mDamagePending = false;
                mPendingTarget = nullptr;
                setCasting(false);
            }
        }
    }

    // [STUN VISUALS]
    if (isStunned()) {
        mMoving = false;
        mVelocity = {0.f, 0.f};
        mDamagePending = false;
        mPendingTarget = nullptr;
        mPendingSkill = nullptr;
        setCasting(false);
        if (mCurrentState != State::Idle && mCurrentState != State::Dead && mCurrentState != State::Fading && mCurrentState != State::Removable) {
            mCurrentState = State::Idle;
            const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
            if (idleClipToUse) {
                mSkin.playAnimation(idleClipToUse);
            }
        }
    }

    // --- Máquina de Estados ---
    switch (mCurrentState) {
    
    case State::Spawning:
        {
             if (mSkin.getBodyBounds().size.x > 0) {
                mCurrentState = State::Idle;
                onWake();
             }
        }
        break;
        
    case State::Idle:
    case State::Walking:
        {
            updateAI(dt);
            
            if (mCurrentState == State::Attacking) {
                break;
            }
            
            State targetState = mMoving ? State::Walking : State::Idle;
            
            if (mCurrentState != targetState) {
                mCurrentState = targetState;
            }
            
            const AnimationClip* walkClipToUse = (mHasTwoHandedWeapon && mWalkTwoHandedClip) ? mWalkTwoHandedClip : mWalkClip;
            const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
            
            if (mCurrentState == State::Walking) {
                if (walkClipToUse && mSkin.getCurrentClip() != walkClipToUse) {
                    mSkin.playAnimation(walkClipToUse);
                }
            } else if (mCurrentState == State::Idle) {
                if (idleClipToUse && mSkin.getCurrentClip() != idleClipToUse) {
                    mSkin.playAnimation(idleClipToUse);
                }
            }
            
            float speedMult = getSpeedMultiplier();
            if (mCurrentState == State::Walking) {
                const AnimationClip* walkClipToUse = (mHasTwoHandedWeapon && mWalkTwoHandedClip) ? mWalkTwoHandedClip : mWalkClip;
                if (walkClipToUse) {
                    float worldStride = mSkin.getWorldStride();
                    if (worldStride > 0.1f) {
                        float currentWorldSpeed = isCharging() ? 1000.f : (mSpeed * speedMult);
                        speedMult = (currentWorldSpeed * walkClipToUse->duration) / (4.0f * worldStride);
                    } else {
                        float baseSpeed = mBaseMovementSpeed > 0.f ? mBaseMovementSpeed : 20.f;
                        float currentWorldSpeed = isCharging() ? 1000.f : (mSpeed * speedMult);
                        speedMult = currentWorldSpeed / baseSpeed;
                    }
                } else {
                    float baseSpeed = mBaseMovementSpeed > 0.f ? mBaseMovementSpeed : 20.f;
                    float currentWorldSpeed = isCharging() ? 1000.f : (mSpeed * speedMult);
                    speedMult = currentWorldSpeed / baseSpeed;
                }
            } else {
                speedMult = 1.0f;
            }

            if (isVisible()) {
                mSkin.update(dt, mMoving, getVisualPosition(), (mAiDir == AiDir::Left ? -1 : 1), speedMult, mTerrainDeform);
            } else {
                mSkin.applyTerrainPhysics(0.f, 0.f, 0.f, 0.f, 0.f);
                mSkin.update(sf::Time::Zero, mMoving, getVisualPosition(), (mAiDir == AiDir::Left ? -1 : 1), 0.f);
            }
        }
        break;

    case State::Attacking:
        if (isVisible()) {
            float speedMult = (mAtkSpeed * getAttackSpeedMultiplier()) / 0.6f;
            const AnimationClip* currentClip = mSkin.getCurrentClip();
            if (currentClip) {
                speedMult = (currentClip->duration * mAtkSpeed * getAttackSpeedMultiplier()) / 0.6f;
            }
            mSkin.update(dt, false, getVisualPosition(), (mAiDir == AiDir::Left ? -1 : 1), speedMult);
        } else {
            mSkin.update(sf::Time::Zero, false, getVisualPosition(), (mAiDir == AiDir::Left ? -1 : 1), 0.f);
        }
        
        if (!mDamagePending && mSkin.getCurrentClip() && mSkin.getCurrentClip()->isLoop) {
            mCurrentState = State::Idle;
        }
        
        if (mSkin.isFinished()) {
            mForceAiUpdate = true;
            updateAI(dt);
            
            if (mCurrentState == State::Attacking && !mSkin.isFinished()) {
            } else if (mCurrentState == State::Attacking && mSkin.isFinished()) {
                if (mMoving) {
                    mCurrentState = State::Walking;
                    const AnimationClip* walkClipToUse = (mHasTwoHandedWeapon && mWalkTwoHandedClip) ? mWalkTwoHandedClip : mWalkClip;
                    if (walkClipToUse) {
                        mSkin.playAnimation(walkClipToUse);
                    }
                } else {
                    float speedMult = getAttackSpeedMultiplier();
                    float effectiveAtkSpeed = mAtkSpeed * speedMult;
                    float timeBetweenAttacks = 1.f / std::max(0.1f, effectiveAtkSpeed);
                    float elapsed = mAttackClock.getElapsedTime().asSeconds();
                    float remainingCooldown = timeBetweenAttacks - elapsed;

                    if (remainingCooldown > 0.15f) {
                        mCurrentState = State::Idle;
                        const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
                        if (idleClipToUse) {
                            mSkin.playAnimation(idleClipToUse);
                        }
                    }
                }
            } else {
                State targetState = mMoving ? State::Walking : State::Idle;
                mCurrentState = targetState;
                if (mCurrentState == State::Walking) {
                    const AnimationClip* walkClipToUse = (mHasTwoHandedWeapon && mWalkTwoHandedClip) ? mWalkTwoHandedClip : mWalkClip;
                    if (walkClipToUse) {
                        mSkin.playAnimation(walkClipToUse);
                    }
                } else if (mCurrentState == State::Idle) {
                    const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
                    if (idleClipToUse) {
                        mSkin.playAnimation(idleClipToUse);
                    }
                }
            }
        }
        break;

    case State::Dead:
        mCurrentState = State::Fading;
        mFadeClock.restart();
        break;

    case State::Fading:
    { 
        float elapsed = mFadeClock.getElapsedTime().asSeconds();
        float fadePercent = std::max(0.f, 1.f - (elapsed / mFadeDuration));
        
        mSkin.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * fadePercent)));

        if (fadePercent == 0.f) {
            mCurrentState = State::Removable;
        }
        break;
    } 
    
    case State::Removable:
        break;
    }

    updateVisuals(dt);

    if (mCurrentState != State::Fading && mCurrentState != State::Spawning) {
         if (getDebuffState().isSlowedMove()) {
             mSkin.setColor(sf::Color(150, 255, 255));
         } else {
             mSkin.setColor(sf::Color::White);
         }
    }
}

void Mob::updateAI(sf::Time dt) {
    float s = dt.asSeconds();

    if (mCurrentState == State::Dead || mCurrentState == State::Fading || mCurrentState == State::Removable) {
        mMoving = false;
        return;
    }
    if (mCurrentState == State::Attacking && !mSkin.isFinished()) {
        mMoving = false;
        return;
    }
    
    if (isStunned()) {
        mMoving = false;
        return;
    }

    bool bypassThrottle = mForceAiUpdate || isCharging();
    mForceAiUpdate = false;

    if (!bypassThrottle && mAiUpdateClock.getElapsedTime().asSeconds() < mAiUpdateThreshold) {
        if (mMoving) {
            float speedMult = getSpeedMultiplier();
            float currentSpeed = isCharging() ? 1000.f : mSpeed;
            mPos += mMoveDir * (currentSpeed * speedMult) * s;
            mVelocity = mMoveDir * (currentSpeed * speedMult);
        } else {
            mVelocity = {0.f, 0.f};
        }
        return;
    }
    
    mAiUpdateClock.restart();

    sf::Vector2f distToSpawn = mPos - mSpawnPoint;
    float distToSpawnSq = (distToSpawn.x * distToSpawn.x) + (distToSpawn.y * distToSpawn.y);

    if (mAiDecision == AiDecision::Chasing) {
        if (distToSpawnSq > getLeashRadiusSq()) {
            returnToSpawn();
        }
    } else if (mAiDecision == AiDecision::Patrolling) {
        if (distToSpawnSq > cfg::Mob::PATROL_RADIUS_SQ) {
            mPatrolTarget = mSpawnPoint; 
        }
    }

    mMoving = false;
    if (isCharging()) {
        if (!mPendingTarget || !mPendingTarget->isAlive()) {
            setCharging(false);
            mPendingSkill = nullptr;
            mPendingTarget = nullptr;
            return;
        }

        sf::Vector2f diff = mPendingTarget->getPosition() - mPos;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        
        if (diff.x > 0.f) mAiDir = AiDir::Right;
        else if (diff.x < 0.f) mAiDir = AiDir::Left;

        if (mPendingSkill) {
            const_cast<Skill*>(mPendingSkill)->updateChargeVisuals(this, mCombatSystem ? mCombatSystem->getParticleSystem() : nullptr);
        }

        const float MELEE_RANGE = 30.f;
        if (distSq <= MELEE_RANGE * MELEE_RANGE) {
            mMoving = false;
            mVelocity = {0.f, 0.f};

            if (mPendingSkill) {
                restoreMana(-mPendingSkill->manaCost);
                float cdrPct = std::clamp(getCooldownReductionPercent(), 0.f, 100.f);
                setSkillCooldown(mPendingSkill->id, mPendingSkill->cooldown * (1.f - cdrPct / 100.f), mPendingSkill->cooldown);

                Skill* mutableSkill = const_cast<Skill*>(mPendingSkill);
                if (mCombatSystem) {
                    mutableSkill->onHit(this, mPendingTarget, mCombatSystem->getFeedback(), mCombatSystem->getParticleSystem(), mCombatSystem);
                    mCombatSystem->registerDebuff(mPendingTarget);
                }
            }

            setCharging(false);
            mPendingSkill = nullptr;
            mPendingTarget = nullptr;
            mAttackClock.restart();
        } else {
            float invDist = Q_rsqrt(distSq);
            mMoveDir = { diff.x * invDist, diff.y * invDist };
            mMoving = true;
            
            float speedMult = getSpeedMultiplier();
            float currentSpeed = 1000.f;
            mPos += mMoveDir * (currentSpeed * speedMult) * s;
            mVelocity = mMoveDir * (currentSpeed * speedMult);
        }

        return;
    }

    switch (mAiDecision) {
        
    case AiDecision::Idle:
    case AiDecision::Patrolling:
        {
            if (mAiDecision == AiDecision::Idle) {
                mAiTimer -= s;
                if (mAiTimer <= 0.f) {
                    float angle = Random::Float(0.f, 360.f) * 3.14159f / 180.f;
                    float dist = Random::Float(0.f, (float)cfg::Mob::PATROL_RADIUS);
                    mPatrolTarget = mSpawnPoint + sf::Vector2f(std::cos(angle) * dist, std::sin(angle) * dist);
                    mAiDecision = AiDecision::Patrolling;
                }
            } else {
                sf::Vector2f diff = mPatrolTarget - mPos;
                if ((diff.x * diff.x) + (diff.y * diff.y) < 100.f) {
                    mAiDecision = AiDecision::Idle;
                    mAiTimer = Random::Float(cfg::Mob::IDLE_TIME_MIN, cfg::Mob::IDLE_TIME_MAX);
                } else {
                    float dist = std::sqrt((diff.x * diff.x) + (diff.y * diff.y));
                    mMoveDir = diff / dist;
                    mMoving = true;
                    if (diff.x > 0.f) mAiDir = AiDir::Right;
                    else if (diff.x < 0.f) mAiDir = AiDir::Left;
                }
            }
            break;
        }
    
    case AiDecision::Chasing:
        {
            if (!mAggroTarget || !mAggroTarget->isAlive()) {
                returnToSpawn();
                break;
            }

            sf::Vector2f distToSpawnVec = mPos - mSpawnPoint;
            float distToSpawnSq = distToSpawnVec.x * distToSpawnVec.x + distToSpawnVec.y * distToSpawnVec.y;
            
            if (distToSpawnSq > getLeashRadiusSq()) {
                if (mMobLeashTimer.getElapsedTime().asSeconds() > cfg::Mob::LEASH_TIME) {
                    returnToSpawn();
                    break;
                }
            } else {
                mMobLeashTimer.restart();
            }

            sf::Vector2f targetPos = mAggroTarget->getPosition();
            sf::Vector2f diff      = targetPos - mPos;
            float distSq           = diff.x * diff.x + diff.y * diff.y;
            mDistToPlayerSq        = distSq;

            if (distSq < 10000.f) mAiUpdateThreshold = 0.1f;
            else if (distSq < 160000.f) mAiUpdateThreshold = 0.2f;
            else mAiUpdateThreshold = 0.5f;

            const float ATTACK_RANGE     = getAttackRange();
            const float ATTACK_RANGE_SQ  = ATTACK_RANGE * ATTACK_RANGE;

            if (diff.x > 0.f)      mAiDir = AiDir::Right;
            else if (diff.x < 0.f) mAiDir = AiDir::Left;

            float speedMult = getAttackSpeedMultiplier();
            float effectiveAtkSpeed = mAtkSpeed * speedMult;
            float timeBetweenAttacks = 1.f / std::max(0.1f, effectiveAtkSpeed);

            Skill* skillToCast = nullptr;
            if (mAttackClock.getElapsedTime().asSeconds() >= timeBetweenAttacks) {
                for (auto& s : mSkills) {
                    if (isSkillReady(s->id) && mCurrentMp >= s->manaCost) {
                        float rangeToUse = s->range > 0 ? (float)s->range : getAttackRange();
                        if (mDistToPlayerSq <= rangeToUse * rangeToUse) {
                            skillToCast = s.get();
                            break;
                        }
                    }
                }
            }

            if (skillToCast) {
                mMoving = false;
                
                restoreMana(-skillToCast->manaCost);
                float cdrPct = std::clamp(getCooldownReductionPercent(), 0.f, 100.f);
                setSkillCooldown(skillToCast->id, skillToCast->cooldown * (1.f - cdrPct / 100.f), skillToCast->cooldown);

                ParticleSystem* ps = mCombatSystem ? mCombatSystem->getParticleSystem() : nullptr;

                if (skillToCast->type == SkillType::Buff) {
                    skillToCast->onCastStart(this, mAggroTarget ? mAggroTarget : this, ps);
                    skillToCast->onExecute(this, mAggroTarget ? mAggroTarget : this, ps);
                    recalculateStats();
                    notifyStatsChanged();
                    mAttackClock.restart();
                } else if (skillToCast->castTime == 0.f && skillToCast->id == 4) {
                    mPendingSkill = skillToCast;
                    mPendingTarget = mAggroTarget;
                    skillToCast->onQueue(this, ps);
                    mAttackClock.restart();
                } else {
                    skillToCast->onCastStart(this, mAggroTarget, ps);
                    mPendingTarget = mAggroTarget;
                    mPendingSkill = skillToCast;
                    mDamagePending = true;
                    mPendingMiss = false;
                    
                    float attackDuration = 1.f / mAtkSpeed;
                    if (skillToCast->castTime > 0.f) {
                        mDamageDelay = skillToCast->castTime;
                        setCasting(true);
                    } else {
                        mDamageDelay = attackDuration * mBlueprint.attackDelayFactor;
                    }
                    
                    mDamageTimer.restart();
                    mAttackClock.restart();
                }

                if (mCurrentState != State::Attacking) {
                    mCurrentState = State::Attacking;
                }
            } else if (distSq > ATTACK_RANGE_SQ) {
                if (mAiUpdateClock.getElapsedTime().asSeconds() < 0.1f) {
                     if (auto* p = dynamic_cast<Player*>(mAggroTarget)) {
                         if (isAlive()) p->addToAggro(this);
                     }
                }

                float stopDist = ATTACK_RANGE * 0.8f; 
                float stopDistSq = stopDist * stopDist;

                if (distSq > stopDistSq) {
                    float invDist = Q_rsqrt(distSq);
                    sf::Vector2f dir = { diff.x * invDist, diff.y * invDist };
                    
                    mMoveDir = dir;
                    mMoving = true;
                }
            } else {
                mMoving = false;

                if (mAttackClock.getElapsedTime().asSeconds() >= timeBetweenAttacks) {
                    if (mAggroTarget) {
                        float dx = mAggroTarget->getPosition().x - mPos.x;
                        if (std::abs(dx) > 1.f) {
                            mAiDir = (dx > 0) ? AiDir::Right : AiDir::Left;
                        }
                    }
                    float attackDuration = 1.f / std::max(0.1f, effectiveAtkSpeed);
                    const AnimationClip* attackClipToUse = (mHasTwoHandedWeapon && mAttackTwoHandedClip) ? mAttackTwoHandedClip : mAttackClip;
                    if (attackClipToUse) {
                        mSkin.playAnimation(attackClipToUse);
                    } else {
                        mSkin.attack(attackDuration * 0.9f, false, true);
                    }
                    
                    if (mCurrentState != State::Attacking) {
                        mCurrentState = State::Attacking;
                    }

                    mPendingTarget = mAggroTarget;
                    mDamagePending = true;
                    mPendingMiss = false;
                    std::string clipName = "";
                    const AnimationClip* clip = mSkin.getCurrentClip();
                    if (clip) clipName = clip->name;
                    float delayFactor = Animation::getHitDelayFactor(clipName);
                    mDamageDelay = attackDuration * delayFactor;
                    mDamageTimer.restart();

                    mAttackClock.restart();
                }
            }
            break;
        }
    
    case AiDecision::Returning:
        {
            sf::Vector2f diff = mSpawnPoint - mPos;
            if ((diff.x * diff.x) + (diff.y * diff.y) < 100.f) {
                mCurrentHp = getMaxHp();
                mCurrentMp = getMaxMp();
                notifyStatsChanged();
                
                mAiDecision = AiDecision::Idle;
                mAiTimer = Random::Float(cfg::Mob::IDLE_TIME_MIN, cfg::Mob::IDLE_TIME_MAX);
            } else {
                float dist = std::sqrt((diff.x * diff.x) + (diff.y * diff.y));
                mMoveDir = diff / dist;
                mMoving = true;
                if (diff.x > 0.f) mAiDir = AiDir::Right;
                else if (diff.x < 0.f) mAiDir = AiDir::Left;
            }
            break;
        }
    }

    if (mMoving) {
        float speedMult = getSpeedMultiplier();
        float currentSpeed = isCharging() ? 1000.f : mSpeed;
        mPos += mMoveDir * (currentSpeed * speedMult) * s;
        mVelocity = mMoveDir * (currentSpeed * speedMult);
    } else {
        mVelocity = {0.f, 0.f};
    }
}

void Mob::onAggroedBy(Entity* attacker) {
    if (mCurrentHp > 0 && attacker && attacker->isAlive() && !isReturningToSpawn()) {
        if (mStance != MobStance::Passive) {
            mAggroTarget = attacker;
            mForceAiUpdate = true;
            if (mAiDecision != AiDecision::Chasing) {
                mAiDecision = AiDecision::Chasing;
            }
        }
    }
}

void Mob::returnToSpawn() {
    mAiDecision = AiDecision::Returning;
    mAggroTarget = nullptr;

    if (mCombatSystem) {
        if (auto* p = mCombatSystem->getPlayer()) {
             p->removeFromAggro(this);
        }
    } else {
         if (auto* p = dynamic_cast<Player*>(mAggroTarget)) {
             p->removeFromAggro(this);
         }
    }
    
    clearDebuffs();
}

void Mob::resetAggro(Entity* requester) {
    clearDebuffs();

    if (mAggroTarget == requester) {
        mAggroTarget = nullptr;
        mAiDecision = AiDecision::Returning;
    }
}

void Mob::cancelPendingSkill() {
    mDamagePending = false;
    mPendingTarget = nullptr;
    mPendingSkill = nullptr;
    setCasting(false);
    if (mCurrentState == State::Attacking && mCurrentState != State::Dead && mCurrentState != State::Fading && mCurrentState != State::Removable) {
        mCurrentState = State::Idle;
        const AnimationClip* idleClipToUse = (mHasTwoHandedWeapon && mIdleTwoHandedClip) ? mIdleTwoHandedClip : mIdleClip;
        if (idleClipToUse) {
            mSkin.playAnimation(idleClipToUse);
        }
    }
}


bool Mob::isReturningToSpawn() const {
    return mAiDecision == AiDecision::Returning;
}
