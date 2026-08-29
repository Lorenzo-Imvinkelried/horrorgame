#include "CombatSystem.h"
#include "core/managers/EntityManager.h"
#include "entities/mob/Mob.h"
#include "utils/CombatCalculator.h"
#include <algorithm>

void CombatSystem::updateAoEWaves(sf::Time dt) {
    if (mActiveWaves.empty()) return;

    for (size_t i = 0; i < mActiveWaves.size(); ) {
        AoEWave& wave = mActiveWaves[i];
        
        wave.currentRadius += wave.speed * dt.asSeconds();
        float currentRadiusSq = wave.currentRadius * wave.currentRadius;
        
        bool waveFinished = false;
        
        while (!wave.targets.empty()) {
            AoEWave::Target& t = wave.targets.back();
            
            if (t.distSq > currentRadiusSq) {
                break; 
            }
            
            Entity* e = t.entity;
            if (isEntityAllocated(e) && e->isAlive()) {
                 if (wave.attacker && isEntityAllocated(wave.attacker)) {
                    float rawAoe = wave.damageDealt * (wave.aoePercent / 100.0f);
                    auto res = CombatCalculator::calculateMitigatedDamage(wave.attacker, e, rawAoe, true);
                    
                    e->setLastHitDirect(false);
                    e->takeDamage(res.totalDamage, wave.attacker);
                    if (res.totalDamage > 0 && wave.attacker) {
                        e->triggerHitEffect(wave.attacker->getPosition());
                    }
                    
                    mFeedback.onHit(e, res.totalDamage, false, false, false, true, 0.f, 1.0f, wave.attacker);
                    
                    if (auto* p = dynamic_cast<Player*>(wave.attacker)) {
                         p->addToAggro(e);
                    }
                 } 
            }
            
            wave.targets.pop_back(); 
        }
        
        if (wave.currentRadius >= wave.maxRadius && wave.targets.empty()) {
            waveFinished = true;
        }
        if (wave.currentRadius > wave.maxRadius * 1.5f) waveFinished = true;

        if (waveFinished) {
            if (i != mActiveWaves.size() - 1) {
                mActiveWaves[i] = std::move(mActiveWaves.back());
            }
            mActiveWaves.pop_back();
        } else {
            ++i;
        }
    }
}

void CombatSystem::updateDebuffsAndBleeds(sf::Time dt) {
    for (size_t i = 0; i < mDebuffedEntities.size(); ) {
        Entity* e = mDebuffedEntities[i];
        
        if (!e || !mEntityManager->isValid(e)) {
            if (i != mDebuffedEntities.size() - 1) {
                mDebuffedEntities[i] = mDebuffedEntities.back();
            }
            mDebuffedEntities.pop_back();
            continue;
        }

        if (!e->isAlive()) {
            if (i != mDebuffedEntities.size() - 1) {
                mDebuffedEntities[i] = mDebuffedEntities.back();
            }
            mDebuffedEntities.pop_back();
            continue;
        }

        Entity::BleedState& b = e->getBleedState();
        Entity::StunState& s = e->getStunState();
        Entity::DebuffState& db = e->getDebuffState();

        bool stillAffected = false;

        // Bleed
        if (b.isBleeding()) {
            stillAffected = true;
            if (b.durationFlat > 0.f) b.durationFlat -= dt.asSeconds();
            if (b.durationPercent > 0.f) b.durationPercent -= dt.asSeconds();
            
            if (b.durationFlat <= 0.f) b.flatDamage = 0;
            if (b.durationPercent <= 0.f) b.percentDamage = 0.f;

            b.tickTimer += dt.asSeconds();

            if (!b.isBleeding()) {
                b.active = false;
            }

            if (b.active && b.tickTimer >= 1.0f) {
                b.tickTimer -= 1.0f; 

                int dmg = 0;
                if (b.durationFlat > 0.f) dmg += b.flatDamage;
                if (b.durationPercent > 0.f && b.percentDamage > 0.f) {
                    dmg += static_cast<int>(b.percentDamage);
                }
                
                if (dmg < 1) dmg = 1;

                int finalDmg = dmg;
                if (b.source && mEntityManager->isValid(b.source)) {
                     auto res = CombatCalculator::calculateMitigatedDamage(b.source, e, (float)dmg, true);
                     finalDmg = res.totalDamage;
                } else {
                     auto res = CombatCalculator::calculateMitigatedDamage(nullptr, e, (float)dmg, true);
                     finalDmg = res.totalDamage;
                }

                e->setLastHitDirect(false);
                e->takeDamage(finalDmg, b.source);

                mFeedback.onBleedTick(e, finalDmg);
            }
        }
        
        // Stun
        if (s.active) {
            stillAffected = true;
            s.duration -= dt.asSeconds();
            if (s.duration <= 0.f) {
                s.reset();
                e->notifyStatsChanged();
            }
        }
        
        // Debuffs
        bool hasSlow = false;
        if (db.slowMoveTimer > 0.f) {
            hasSlow = true;
            db.slowMoveTimer -= dt.asSeconds();
            if (db.slowMoveTimer <= 0.f) { 
                db.slowMovePercent = 0.f; 
                e->notifyStatsChanged();
            }
        }
        if (db.slowAttackTimer > 0.f) {
            hasSlow = true;
            db.slowAttackTimer -= dt.asSeconds();
            if (db.slowAttackTimer <= 0.f) { 
                db.slowAttackPercent = 0.f; 
                e->notifyStatsChanged();
            }
        }
        if (hasSlow) stillAffected = true;

        if (!stillAffected) {
             if (i != mDebuffedEntities.size() - 1) {
                mDebuffedEntities[i] = mDebuffedEntities.back();
            }
            mDebuffedEntities.pop_back();
        } else {
            ++i;
        }
    }
}

