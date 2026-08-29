#include "Tooltip.h"
#include "Config.h"
#include <cmath>
#include <iomanip>
#include <sstream>

Tooltip::Tooltip(sf::Texture *fontTexture)
    : mFontTexture(fontTexture), mVisible(false), mShowComparison(false) {
  // Main Background
  mBackground.setFillColor(cfg::UI::Tooltip::BG_COLOR);
  mBackground.setOutlineColor(cfg::UI::Tooltip::BORDER_COLOR);
  mBackground.setOutlineThickness(cfg::UI::Tooltip::BORDER_SIZE);

  // Comparison Background
  mEquippedBackground.setFillColor(cfg::UI::Tooltip::BG_COLOR);
  mEquippedBackground.setOutlineColor(cfg::UI::Tooltip::BORDER_COLOR);
  mEquippedBackground.setOutlineThickness(cfg::UI::Tooltip::BORDER_SIZE);
}

void Tooltip::addLine(std::vector<TooltipLine> &targetLines,
                      const std::string &text, sf::Color color) {
  TooltipLine line;
  line.parts.push_back({text, color});
  targetLines.push_back(line);
}

bool Tooltip::isDiffSignificant(float diff) {
  return std::abs(diff) > 0.01f;
}

std::string Tooltip::getStatDisplayName(Stat stat) {
  switch (stat) {
  case Stat::STR:
    return "STR";
  case Stat::DEX:
    return "AGI";
  case Stat::INT:
    return "INT";
  case Stat::VIT:
    return "VIT";
  case Stat::ATTACK:
    return "Atq. F";
  case Stat::DEFENSE:
    return "Def. F";
  case Stat::MAX_HP:
    return "HP";
  case Stat::MAX_MP:
    return "MP";
  case Stat::HP_REGEN:
    return "Reg. HP";
  case Stat::MP_REGEN:
    return "Reg. MP";
  case Stat::ACCURACY:
    return "Precisión";
  case Stat::EVASION:
    return "Evasión";
  case Stat::ATK_SPEED:
    return "Vel. Atq.";
  case Stat::CRIT_CHANCE:
    return "Tasa Critico";
  case Stat::CRIT_DMG:
    return "Daño Critico";
  case Stat::LIFESTEAL:
    return "Robo de Vida";
  case Stat::TENACITY:
    return "Tenacidad";
  case Stat::MOVE_SPEED:
    return "Vel. Mov";
  case Stat::ATTACK_RANGE:
    return "Rango";
  case Stat::ARMOR_PEN_FLAT:
    return "Pen. Def";
  case Stat::ARMOR_PEN_PERCENT:
    return "Pen. Def %";
  case Stat::TRUE_DMG_PERCENT:
    return "Daño Verdadero %";
  case Stat::EXECUTE_DMG_PERCENT:
    return "Daño Ejecución %";
  case Stat::EXECUTE_THRESH_PERCENT:
    return "Umbral Ejecución %";
  case Stat::BLOCK_CHANCE:
    return "Prob. Bloqueo %";
  case Stat::BLOCK_VALUE:
    return "Valor Bloqueo";
  case Stat::THORNS:
    return "Espinas";
  case Stat::DMG_REDUCTION:
    return "Reducción de Daño";
  case Stat::CRIT_AVOIDANCE:
    return "Resistencia Critico F %";
  case Stat::MANA_STEAL:
    return "Robo de Mana %";
  case Stat::XP_BONUS:
    return "Bono de XP %";
  case Stat::DOUBLE_STRIKE:
    return "Doble Golpe %";
  case Stat::TRIPLE_STRIKE:
    return "Triple Golpe %";
  case Stat::STUN_CHANCE:
    return "Prob. Stun %";
  case Stat::STUN_DURATION:
    return "Duración Stun";
  case Stat::BLEED_DMG_PERCENT:
    return "Daño Sangrado %";
  case Stat::BLEED_FLAT:
    return "Sangrado Plano";
  case Stat::BLEED_DURATION_FLAT:
    return "Duracion Sangrado";
  case Stat::BLEED_DURATION_PERCENT:
    return "Duración Sangrado %";
  case Stat::SLOW_MOVE_PERCENT:
    return "Ralentizar Mov %";
  case Stat::SLOW_MOVE_DURATION:
    return "Duración Ral. Mov";
  case Stat::SLOW_ATTACK_PERCENT:
    return "Ralentizar Atq %";
  case Stat::SLOW_ATTACK_DURATION:
    return "Duración Slow Atq";
  case Stat::AOE_RADIUS:
    return "Radio de AoE";
  case Stat::AOE_DAMAGE_PERCENT:
    return "Daño de AoE %";
  case Stat::ANTI_ARMOR_PEN_FLAT:
    return "Anti Pen. Armadura";
  case Stat::ANTI_ARMOR_PEN_PERCENT:
    return "Anti Pen. Armadura %";
  case Stat::PHYSICAL_DAMAGE_BONUS:
    return "Bono Daño Físico";
  case Stat::ENEMY_MAX_HP_DAMAGE_PERCENT:
    return "Daño Max HP Enemigo";
  case Stat::COOLDOWN_REDUCTION:
    return "Reducción Enfriamiento";
  default:
    return "Desconocido";
  }
}

