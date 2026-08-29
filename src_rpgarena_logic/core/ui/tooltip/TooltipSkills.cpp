#include "Tooltip.h"
#include "core/skills/Skill.h"
#include "entities/player/Player.h"
#include "core/systems/SkillUpgradeSystem.h"
#include "core/managers/TitleManager.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

void Tooltip::show(const Skill &skill, sf::Vector2f position,
                   sf::Vector2u windowSize, Player* player) {
  mVisible = true;
  mCurrentItem = nullptr;
  mCurrentEquippedItem = nullptr;
  mLines.clear();
  mEquippedLines.clear();
  mShowComparison = false;

  // 1. Header (Name + Level)
  TooltipLine header;
  int lvl = SkillUpgradeSystem::getInstance().getSkillLevel(skill.id);
  header.parts.push_back({skill.name, sf::Color(255, 200, 50)});
  header.parts.push_back({" (Niv. " + std::to_string(lvl) + ")", sf::Color(180, 230, 180)});
  mLines.push_back(header);

  // Type/Category
  std::string typeStr = "Activa";
  if (skill.type == SkillType::Passive)
    typeStr = "Pasiva";
  else if (skill.type == SkillType::Buff)
    typeStr = "Buff";

  TooltipLine sub;
  sub.parts.push_back({typeStr, sf::Color(180, 180, 180)});
  mLines.push_back(sub);

  mLines.push_back(TooltipLine());

  // 2. Costs & CD
  if (skill.cooldown > 0.f) {
    float displayCd = skill.cooldown;
    if (player) {
        float cdrPct = std::clamp(player->getCooldownReductionPercent(), 0.f, 100.f);
        displayCd = skill.cooldown * (1.f - cdrPct / 100.f);
    }
    if (displayCd < 0.f) displayCd = 0.f;

    TooltipLine cdLine;
    std::stringstream ss;
    ss << "Cooldown: " << std::fixed << std::setprecision(1) << displayCd << "s";
    cdLine.parts.push_back({ss.str(), sf::Color::Cyan});
    mLines.push_back(cdLine);
  }
  if (skill.manaCost > 0) {
    addStatLine(mLines, "Mana", (float)skill.manaCost, true, false,
                sf::Color(100, 150, 255));
  }

  // 3. Stats (Damage, Range)
  if (skill.damageFlat > 0 || skill.id == 4 || skill.id == 5) {
    int baseDmg = skill.getEffectiveDamageFlat();
    int bonusDmg = player ? player->getAttack() : 0;
    
    TooltipLine dmgLine;
    dmgLine.parts.push_back({"Daño Base: " + std::to_string(baseDmg), sf::Color::White});
    dmgLine.parts.push_back({" + " + std::to_string(bonusDmg), sf::Color(100, 180, 255)});
    mLines.push_back(dmgLine);
  }
  if (skill.range > 0) {
    addStatLine(mLines, "Rango", (float)skill.range, true, false,
                sf::Color::White);
  }

  float effBuffDur = skill.getEffectiveBuffDuration();
  if (effBuffDur > 0.f) {
    addStatLine(mLines, "Duración", effBuffDur, false, false,
                sf::Color::Green);
  }

  float effStunDur = skill.getEffectiveStunDuration();
  if (effStunDur > 0.f) {
    TooltipLine stunLine;
    std::stringstream ss;
    ss << "Aturdimiento: " << std::fixed << std::setprecision(1) << effStunDur << "s";
    stunLine.parts.push_back({ss.str(), sf::Color(255, 215, 0)});
    mLines.push_back(stunLine);
  }

  if (skill.chargesGranted > 0) {
    TooltipLine chargeLine;
    std::string chargeText = "+" + std::to_string(skill.chargesGranted) + (skill.chargesGranted > 1 ? " Cargas" : " Carga");
    chargeLine.parts.push_back({chargeText, sf::Color(255, 180, 50)});
    mLines.push_back(chargeLine);
  }

  for (const auto &eff : skill.effects) {
    if (eff.type == EffectType::BUFF_STAT) {
      std::string label = getStatDisplayName(eff.statToBuff);
      bool isInt = isStatInteger(eff.statToBuff);
      bool isPercentage = !isInt;
      float effVal = skill.getEffectiveValue(eff.statToBuff, eff.value);

      sf::Color valColor = (effVal >= 0.f) ? sf::Color::Green : sf::Color::Red;
      addStatLine(mLines, label, effVal, isInt, isPercentage, valColor);
    }
  }

  // 4. Description
  if (!skill.description.empty()) {
    mLines.push_back(TooltipLine());
    TooltipLine descL;
    descL.parts.push_back({skill.description, sf::Color(220, 220, 220)});
    mLines.push_back(descL);
  }

  calculateSize(mLines, mBackground);
  setPosition(position, windowSize);
}

void Tooltip::show(const Title &title, sf::Vector2f position,
                   sf::Vector2u windowSize) {
  mVisible = true;
  mCurrentItem = nullptr;
  mCurrentEquippedItem = nullptr;
  mLines.clear();
  mEquippedLines.clear();
  mShowComparison = false;

  // 1. Header (Title name using title's color)
  TooltipLine header;
  header.parts.push_back({title.name, title.color});
  mLines.push_back(header);

  // 2. Subheader (Character Title)
  TooltipLine subHeader;
  subHeader.parts.push_back({"Titulo del Personaje", sf::Color(150, 150, 150)});
  mLines.push_back(subHeader);

  // 3. Spacer
  mLines.push_back(TooltipLine());

  // 4. Stats
  for (const auto &[stat, value] : title.stats) {
    if (std::abs(value) <= 0.001f)
      continue;

    std::string label = getStatDisplayName(stat);
    bool isInt = isStatInteger(stat);
    bool isPercent = isStatPercent(stat);

    std::stringstream ss;
    ss << label << ": " << (value >= 0.f ? "+" : "");
    if (isInt) {
      ss << static_cast<int>(value);
    } else {
      ss << std::fixed << std::setprecision(1) << value;
    }
    if (isPercent) {
      ss << "%";
    }

    sf::Color valColor = (value >= 0.f) ? sf::Color::Green : sf::Color::Red;
    addLine(mLines, ss.str(), valColor);
  }

  calculateSize(mLines, mBackground);
  setPosition(position, windowSize);
}

void Tooltip::show(const std::string& name, const std::string& description, float remainingDuration, sf::Vector2f position, sf::Vector2u windowSize) {
  mVisible = true;
  mCurrentItem = nullptr;
  mCurrentEquippedItem = nullptr;
  mLines.clear();
  mEquippedLines.clear();
  mShowComparison = false;

  // 1. Header (Status Effect Name)
  TooltipLine header;
  header.parts.push_back({name, sf::Color(249, 194, 43)});
  mLines.push_back(header);

  // 2. Subheader (Remaining Duration)
  TooltipLine subHeader;
  std::stringstream ss;
  ss << "Duracion restante: " << std::fixed << std::setprecision(1) << remainingDuration << "s";
  subHeader.parts.push_back({ss.str(), sf::Color(180, 180, 180)});
  mLines.push_back(subHeader);

  // 3. Spacer
  mLines.push_back(TooltipLine());

  // 4. Description
  addLine(mLines, description, sf::Color::White);

  calculateSize(mLines, mBackground);
  setPosition(position, windowSize);
}
