#include "SkillManager.h"
// Active
#include "active/PowerStrike.h"
#include "active/Metamorphosis.h"
#include "active/Charge.h"
#include "active/BasicAttack.h"
#include "active/Whirlwind.h"
#include "active/TotemHeal_1/TotemHeal_1.h"
#include "active/OracionCurativa.h"
#include "active/ShieldSlam.h"
#include "mage/basic-orb.h"

// Buffs
#include "buffs/BerserkerFury.h"
#include "buffs/CorreWachin.h"
#include "buffs/StrBuff.h"
#include "buffs/AgiBuff.h"
#include "buffs/IntBuff.h"
#include "buffs/VitBuff.h"
#include "buffs/AttackBuff.h"
#include "buffs/DefenseBuff.h"
#include "buffs/MaxHpBuff.h"
#include "buffs/MoveSpeedBuff.h"
#include "buffs/AtkSpeedBuff.h"
#include "buffs/CritChanceBuff.h"

// Debuffs
#include "debuffs/StrDebuff.h"
#include "debuffs/AgiDebuff.h"
#include "debuffs/IntDebuff.h"
#include "debuffs/VitDebuff.h"
#include "debuffs/AttackDebuff.h"
#include "debuffs/DefenseDebuff.h"
#include "debuffs/MaxHpDebuff.h"
#include "debuffs/MoveSpeedDebuff.h"
#include "debuffs/AtkSpeedDebuff.h"
#include "debuffs/CritChanceDebuff.h"

#include "utils/TinyJson.h"
#include <fstream>
#include <iostream>
#include <sstream>


SkillManager::SkillManager(ResourceManager &res) : mRes(res) {
  // [ARCHI] Register Concrete Classes
  registerSkillType<PowerStrike>(1);
  registerSkillType<BerserkerFury>(2);
  registerSkillType<Metamorphosis>(3);
  registerSkillType<Charge>(4);
  registerSkillType<BasicAttack>(5);
  registerSkillType<CorreWachin>(6);
  registerSkillType<BasicOrb>(7);
  registerSkillType<TotemHeal_1>(8);
  registerSkillType<OracionCurativa>(9);
  registerSkillType<Whirlwind>(30);
  registerSkillType<ShieldSlam>(31);

  // Buffs (10-19)
  registerSkillType<StrBuff>(10);
  registerSkillType<AgiBuff>(11);
  registerSkillType<IntBuff>(12);
  registerSkillType<VitBuff>(13);
  registerSkillType<AttackBuff>(14);
  registerSkillType<DefenseBuff>(15);
  registerSkillType<MaxHpBuff>(16);
  registerSkillType<MoveSpeedBuff>(17);
  registerSkillType<AtkSpeedBuff>(18);
  registerSkillType<CritChanceBuff>(19);

  // Debuffs (20-29)
  registerSkillType<StrDebuff>(20);
  registerSkillType<AgiDebuff>(21);
  registerSkillType<IntDebuff>(22);
  registerSkillType<VitDebuff>(23);
  registerSkillType<AttackDebuff>(24);
  registerSkillType<DefenseDebuff>(25);
  registerSkillType<MaxHpDebuff>(26);
  registerSkillType<MoveSpeedDebuff>(27);
  registerSkillType<AtkSpeedDebuff>(28);
  registerSkillType<CritChanceDebuff>(29);
}