bool Tooltip::isStatInteger(Stat stat) {
  switch (stat) {
  case Stat::STR:
  case Stat::DEX:
  case Stat::INT:
  case Stat::VIT:
  case Stat::ATTACK:
  case Stat::DEFENSE:
  case Stat::MAX_HP:
  case Stat::MAX_MP:
  case Stat::BLOCK_VALUE:
  case Stat::THORNS:
  case Stat::BLEED_FLAT:
  case Stat::ATTACK_RANGE:
  case Stat::AOE_RADIUS:
  case Stat::ARMOR_PEN_FLAT:
  case Stat::ANTI_ARMOR_PEN_FLAT:
  case Stat::MOVE_SPEED:
    return true;
  default:
    return false;
  }
}

bool Tooltip::isStatPercent(Stat stat) {
  switch (stat) {
  case Stat::CRIT_CHANCE:
  case Stat::CRIT_DMG:
  case Stat::ARMOR_PEN_PERCENT:
  case Stat::TRUE_DMG_PERCENT:
  case Stat::EXECUTE_DMG_PERCENT:
  case Stat::EXECUTE_THRESH_PERCENT:
  case Stat::BLOCK_CHANCE:
  case Stat::DMG_REDUCTION:
  case Stat::CRIT_AVOIDANCE:
  case Stat::LIFESTEAL:
  case Stat::MANA_STEAL:
  case Stat::XP_BONUS:
  case Stat::DOUBLE_STRIKE:
  case Stat::TRIPLE_STRIKE:
  case Stat::STUN_CHANCE:
  case Stat::BLEED_DMG_PERCENT:
  case Stat::BLEED_DURATION_PERCENT:
  case Stat::SLOW_MOVE_PERCENT:
  case Stat::SLOW_ATTACK_PERCENT:
  case Stat::AOE_DAMAGE_PERCENT:
  case Stat::ANTI_ARMOR_PEN_PERCENT:
  case Stat::ENEMY_MAX_HP_DAMAGE_PERCENT:
    return true;
  default:
    return false;
  }
}

