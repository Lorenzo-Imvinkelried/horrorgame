#pragma once

#include <SFML/Graphics.hpp>

class Entity;
class FXSystem;
class ParticleSystem;
struct DamageResult;

class CombatFeedback {
public:
    CombatFeedback(FXSystem& fx, ParticleSystem* particles);

    void onAttackStart(Entity* attacker, Entity* target);
    void onHit(Entity* target, int damage, bool isCrit, bool isTrueDamage, bool isBlocked = false, bool showParticles = true, float offsetY = 0.f, float scale = 1.0f, Entity* attacker = nullptr);
    void onHitImpact(Entity* target, bool isCrit); 
    void onSkillHit(Entity* target, int skillId);  
    void onHeal(Entity* target, int amount, Entity* source = nullptr);
    void onMiss(Entity* target, Entity* attacker = nullptr);
    void onExperience(Entity* source, int amount);
    
    void onBleedTick(Entity* target, int damage);
    void onStun(Entity* target, float duration);
    void onAoE(sf::Vector2f center, float radius, Entity* owner = nullptr);
    void onLevelUp(Entity* entity);

    void setParticleSystem(ParticleSystem* ps) { mParticles = ps; }

private:
    FXSystem& mFX;
    ParticleSystem* mParticles = nullptr;
};