bool SkillManager::loadSkills(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "[SkillManager] Error loading " << filepath << "\n";
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string jsonStr = buffer.str();

  // TinyJson uses json::parse directly
  json::Value data = json::parse(jsonStr);
  if (data.type == json::Type::Null) {
    std::cerr << "[SkillManager] Failed to parse JSON or empty.\n";
    return false;
  }

  // Expecting Array of Objects
  if (data.type == json::Type::Array) {
    const auto &list = data.asArray();
    for (const auto &val : list) {
      if (val.type != json::Type::Object)
        continue;

      const auto &obj = val.asObject();
      // [ARCHI] Factory Logic
      std::unique_ptr<Skill> newSkill;

      int tempId = 0;
      if (obj.count("id"))
        tempId = obj.at("id").asInt();

      // [ARCHI] Use Factory Lookup
      if (mFactories.count(tempId)) {
        newSkill = mFactories[tempId]();
      } else {
        newSkill = std::make_unique<Skill>();
      }

      // Populate Base Data
      newSkill->id = tempId;
      if (obj.count("name"))
        newSkill->name = obj.at("name").asString();
      if (obj.count("description"))
        newSkill->description = obj.at("description").asString();
      if (obj.count("icon"))
        newSkill->iconPath = obj.at("icon").asString();
      if (obj.count("atlas_index") && obj.at("atlas_index").type == json::Type::Array) {
        const auto& arr = obj.at("atlas_index").asArray();
        if (arr.size() >= 2) {
          int col = arr[0].asInt();
          int row = arr[1].asInt();
          newSkill->atlasX = col * 18 + 1;
          newSkill->atlasY = row * 18 + 1;
        }
      }
      if (obj.count("cooldown"))
        newSkill->cooldown = (float)obj.at("cooldown").asDouble();
      if (obj.count("manaCost"))
        newSkill->manaCost = obj.at("manaCost").asInt();
      if (obj.count("damageFlat"))
        newSkill->damageFlat = obj.at("damageFlat").asInt();
      if (obj.count("range"))
        newSkill->range = obj.at("range").asInt();
      if (obj.count("duration"))
        newSkill->buffDuration = (float)obj.at("duration").asDouble(); // [NEW]
      if (obj.count("castTime"))
        newSkill->castTime = (float)obj.at("castTime").asDouble();
      if (obj.count("stunDuration"))
        newSkill->stunDuration = (float)obj.at("stunDuration").asDouble();

      std::string typeStr = "";
      if (obj.count("type"))
        typeStr = obj.at("type").asString();

      if (typeStr == "Passive")
        newSkill->type = SkillType::Passive;
      else if (typeStr == "Buff")
        newSkill->type = SkillType::Buff;
      else
        newSkill->type = SkillType::Active;

      // Parse target type
      if (obj.count("target")) {
        newSkill->targetType = obj.at("target").asString();
      } else {
        if (newSkill->type == SkillType::Buff || newSkill->type == SkillType::Passive) {
          newSkill->targetType = "SELF";
        } else {
          newSkill->targetType = "ENEMY";
        }
      }

      // Parse associated status effect id
      if (obj.count("statusEffectId")) {
        newSkill->statusEffectId = obj.at("statusEffectId").asString();
      }

      // Parse default action bar slot
      if (obj.count("default_slot")) {
        newSkill->defaultSlot = obj.at("default_slot").asInt();
      }

      // Parse charges granted to TapSystem
      if (obj.count("charges_granted")) {
        newSkill->chargesGranted = obj.at("charges_granted").asInt();
      } else if (obj.count("chargesGranted")) {
        newSkill->chargesGranted = obj.at("chargesGranted").asInt();
      }

      // Parse requires_shield
      if (obj.count("requires_shield")) {
        newSkill->requiresShield = obj.at("requires_shield").asBool();
      } else if (obj.count("requiresShield")) {
        newSkill->requiresShield = obj.at("requiresShield").asBool();
      }

      // [EFFECTS] Parse Effects Array
      if (obj.count("effects") && obj.at("effects").type == json::Type::Array) {
        const auto &effectList = obj.at("effects").asArray();
        for (const auto &effVal : effectList) {
          if (effVal.type != json::Type::Object)
            continue;
          const auto &effObj = effVal.asObject();

          EffectDef effect;
          std::string eType =
              effObj.count("type") ? effObj.at("type").asString() : "NONE";
          if (eType == "DAMAGE")
            effect.type = EffectType::DAMAGE;
          else if (eType == "HEAL")
            effect.type = EffectType::HEAL;
          else if (eType == "BUFF_STAT")
            effect.type = EffectType::BUFF_STAT;
          else if (eType == "STUN")
            effect.type = EffectType::STUN;

          if (effObj.count("stat")) {
            effect.statToBuff = stringToStat(effObj.at("stat").asString());
          }

          if (effObj.count("value"))
            effect.value = (float)effObj.at("value").asDouble();
          if (effObj.count("value"))
            effect.value = (float)effObj.at("value").asDouble();

          // [LOGIC] Use individual duration if present, else use global
          // buffDuration
          if (effObj.count("duration")) {
            effect.duration = (float)effObj.at("duration").asDouble();
          } else {
            effect.duration = newSkill->buffDuration;
          }

          newSkill->effects.push_back(effect);
        }
      }

      // Load Texture
      try {
        if (!newSkill->iconPath.empty())
          newSkill->iconTexture = &mRes.getTexture(newSkill->iconPath);
      } catch (...) {
        std::cerr << "[SkillManager] Failed to load icon for skill "
                  << newSkill->name << "\n";
      }

      mSkills[newSkill->id] = std::move(newSkill);
    }
  } else {
    std::cerr << "[SkillManager] JSON Format Error: Expected Array.\n";
    return false;
  }

  std::cout << "[SkillManager] Loaded " << mSkills.size() << " skills.\n";
  return true;
}

const Skill *SkillManager::getSkill(int id) const {
  auto it = mSkills.find(id);
  if (it != mSkills.end()) {
    return it->second.get();
  }
  return nullptr;
}

std::unique_ptr<Skill> SkillManager::cloneSkill(int id) const {
  auto factoryIt = mFactories.find(id);
  std::unique_ptr<Skill> newSkill;
  if (factoryIt != mFactories.end()) {
    newSkill = factoryIt->second();
  } else {
    newSkill = std::make_unique<Skill>();
  }

  auto templateIt = mSkills.find(id);
  if (templateIt != mSkills.end()) {
    const Skill* tpl = templateIt->second.get();
    newSkill->id = tpl->id;
    newSkill->name = tpl->name;
    newSkill->description = tpl->description;
    newSkill->iconPath = tpl->iconPath;
    newSkill->cooldown = tpl->cooldown;
    newSkill->manaCost = tpl->manaCost;
    newSkill->damageFlat = tpl->damageFlat;
    newSkill->damagePercent = tpl->damagePercent;
    newSkill->range = tpl->range;
    newSkill->buffDuration = tpl->buffDuration;
    newSkill->castTime = tpl->castTime;
    newSkill->stunDuration = tpl->stunDuration;
    newSkill->type = tpl->type;
    newSkill->iconTexture = tpl->iconTexture;
    newSkill->atlasX = tpl->atlasX;
    newSkill->atlasY = tpl->atlasY;
    newSkill->targetType = tpl->targetType;
    newSkill->statusEffectId = tpl->statusEffectId;
    newSkill->defaultSlot = tpl->defaultSlot;
    newSkill->effects = tpl->effects;
  }
  return newSkill;
}