std::string Tooltip::getStoneStatsString(const Item& stone) {
    std::vector<std::string> parts;
    const auto& s = stone.stats;
    if (s.physicalDamage > 0) parts.push_back("+" + std::to_string(s.physicalDamage) + " Dmg");
    if (s.defense > 0) parts.push_back("+" + std::to_string(s.defense) + " Def");
    if (s.strength > 0) parts.push_back("+" + std::to_string(s.strength) + " STR");
    if (s.agility > 0) parts.push_back("+" + std::to_string(s.agility) + " AGI");
    if (s.intelligence > 0) parts.push_back("+" + std::to_string(s.intelligence) + " INT");
    if (s.vitality > 0) parts.push_back("+" + std::to_string(s.vitality) + " VIT");
    if (s.hpBonus > 0) parts.push_back("+" + std::to_string(s.hpBonus) + " HP");
    if (s.mpBonus > 0) parts.push_back("+" + std::to_string(s.mpBonus) + " MP");
    if (s.critChance > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.critChance << "% Crit";
        parts.push_back(ss.str());
    }
    if (s.critDamage > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.critDamage << "% CritDmg";
        parts.push_back(ss.str());
    }
    if (s.lifestealPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.lifestealPercent << "% Lifesteal";
        parts.push_back(ss.str());
    }
    if (s.armorPenetration > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.armorPenetration << "% Pen.";
        parts.push_back(ss.str());
    }
    if (s.cooldownReductionPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.cooldownReductionPercent << "% CDR";
        parts.push_back(ss.str());
    }
    if (s.attackPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.attackPercent << "% Atq Físico";
        parts.push_back(ss.str());
    }
    if (s.defensePercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.defensePercent << "% Def";
        parts.push_back(ss.str());
    }
    if (s.hpPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.hpPercent << "% HP";
        parts.push_back(ss.str());
    }
    if (s.mpPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.mpPercent << "% MP";
        parts.push_back(ss.str());
    }
    if (s.strengthPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.strengthPercent << "% STR";
        parts.push_back(ss.str());
    }
    if (s.agilityPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.agilityPercent << "% AGI";
        parts.push_back(ss.str());
    }
    if (s.intelligencePercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.intelligencePercent << "% INT";
        parts.push_back(ss.str());
    }
    if (s.vitalityPercent > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.vitalityPercent << "% VIT";
        parts.push_back(ss.str());
    }
    if (s.physicalDamageBonus > 0.001f) {
        std::stringstream ss;
        ss << "+" << std::fixed << std::setprecision(1) << s.physicalDamageBonus << "% Bonus Daño Físico";
        parts.push_back(ss.str());
    }
    
    std::string res = "";
    for (size_t i = 0; i < parts.size(); ++i) {
        res += parts[i];
        if (i + 1 < parts.size()) res += ", ";
    }
    return res;
}

void Tooltip::addStatLine(std::vector<TooltipLine> &targetLines,
                          const std::string &label, float value, bool isInt,
                          bool isPercent, sf::Color valueColor) {
  if (std::abs(value) <= 0.001f)
    return;

  TooltipLine line;
  std::stringstream ss;
  ss << label << ": ";

  if (isInt)
    ss << (int)value;
  else
    ss << std::fixed << std::setprecision(1) << value;

  if (isPercent)
    ss << "%";

  line.parts.push_back({ss.str(), valueColor});
  targetLines.push_back(line);
}

void Tooltip::addComparisonStatLine(const std::string &label, float value,
                                    float equippedValue, bool isInt,
                                    bool isPercent, bool isLowerBetter,
                                    sf::Color textColor, int fortLevel,
                                    float baseValue, int eqFortLevel,
                                    float eqBaseValue) {
  if (value <= 0.001f && equippedValue <= 0.001f)
    return;

  // 1. Main Tooltip (Hovered Item) - Shows Value AND Diff
  TooltipLine mainLine;
  {
    std::stringstream ss;
    ss << label << ": ";
    if (value > 0.001f) {
      if (fortLevel > 0 && baseValue > 0.001f && value > baseValue) {
        int bonus = (int)std::round(value - baseValue);
        if (isInt)
          ss << (int)baseValue << " +" << bonus;
        else
          ss << std::fixed << std::setprecision(1) << baseValue << " +"
             << bonus;
      } else {
        if (isInt)
          ss << (int)value;
        else
          ss << std::fixed << std::setprecision(1) << value;
      }
      if (isPercent)
        ss << "%";
    } else {
      ss << "-";
    }
    mainLine.parts.push_back({ss.str(), textColor});

    // Diff Logic
    if (equippedValue > -9990.f && mShowComparison) {
      float diff = value - equippedValue;
      if (isDiffSignificant(diff)) {
        std::stringstream ssDiff;
        bool positive = diff > 0;
        ssDiff << " (";
        if (positive)
          ssDiff << "+";
        if (isInt)
          ssDiff << (int)diff;
        else
          ssDiff << std::fixed << std::setprecision(1) << diff;
        if (isPercent)
          ssDiff << "%";
        ssDiff << ")";

        sf::Color diffColor;
        if (isLowerBetter)
          diffColor = (diff < 0) ? sf::Color::Green : sf::Color::Red;
        else
          diffColor = (diff > 0) ? sf::Color::Green : sf::Color::Red;

        mainLine.parts.push_back({ssDiff.str(), diffColor});
      }
    }
  }
  if (value > 0.001f || (equippedValue > -9990.f && mShowComparison)) {
    mLines.push_back(mainLine);
  }

  // 2. Equipped Tooltip (Comparison) - Shows ONLY Raw Value
  if (mShowComparison && equippedValue > 0.001f) {
    TooltipLine eqLine;
    std::stringstream ssEq;
    ssEq << label << ": ";
    if (eqFortLevel > 0 && eqBaseValue > 0.001f &&
        equippedValue > eqBaseValue) {
      int bonus = (int)std::round(equippedValue - eqBaseValue);
      if (isInt)
        ssEq << (int)eqBaseValue << " +" << bonus;
      else
        ssEq << std::fixed << std::setprecision(1) << eqBaseValue << " +"
             << bonus;
    } else {
      if (isInt)
        ssEq << (int)equippedValue;
      else
        ssEq << std::fixed << std::setprecision(1) << equippedValue;
    }
    if (isPercent)
      ssEq << "%";

    sf::Color eqColor =
        (textColor == sf::Color::White) ? sf::Color(200, 200, 200) : textColor;
    eqLine.parts.push_back({ssEq.str(), eqColor});
    mEquippedLines.push_back(eqLine);
  }
}

