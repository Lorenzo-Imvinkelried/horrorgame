#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

class Entity;
class Player;
class CombatSystem;
class FXSystem;

class AggroSystem {
public:
    AggroSystem() = default;
    ~AggroSystem() = default;

    void update(sf::Time dt, Player* player, const std::vector<std::unique_ptr<Entity>>& entities, CombatSystem* combatSystem, FXSystem* fxSystem);
    void clear();
    bool isSuspicious(const Entity* mob) const;

private:
    float calculateEffectiveRange(float baseRangeViolent, float targetMalice) const;
    float calculateReactionTime(float targetMalice) const;

private:
    struct SuspicionData {
        float timer = 0.f;
        float fxTimer = 0.35f; // trigger immediately on first frame inside
    };
    std::unordered_map<Entity*, SuspicionData> mSuspicionMap;
};
