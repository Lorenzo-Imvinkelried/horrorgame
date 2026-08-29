#pragma once
#include <string>
#include <map>
#include <algorithm>

enum class Stat {
    // Primary
    STR, DEX, INT, VIT,
    
    // Combat
    ATTACK,
    DEFENSE,
    MAX_HP,
    MAX_MP,
    HP_REGEN,
    MP_REGEN,
    ACCURACY,
    EVASION,

    // Offensive
    ATK_SPEED,
    CRIT_CHANCE,
    CRIT_DMG,
    ARMOR_PEN_FLAT,
    ARMOR_PEN_PERCENT,
    TRUE_DMG_PERCENT,
    EXECUTE_DMG_PERCENT,
    EXECUTE_THRESH_PERCENT,
    
    // Defensive
    BLOCK_CHANCE,
    BLOCK_VALUE,
    THORNS,
    TENACITY,
    DMG_REDUCTION,
    CRIT_AVOIDANCE,
    
    // Utility
    LIFESTEAL,
    MANA_STEAL,
    MOVE_SPEED,
    XP_BONUS,
    COOLDOWN_REDUCTION,
    
    // Multi Strike
    DOUBLE_STRIKE,
    TRIPLE_STRIKE,
    
    // Debuff Appliers
    STUN_CHANCE,
    STUN_DURATION,
    BLEED_CHANCE, // If implemented
    BLEED_DMG_PERCENT,
    ATTACK_RANGE,
    
    // Additional/Special Stats for titles & items
    BLEED_FLAT,
    BLEED_DURATION_FLAT,
    BLEED_DURATION_PERCENT,
    SLOW_MOVE_PERCENT,
    SLOW_MOVE_DURATION,
    SLOW_ATTACK_PERCENT,
    SLOW_ATTACK_DURATION,
    AOE_RADIUS,
    AOE_DAMAGE_PERCENT,
    ANTI_ARMOR_PEN_FLAT,
    ANTI_ARMOR_PEN_PERCENT,
    PHYSICAL_DAMAGE_BONUS,
    ENEMY_MAX_HP_DAMAGE_PERCENT,
    
    None
};

enum class StatusEffect {
    None = 0,
    Stun,
    Silence,
    Root,
    Fear,
    Polymorph,
    Banish
};