void Tooltip::calculateSize(const std::vector<TooltipLine> &lines,
                            sf::RectangleShape &bg, const Item* panelItem) {
  float maxWidth = 0.f;
  float totalHeight = PADDING * 2.f;
  BitmapText measureText;
  measureText.setTexture(mFontTexture);
  measureText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});

  for (const auto &line : lines) {
    float lineWidth = 0.f;
    float lineHeight = 0.f;

    if (line.parts.empty()) {
      totalHeight += 8.f;
      continue;
    }

    for (const auto &part : line.parts) {
      measureText.setString(part.text);
      sf::FloatRect b = measureText.getGlobalBounds();
      lineWidth += b.size.x;
      float h = b.size.y;
      if (h > lineHeight)
        lineHeight = h;
    }
    if (lineWidth > maxWidth)
      maxWidth = lineWidth;
    totalHeight += lineHeight + LINE_SPACING;
  }

  // Add vertical sockets space if item has sockets
  if (panelItem && panelItem->maxSockets > 0) {
      float socketRowHeight = 16.f;
      totalHeight += panelItem->maxSockets * socketRowHeight + 8.f;
      
      BitmapText socketMeasureText;
      socketMeasureText.setTexture(mFontTexture);
      socketMeasureText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
      
      for (int i = 0; i < panelItem->maxSockets; ++i) {
          std::string textStr = "";
          if (i < (int)panelItem->socketedStones.size() && panelItem->socketedStones[i]) {
              textStr = panelItem->socketedStones[i]->name + " (" + getStoneStatsString(*panelItem->socketedStones[i]) + ")";
          }
          if (!textStr.empty()) {
              socketMeasureText.setString(textStr);
              float w = 14.f + 8.f + socketMeasureText.getGlobalBounds().size.x;
              if (w > maxWidth) {
                  maxWidth = w;
              }
          } else {
              if (14.f > maxWidth) {
                  maxWidth = 14.f;
              }
          }
      }
  }

  bg.setSize({maxWidth + PADDING * 2.f, totalHeight});
}

void Tooltip::setPosition(sf::Vector2f position, sf::Vector2u windowSize) {
  sf::Vector2f padding(15.f, 15.f);
  sf::Vector2f startPos = position + padding;

  sf::Vector2f sizeMain = mBackground.getSize();
  sf::Vector2f sizeEq =
      mShowComparison ? mEquippedBackground.getSize() : sf::Vector2f(0.f, 0.f);

  float totalWidth = sizeMain.x;
  if (mShowComparison)
    totalWidth += sizeEq.x + 10.f;

  if (startPos.x + totalWidth > windowSize.x) {
    startPos.x = position.x - totalWidth - 15.f;
  }

  float maxHeight = std::max(sizeMain.y, sizeEq.y);
  if (startPos.y + maxHeight > windowSize.y) {
    startPos.y = position.y - maxHeight - 15.f;
  }

  if (startPos.x < 0.f)
    startPos.x = 0.f;
  if (startPos.y < 0.f)
    startPos.y = 0.f;

  mBackground.setPosition(startPos);

  if (mShowComparison) {
    sf::Vector2f eqPos = startPos;
    eqPos.x += sizeMain.x + 10.f;
    mEquippedBackground.setPosition(eqPos);
  }
}

void Tooltip::hide() {
  mVisible = false;
  mCurrentItem = nullptr;
  mCurrentEquippedItem = nullptr;
}
