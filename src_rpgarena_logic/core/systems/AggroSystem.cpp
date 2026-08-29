#include "AggroSystem.h"
#include "entities/mob/Mob.h"
#include "entities/player/Player.h"
#include "core/systems/combat/CombatSystem.h"
#include "FXSystem.h"
#include <algorithm>
#include <cmath>

float AggroSystem::calculateEffectiveRange(float baseRangeViolent, float targetMalice) const {
    float factor = 1.0f + (targetMalice / 10000.0f) * 0.35f;
    factor = std::clamp(factor, 0.5f, 1.50f);
    return baseRangeViolent * factor;
}

float AggroSystem::calculateReactionTime(float targetMalice) const {
    float reaction = 1.0f * (1.0f - (targetMalice / 10000.0f) * 0.8f);
    return std::max(0.1f, reaction);
}

void AggroSystem::clear() {
    mSuspicionMap.clear();
}

bool AggroSystem::isSuspicious(const Entity* mob) const {
    return mSuspicionMap.find(const_cast<Entity*>(mob)) != mSuspicionMap.end();
}

void AggroSystem::update(sf::Time dt, Player* player, const std::vector<std::unique_ptr<Entity>>& entities, CombatSystem* combatSystem, FXSystem* fxSystem) {
    if (!player || !player->isAlive()) {
        mSuspicionMap.clear();
        return;
    }

    float s = dt.asSeconds();
    float playerMalice = player->getMalice();
    bool playerInCombat = player->isInCombat() || (combatSystem && combatSystem->isInCombat());

    for (const auto& entityPtr : entities) {
        if (!entityPtr) continue;
        Mob* mob = dynamic_cast<Mob*>(entityPtr.get());
        if (!mob || !mob->isAlive() || mob->isReturningToSpawn()) {
            mSuspicionMap.erase(entityPtr.get());
            continue;
        }

        // Only violent mobs inspect targets automatically
        if (mob->getStance() != MobStance::Violent) {
            mSuspicionMap.erase(mob);
            continue;
        }

        // If mob is already in active chasing/aggro, clear suspicion timer
        if (mob->isAggro()) {
            mSuspicionMap.erase(mob);
            continue;
        }

        float effectiveRange = calculateEffectiveRange(mob->getRangeViolent(), playerMalice);
        float reactionTime = calculateReactionTime(playerMalice);

        sf::Vector2f diff = player->getPosition() - mob->getPosition();
        float distSq = diff.x * diff.x + diff.y * diff.y;

        if (distSq <= (effectiveRange * effectiveRange)) {
            // If player is already in combat (e.g. attacked or engaged another mob),
            // any watching/nearby violent mob immediately joins the fight (!), skipping suspicion (?)
            if (playerInCombat) {
                mob->onAggroedBy(player);
                player->addToAggro(mob);
                if (combatSystem) {
                    combatSystem->engageCombat();
                }
                mSuspicionMap.erase(mob);
                continue;
            }

            // Inside aggro radius: increase suspicion timer
            SuspicionData& data = mSuspicionMap[mob];
            data.timer += s;
            data.fxTimer += s;

            if (data.timer >= reactionTime) {
                // Aggro confirmed!
                mob->onAggroedBy(player);
                player->addToAggro(mob);

                if (combatSystem) {
                    combatSystem->engageCombat();
                }

                mSuspicionMap.erase(mob);
            }
        } else {
            // radio de afuera
            auto it = mSuspicionMap.find(mob);
            if (it != mSuspicionMap.end()) {
                it->second.timer -= s * 2.0f; // nivel sospecha bajada rapida
                if (it->second.timer <= 0.0f) {
                    mSuspicionMap.erase(it);
                }
            }
        }
    }
}
