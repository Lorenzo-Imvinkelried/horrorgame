#pragma once
#include <string>
#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include "core/stats/Stats.h"

// Forward Decls
class Entity;
class ParticleSystem;
class CombatFeedback;
class CombatSystem; // Needed for specific callbacks if tricky

enum class SkillType {
    Active,
    Passive,
    Buff
};

// [ARCHI] Polymorphic Base Class
class Skill {
public:
    virtual ~Skill() = default;

    int id;
    std::string name;
    std::string description;
    std::string iconPath;
    float cooldown;
    int manaCost;
    
    // Combat Stats
    int damageFlat;
    int getEffectiveDamageFlat() const;
    float getEffectiveStunDuration() const;
    float getEffectiveBuffDuration() const;
    float getEffectiveValue(float baseVal) const;
    float getEffectiveValue(Stat stat, float baseVal) const;
    float damagePercent;
    int range; 
    float buffDuration = 0.f; // [NEW] Global duration for buffs 
    float castTime = 0.f;
    float stunDuration = 0.f; // [NEW] Stun duration override

    SkillType type;
    const sf::Texture* iconTexture = nullptr; 
    std::vector<EffectDef> effects;
    int atlasX = 0; // [NEW] Coordinates in skills atlas
    int atlasY = 0; // [NEW] Coordinates in skills atlas
    std::string targetType = "ENEMY"; // [NEW] "ENEMY" or "SELF"
    std::string statusEffectId = ""; // [NEW] UI status effect id
    int defaultSlot = -1; // [NEW] Default action bar slot to equip this skill (-1 = none)
    int chargesGranted = 0; // [NEW] TapSystem charges granted on cast/hit
    bool requiresShield = false; // [NEW] If true, requires a shield equipped

    // Requirement validation before casting
    virtual bool canCast(const Entity* caster, std::string* outReason = nullptr) const;

    // [VIRTUAL INTERFACE]
    // Called when the casting starts (Animation, Mana consume?, Hand Particles)
    // [VIRTUAL INTERFACE]
    // Called when the casting starts (Animation, Mana consume?, Hand Particles)
    virtual void onCastStart(Entity* caster, Entity* target, ParticleSystem* particles);
    virtual void onCastTick(Entity* caster, ParticleSystem* particles) {}
    virtual void onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {} // The "Effect" moment

    virtual void onHit(Entity* caster, Entity* target, CombatFeedback& feedback, ParticleSystem* particles, class CombatSystem* combatSystem = nullptr);
    
    // [VISUALS & TIMING]
    virtual float getSpeedMultiplier() const { return 1.0f; }
    virtual void onQueue(Entity* caster, ParticleSystem* particles) {}
    virtual void onRemoveEffects(Entity* caster, ParticleSystem* particles) {}
    virtual void updateChargeVisuals(Entity* caster, ParticleSystem* particles) {} // [NEW] Continuous emission
};