void CombatSystem::applyAoEAt(Entity* attacker, sf::Vector2f center, float radius, int damageDealt, float aoePercent) {
    if (!attacker || !mEntityManager) return;
    if (radius <= 1.0f) return;

    mFeedback.onAoE(center, radius, nullptr);

    std::vector<Entity*> nearbyEntities = mEntityManager->querySpatialGrid(center, radius);
    if (nearbyEntities.empty()) return;

    AoEWave wave;
    wave.attacker = attacker;
    wave.center = center;
    wave.currentRadius = 0.f;
    wave.maxRadius = radius;
    wave.damageDealt = damageDealt;
    wave.aoePercent = aoePercent;
    wave.speed = 1500.0f;

    float radiusSq = radius * radius;

    for (Entity* e : nearbyEntities) {
        if (!e || !e->isAlive() || e == attacker) continue;

        bool sameType = (dynamic_cast<Player*>(attacker) && dynamic_cast<Player*>(e)) ||
                        (dynamic_cast<Mob*>(attacker) && dynamic_cast<Mob*>(e));
        if (sameType) continue; 

        sf::Vector2f diff = e->getPosition() - center;
        float distSq = diff.x * diff.x + diff.y * diff.y;

        if (distSq <= radiusSq) {
             wave.targets.push_back({e, distSq});
        }
    }
    
    if (wave.targets.empty()) return;

    std::sort(wave.targets.begin(), wave.targets.end(), [](const AoEWave::Target& a, const AoEWave::Target& b) {
        return a.distSq > b.distSq;
    });

    mActiveWaves.push_back(std::move(wave));
}

void CombatSystem::applyAoE(Entity* attacker, Entity* primaryTarget, int damageDealt) {
    if (!attacker || !primaryTarget || !mEntityManager) return;

    float aoeRadius = attacker->getAoeRadius();
    float aoePercent = attacker->getAoeDamagePercent();

    if (aoeRadius <= 1.0f || aoePercent <= 0.0f) return;

    sf::FloatRect bounds = primaryTarget->getGlobalBounds();
    sf::Vector2f centerPos;
    centerPos.x = bounds.position.x + bounds.size.x * 0.5f;
    centerPos.y = (bounds.position.y + bounds.size.y) - 5.0f; 
    
    mFeedback.onAoE(centerPos, aoeRadius, primaryTarget);
    
    std::vector<Entity*> nearbyEntities = mEntityManager->querySpatialGrid(centerPos, aoeRadius);
    if (nearbyEntities.empty()) return;

    AoEWave wave;
    wave.attacker = attacker;
    wave.center = centerPos;
    wave.currentRadius = 0.f;
    wave.maxRadius = aoeRadius;
    wave.damageDealt = damageDealt;
    wave.aoePercent = aoePercent;
    wave.speed = 1500.0f; 
    
    float radiusSq = aoeRadius * aoeRadius;

    for (Entity* e : nearbyEntities) {
        if (!e || !e->isAlive() || e == attacker || e == primaryTarget) continue;

        bool sameType = (dynamic_cast<Player*>(attacker) && dynamic_cast<Player*>(e)) ||
                        (dynamic_cast<Mob*>(attacker) && dynamic_cast<Mob*>(e));
        if (sameType) continue; 

        sf::Vector2f diff = e->getPosition() - centerPos;
        float distSq = diff.x * diff.x + diff.y * diff.y;

        if (distSq <= radiusSq) {
             wave.targets.push_back({e, distSq});
        }
    }
    
    if (wave.targets.empty()) return;

    std::sort(wave.targets.begin(), wave.targets.end(), [](const AoEWave::Target& a, const AoEWave::Target& b) {
        return a.distSq > b.distSq;
    });

    mActiveWaves.push_back(std::move(wave));
}

void CombatSystem::registerDebuff(Entity* target) {
    if (!target) return;
    
    for (Entity* e : mDebuffedEntities) {
        if (e == target) return; 
    }
    mDebuffedEntities.push_back(target);
}

void CombatSystem::onEntityDeath(Entity* target) {
    if (!target) return;
    for (size_t i = 0; i < mDebuffedEntities.size(); ++i) {
        if (mDebuffedEntities[i] == target) {
            if (i != mDebuffedEntities.size() - 1) {
                mDebuffedEntities[i] = mDebuffedEntities.back();
            }
            mDebuffedEntities.pop_back();
            return;
        }
    }
}