inline Stat stringToStat(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    
    if (u == "STR" || u == "STRENGTH") return Stat::STR;
    if (u == "DEX" || u == "DEXTERITY" || u == "AGI" || u == "AGILITY") return Stat::DEX;
    if (u == "INT" || u == "INTELLIGENCE") return Stat::INT;
    if (u == "VIT" || u == "VITALITY") return Stat::VIT;
    
    if (u == "ATTACK" || u == "ATK") return Stat::ATTACK;
    if (u == "DEFENSE" || u == "DEF") return Stat::DEFENSE;
    if (u == "MAXHP" || u == "MAX_HP" || u == "HP") return Stat::MAX_HP;
    if (u == "MAXMP" || u == "MAX_MP" || u == "MP" || u == "MANA") return Stat::MAX_MP;
    
    if (u == "ACCURACY" || u == "ACC") return Stat::ACCURACY;
    if (u == "EVASION" || u == "DODGE" || u == "EVA") return Stat::EVASION;
    
    if (u == "ATKSPEED" || u == "ATK_SPEED") return Stat::ATK_SPEED;
    if (u == "CRITCHANCE" || u == "CRIT_CHANCE" || u == "CRIT") return Stat::CRIT_CHANCE;
    if (u == "CRITDMG" || u == "CRIT_DMG" || u == "DCRIT") return Stat::CRIT_DMG;
    if (u == "LIFESTEAL" || u == "ROBOVIDA") return Stat::LIFESTEAL;
    if (u == "TENACITY") return Stat::TENACITY;
    if (u == "MOVESPEED" || u == "SPEED" || u == "VEL") return Stat::MOVE_SPEED;
    if (u == "RANGE" || u == "ATTACK_RANGE" || u == "ATTACKRANGE" || u == "RANGO") return Stat::ATTACK_RANGE;

    if (u == "HP_REGEN" || u == "HPREGEN" || u == "HPREG") return Stat::HP_REGEN;
    if (u == "MP_REGEN" || u == "MPREGEN" || u == "MPREG") return Stat::MP_REGEN;
    if (u == "ARMOR_PEN_FLAT" || u == "ARMORPENFLAT" || u == "PEN") return Stat::ARMOR_PEN_FLAT;
    if (u == "ARMOR_PEN_PERCENT" || u == "ARMORPENPERCENT" || u == "PENPCT") return Stat::ARMOR_PEN_PERCENT;
    if (u == "TRUE_DMG_PERCENT" || u == "TRUEDMGPERCENT" || u == "TRUEDMG") return Stat::TRUE_DMG_PERCENT;
    if (u == "EXECUTE_DMG_PERCENT" || u == "EXECUTEDMGPERCENT" || u == "EXECDMG") return Stat::EXECUTE_DMG_PERCENT;
    if (u == "EXECUTE_THRESH_PERCENT" || u == "EXECUTETHRESHPERCENT" || u == "EXECTHR") return Stat::EXECUTE_THRESH_PERCENT;
    if (u == "BLOCK_CHANCE" || u == "BLOCKCHANCE" || u == "BLOCK") return Stat::BLOCK_CHANCE;
    if (u == "BLOCK_VALUE" || u == "BLOCKVALUE") return Stat::BLOCK_VALUE;
    if (u == "THORNS") return Stat::THORNS;
    if (u == "DMG_REDUCTION" || u == "DMGREDUCTION" || u == "DMGRED") return Stat::DMG_REDUCTION;
    if (u == "CRIT_AVOIDANCE" || u == "CRITAVOIDANCE" || u == "CRITAVOID") return Stat::CRIT_AVOIDANCE;
    if (u == "MANA_STEAL" || u == "MANASTEAL") return Stat::MANA_STEAL;
    if (u == "XP_BONUS" || u == "XPBONUS") return Stat::XP_BONUS;
    if (u == "COOLDOWN_REDUCTION" || u == "COOLDOWNREDUCTION" || u == "CDR") return Stat::COOLDOWN_REDUCTION;

    if (u == "DOUBLE_STRIKE" || u == "DOUBLESTRIKE" || u == "DBLHIT") return Stat::DOUBLE_STRIKE;
    if (u == "TRIPLE_STRIKE" || u == "TRIPLESTRIKE" || u == "TRIHIT") return Stat::TRIPLE_STRIKE;

    if (u == "STUN_CHANCE" || u == "STUNCHANCE" || u == "STUN") return Stat::STUN_CHANCE;
    if (u == "STUN_DURATION" || u == "STUNDURATION") return Stat::STUN_DURATION;
    if (u == "BLEED_CHANCE" || u == "BLEEDCHANCE") return Stat::BLEED_CHANCE;
    if (u == "BLEED_DMG_PERCENT" || u == "BLEEDPERCENT" || u == "BLEEDDMG") return Stat::BLEED_DMG_PERCENT;

    if (u == "BLEED_FLAT" || u == "BLEEDFLAT") return Stat::BLEED_FLAT;
    if (u == "BLEED_DURATION_FLAT" || u == "BLEEDDURATIONFLAT") return Stat::BLEED_DURATION_FLAT;
    if (u == "BLEED_DURATION_PERCENT" || u == "BLEEDDURATIONPERCENT" || u == "BLEEDDURATIONPCT") return Stat::BLEED_DURATION_PERCENT;
    if (u == "SLOW_MOVE_PERCENT" || u == "SLOWMOVEPERCENT" || u == "SLOWMOVEPCT" || u == "SLOWMOVE") return Stat::SLOW_MOVE_PERCENT;
    if (u == "SLOW_MOVE_DURATION" || u == "SLOWMOVEDURATION") return Stat::SLOW_MOVE_DURATION;
    if (u == "SLOW_ATTACK_PERCENT" || u == "SLOWATTACKPERCENT" || u == "SLOWATTACKPCT" || u == "SLOWATK") return Stat::SLOW_ATTACK_PERCENT;
    if (u == "SLOW_ATTACK_DURATION" || u == "SLOWATTACKDURATION") return Stat::SLOW_ATTACK_DURATION;
    if (u == "AOE_RADIUS" || u == "AOERADIUS" || u == "AOERAD") return Stat::AOE_RADIUS;
    if (u == "AOE_DAMAGE_PERCENT" || u == "AOEDAMAGEPERCENT" || u == "AOEDAMAGEPCT" || u == "AOEDMG") return Stat::AOE_DAMAGE_PERCENT;
    if (u == "ANTI_ARMOR_PEN_FLAT" || u == "ANTIARMORPENFLAT" || u == "ANTIPEN") return Stat::ANTI_ARMOR_PEN_FLAT;
    if (u == "ANTI_ARMOR_PEN_PERCENT" || u == "ANTIARMORPENPERCENT" || u == "ANTIPENPCT") return Stat::ANTI_ARMOR_PEN_PERCENT;
    if (u == "PHYSICAL_DAMAGE_BONUS" || u == "PHYSICALDMGBONUS" || u == "PHYSICALDAMAGEBONUS" || u == "PHYSDMG") return Stat::PHYSICAL_DAMAGE_BONUS;
    if (u == "ENEMY_MAX_HP_DAMAGE_PERCENT" || u == "ENEMYMAXHPDAMAGEPERCENT" || u == "ENEMYMAXHPDMG") return Stat::ENEMY_MAX_HP_DAMAGE_PERCENT;

    return Stat::None;
}

// Effect Types
enum class EffectType {
    NONE,
    DAMAGE,         // Instant Damage
    HEAL,           // Instant Heal
    BUFF_STAT,      // Temporary Stat Boost
    STUN,           // Apply Stun
    SPAWN_PROJECTILE // e.g. Fireball
};

struct EffectDef {
    EffectType type = EffectType::NONE;
    Stat statToBuff = Stat::None; // Only for BUFF_STAT
    float value = 0.f;            // Dmg amount, or Stat amount
    float duration = 0.f;         // 0 for instant
};
