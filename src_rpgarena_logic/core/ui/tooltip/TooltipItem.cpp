#include "Tooltip.h"
#include <iomanip>
#include <sstream>

void Tooltip::populateItemContent(const Item &item,
                                  std::vector<Tooltip::TooltipLine> &lines,
                                  bool isEquippedHeader) {
  Tooltip::TooltipLine header;
  header.parts.push_back({item.name, sf::Color::White});
  if (item.stackCount > 1) {
      header.parts.push_back({" x" + std::to_string(item.stackCount), sf::Color(255, 235, 40)});
  }
  lines.push_back(header);

  if (isEquippedHeader) {
    Tooltip::TooltipLine subHeader;
    subHeader.parts.push_back({"[Equipo Actual]", sf::Color(100, 255, 100)});
    lines.push_back(subHeader);
  }

  // Tipo
  std::string typeStr = "Desconocido";
  switch (item.type) {
  case ItemType::Weapon:
    typeStr = "Arma";
    if (item.gripType == GripType::TwoHanded) {
      typeStr += " (2 manos)";
    } else {
      typeStr += " (1 mano)";
    }
    break;
  case ItemType::Armor:
    typeStr = "Armadura";
    switch (item.slotType) {
      case EquipmentSlot::Head: typeStr += " (Casco)"; break;
      case EquipmentSlot::Cape: typeStr += " (Capa)"; break;
      case EquipmentSlot::Chest: typeStr += " (Pechera)"; break;
      case EquipmentSlot::Hands: typeStr += " (Guantes)"; break;
      case EquipmentSlot::Legs: typeStr += " (Pantalon)"; break;
      case EquipmentSlot::Feet: typeStr += " (Botas)"; break;
      default: break;
    }
    break;
  case ItemType::Ring:
    typeStr = "Anillo";
    break;
  case ItemType::Potion:
    typeStr = "Pocion";
    break;
  case ItemType::Misc:
    typeStr = "Miscelaneo";
    break;
  }

  Tooltip::TooltipLine typeLine;
  typeLine.parts.push_back({"Tipo: " + typeStr, sf::Color(200, 200, 200)});
  lines.push_back(typeLine);

  // Calidad
  std::string qualStr = getQualityDisplayName(item.quality);
  sf::Color qualColor = getQualityColor(item.quality);
  if (qualColor == sf::Color::Transparent) {
      qualColor = sf::Color(220, 220, 220);
  }

  Tooltip::TooltipLine qualLine;
  qualLine.parts.push_back({"Calidad: " + qualStr, qualColor});
  lines.push_back(qualLine);

  // Nivel
  Tooltip::TooltipLine lvlLine;
  lvlLine.parts.push_back(
      {"Nivel: " + std::to_string(item.level), sf::Color(220, 220, 220)});
  lines.push_back(lvlLine);

  // Spacer
  lines.push_back(Tooltip::TooltipLine());
}

void Tooltip::show(const Item &item, sf::Vector2f position,
                   sf::Vector2u windowSize, const Item *equippedItem) {
  mVisible = true;
  mCurrentItem = &item;
  mCurrentEquippedItem = equippedItem;
  mLines.clear();
  mEquippedLines.clear();

  mShowComparison = (equippedItem != nullptr && equippedItem != &item);

  // --- 1. Populate HEADER Info Separately ---
  populateItemContent(item, mLines, false);
  if (mShowComparison) {
    populateItemContent(*equippedItem, mEquippedLines, true);
  }

  // --- 2. Populate STATS (Shared Logic) ---
  const auto &s = item.stats;
  const ItemStats *eqS = mShowComparison ? &equippedItem->stats : nullptr;

  addComparisonStatLine(
      "Daño Fisico", s.physicalDamage,
      eqS ? (float)eqS->physicalDamage : -9999.f, true, false, false,
      sf::Color::White, item.fortificationLevel, (float)item.basePhysicalDamage,
      equippedItem ? equippedItem->fortificationLevel : 0,
      equippedItem ? (float)equippedItem->basePhysicalDamage : 0.f);

  addComparisonStatLine("Vel. Ataque", s.attackSpeed,
                        eqS ? eqS->attackSpeed : -9999.f, false, false,
                        false);
  addComparisonStatLine("Vel. Mov", s.moveSpeedBonus,
                        eqS ? eqS->moveSpeedBonus : -9999.f, true, false,
                        false);

  addComparisonStatLine("Defensa", s.defense,
                        eqS ? (float)eqS->defense : -9999.f, true, false,
                        false, sf::Color::White, item.fortificationLevel,
                        (float)item.baseDefense,
                        equippedItem ? equippedItem->fortificationLevel : 0,
                        equippedItem ? (float)equippedItem->baseDefense : 0.f);

  addComparisonStatLine("STR", (float)s.strength,
                        eqS ? (float)eqS->strength : -9999.f, true, false,
                        false);
  addComparisonStatLine("AGI", (float)s.agility,
                        eqS ? (float)eqS->agility : -9999.f, true, false,
                        false);
  addComparisonStatLine("INT", (float)s.intelligence,
                        eqS ? (float)eqS->intelligence : -9999.f, true, false,
                        false);
  addComparisonStatLine("VIT", (float)s.vitality,
                        eqS ? (float)eqS->vitality : -9999.f, true, false,
                        false);

  addComparisonStatLine("HP", s.hpBonus, eqS ? (float)eqS->hpBonus : -9999.f,
                        true, false, false);
  addComparisonStatLine("MP", s.mpBonus, eqS ? (float)eqS->mpBonus : -9999.f,
                        true, false, false);

  addComparisonStatLine("Pen. Armadura", s.armorPenetration,
                        eqS ? (float)eqS->armorPenetration : -9999.f, false,
                        true, false);

  // [SPECIAL WEAPON/ARMOR STATS]
  bool hasSpecial =
      (s.lifestealPercent > 0.001f || s.stunChance > 0.001f || s.bleedFlat > 0 ||
       s.bleedPercent > 0.001f || s.slowMovePercent > 0.001f ||
       s.slowAttackPercent > 0.001f || s.aoeRadius > 0.001f ||
       s.cooldownReductionPercent > 0.001f ||
       s.damageReduction > 0.001f || s.tenacity > 0.001f ||
       s.critAvoidance > 0.001f || s.evasion > 0 ||
       s.thornsPercent > 0.001f || s.hpRegenPercent > 0.001f ||
       s.mpRegenPercent > 0.001f || s.blockChance > 0.001f ||
       s.blockValuePercent > 0.001f || s.antiArmorPenPercent > 0.001f ||
       s.antiArmorPenFlat > 0 || s.manaStealPercent > 0.001f ||
       s.xpBonusPercent > 0.001f || s.critChance > 0.001f ||
       s.critDamage > 0.001f);

  bool eqHasSpecial = false;
  if (eqS) {
      eqHasSpecial =
          (eqS->lifestealPercent > 0.001f || eqS->stunChance > 0.001f || eqS->bleedFlat > 0 ||
           eqS->bleedPercent > 0.001f || eqS->slowMovePercent > 0.001f ||
           eqS->slowAttackPercent > 0.001f || eqS->aoeRadius > 0.001f ||
           eqS->cooldownReductionPercent > 0.001f ||
           eqS->damageReduction > 0.001f || eqS->tenacity > 0.001f ||
           eqS->critAvoidance > 0.001f || eqS->evasion > 0 ||
           eqS->thornsPercent > 0.001f || eqS->hpRegenPercent > 0.001f ||
           eqS->mpRegenPercent > 0.001f || eqS->blockChance > 0.001f ||
           eqS->blockValuePercent > 0.001f || eqS->antiArmorPenPercent > 0.001f ||
           eqS->antiArmorPenFlat > 0 || eqS->manaStealPercent > 0.001f ||
           eqS->xpBonusPercent > 0.001f || eqS->critChance > 0.001f ||
           eqS->critDamage > 0.001f);
  }

  if (hasSpecial || eqHasSpecial) {
    mLines.push_back(Tooltip::TooltipLine());
    if (mShowComparison) {
      mEquippedLines.push_back(Tooltip::TooltipLine());
    }
  }

  sf::Color specialColor = sf::Color(0, 200, 255);

  addComparisonStatLine("Robo de Vida", s.lifestealPercent,
                        eqS ? eqS->lifestealPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Prob. Aturdir", s.stunChance,
                        eqS ? eqS->stunChance : -9999.f, false, true, false,
                        specialColor);
  addComparisonStatLine("Dur. Aturdir", s.stunDuration,
                        eqS ? eqS->stunDuration : -9999.f, false, false, false,
                        specialColor);
  if (s.bleedFlat > 0 || (eqS && eqS->bleedFlat > 0)) {
    addComparisonStatLine("Sangrado Plano", (float)s.bleedFlat,
                          eqS ? (float)eqS->bleedFlat : -9999.f, true, false,
                          false, specialColor);
  }
  if (s.bleedPercent > 0.001f || (eqS && eqS->bleedPercent > 0.001f)) {
    addComparisonStatLine("Sangrado %", s.bleedPercent,
                          eqS ? eqS->bleedPercent : -9999.f, false, true, false,
                          specialColor);
  }
  if (s.bleedDurationFlat > 0.001f || s.bleedDurationPercent > 0.001f ||
      (eqS && (eqS->bleedDurationFlat > 0.001f ||
               eqS->bleedDurationPercent > 0.001f))) {
    float bDur = s.bleedDurationFlat > 0.f ? s.bleedDurationFlat
                                           : s.bleedDurationPercent;
    float eqBDur = -9999.f;
    if (eqS) {
      eqBDur = eqS->bleedDurationFlat > 0.f ? eqS->bleedDurationFlat
                                            : eqS->bleedDurationPercent;
    }
    addComparisonStatLine("Dur. Sangrado", bDur, eqBDur, false, false, false,
                          specialColor);
  }
  if (s.slowMovePercent > 0.001f || (eqS && eqS->slowMovePercent > 0.001f)) {
    addComparisonStatLine("Ralentizar Mov.", s.slowMovePercent,
                          eqS ? eqS->slowMovePercent : -9999.f, false, true,
                          false, specialColor);
    addComparisonStatLine("Dur. Ral. Mov.", s.slowMoveDuration,
                          eqS ? eqS->slowMoveDuration : -9999.f, false, false,
                          false, specialColor);
  }
  if (s.slowAttackPercent > 0.001f ||
      (eqS && eqS->slowAttackPercent > 0.001f)) {
    addComparisonStatLine("Ralentizar Atk", s.slowAttackPercent,
                          eqS ? eqS->slowAttackPercent : -9999.f, false, true,
                          false, specialColor);
    addComparisonStatLine("Dur. Ral. Atk.", s.slowAttackDuration,
                          eqS ? eqS->slowAttackDuration : -9999.f, false, false,
                          false, specialColor);
  }
  addComparisonStatLine("Radio de AoE", s.aoeRadius,
                        eqS ? eqS->aoeRadius : -9999.f, true, false, false,
                        specialColor);
  addComparisonStatLine("Daño de AoE", s.aoeDamagePercent,
                        eqS ? eqS->aoeDamagePercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Reducción Enfriamiento", s.cooldownReductionPercent,
                        eqS ? eqS->cooldownReductionPercent : -9999.f, false, true,
                        false, specialColor);

  addComparisonStatLine("Atq Físico %", s.attackPercent,
                        eqS ? eqS->attackPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Defensa %", s.defensePercent,
                        eqS ? eqS->defensePercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("HP %", s.hpPercent,
                        eqS ? eqS->hpPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("MP %", s.mpPercent,
                        eqS ? eqS->mpPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("STR %", s.strengthPercent,
                        eqS ? eqS->strengthPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("AGI %", s.agilityPercent,
                        eqS ? eqS->agilityPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("INT %", s.intelligencePercent,
                        eqS ? eqS->intelligencePercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("VIT %", s.vitalityPercent,
                        eqS ? eqS->vitalityPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Bonus Daño Físico", s.physicalDamageBonus,
                        eqS ? eqS->physicalDamageBonus : -9999.f, false, true,
                        false, specialColor);

  addComparisonStatLine("Critico", s.critChance,
                        eqS ? eqS->critChance : -9999.f, false, true, false,
                        specialColor);
  addComparisonStatLine("Daño Crit", s.critDamage,
                        eqS ? eqS->critDamage : -9999.f, false, true, false,
                        specialColor);

  addComparisonStatLine("Reducción de Daño", s.damageReduction,
                        eqS ? eqS->damageReduction : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Tenacidad", s.tenacity,
                        eqS ? eqS->tenacity : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Resist. Critico", s.critAvoidance,
                        eqS ? eqS->critAvoidance : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Evasión", (float)s.evasion,
                        eqS ? (float)eqS->evasion : -9999.f, true, false,
                        false, specialColor);
  addComparisonStatLine("Espinas", s.thornsPercent,
                        eqS ? eqS->thornsPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Regeneración HP", s.hpRegenPercent,
                        eqS ? eqS->hpRegenPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Regeneración MP", s.mpRegenPercent,
                        eqS ? eqS->mpRegenPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Prob. Bloqueo", s.blockChance,
                        eqS ? eqS->blockChance : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Valor Bloqueo", s.blockValuePercent,
                        eqS ? eqS->blockValuePercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Anti Pen. Defensa %", s.antiArmorPenPercent,
                        eqS ? eqS->antiArmorPenPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Anti Pen. Defensa plana", (float)s.antiArmorPenFlat,
                        eqS ? (float)eqS->antiArmorPenFlat : -9999.f, true, false,
                        false, specialColor);
  addComparisonStatLine("Robo de Maná", s.manaStealPercent,
                        eqS ? eqS->manaStealPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Bono Exp", s.xpBonusPercent,
                        eqS ? eqS->xpBonusPercent : -9999.f, false, true,
                        false, specialColor);
  addComparisonStatLine("Precisión", (float)s.accuracy,
                        eqS ? (float)eqS->accuracy : -9999.f, true, false,
                        false, specialColor);
  addComparisonStatLine("Daño Verdadero", (float)s.trueDamagePercent,
                        eqS ? (float)eqS->trueDamagePercent : -9999.f, true, true,
                        false, specialColor);
  addComparisonStatLine("Daño Ejecución", (float)s.executeDamagePercent,
                        eqS ? (float)eqS->executeDamagePercent : -9999.f, true, true,
                        false, specialColor);
  addComparisonStatLine("Umbral Ejecución", (float)s.executeHealthThresholdPercent,
                        eqS ? (float)eqS->executeHealthThresholdPercent : -9999.f, true, true,
                        false, specialColor);

  // --- CULTIVO SYSTEM LINES ---
  auto addCultivoLines = [&](const Item& itm, std::vector<TooltipLine>& lines) {
    if (itm.cultivoLevel > 0 || !itm.cultivoSelectedStats.empty()) {
      lines.push_back(TooltipLine());
      TooltipLine headerL;
      headerL.parts.push_back({"Cultivo Nivel " + std::to_string(itm.cultivoLevel), sf::Color(0, 220, 255)});
      lines.push_back(headerL);

      sf::Color cultivoStatColor = sf::Color(100, 255, 200);
      const auto& cs = itm.cultivoBonusStats;
      for (const auto& statId : itm.cultivoSelectedStats) {
        std::stringstream ss;
        if (statId == "STR" || statId == "strength") ss << "STR: +" << cs.strength;
        else if (statId == "AGI" || statId == "agility") ss << "AGI: +" << cs.agility;
        else if (statId == "INT" || statId == "intelligence") ss << "INT: +" << cs.intelligence;
        else if (statId == "VIT" || statId == "vitality") ss << "VIT: +" << cs.vitality;
        else if (statId == "physicalDamage" || statId == "attack") ss << "Daño Físico: +" << cs.physicalDamage;
        else if (statId == "defense") ss << "Defensa: +" << cs.defense;
        else if (statId == "hpBonus" || statId == "hp") ss << "HP: +" << cs.hpBonus;
        else if (statId == "mpBonus" || statId == "mp") ss << "MP: +" << cs.mpBonus;
        else if (statId == "critChance") ss << "Prob. Crítico: +" << std::fixed << std::setprecision(1) << cs.critChance << "%";
        else if (statId == "critDamage") ss << "Daño Crítico: +" << std::fixed << std::setprecision(1) << cs.critDamage << "%";
        else if (statId == "lifestealPercent") ss << "Robo de Vida: +" << std::fixed << std::setprecision(1) << cs.lifestealPercent << "%";
        else if (statId == "cooldownReductionPercent") ss << "Red. Enfriamiento: +" << std::fixed << std::setprecision(1) << cs.cooldownReductionPercent << "%";
        else if (statId == "attackPercent") ss << "Atq Físico: +" << std::fixed << std::setprecision(1) << cs.attackPercent << "%";
        else if (statId == "defensePercent") ss << "Defensa: +" << std::fixed << std::setprecision(1) << cs.defensePercent << "%";
        else if (statId == "hpPercent") ss << "Vida Máx: +" << std::fixed << std::setprecision(1) << cs.hpPercent << "%";
        else if (statId == "physicalDamageBonus") ss << "Bonus Daño Físico: +" << std::fixed << std::setprecision(1) << cs.physicalDamageBonus << "%";
        else if (statId == "armorPenetration") ss << "Pen. Armadura: +" << std::fixed << std::setprecision(1) << cs.armorPenetration << "%";

        if (!ss.str().empty()) {
          TooltipLine statL;
          statL.parts.push_back({ss.str(), cultivoStatColor});
          lines.push_back(statL);
        }
      }
    }
  };

  addCultivoLines(item, mLines);
  if (equippedItem) {
    addCultivoLines(*equippedItem, mEquippedLines);
  }

  calculateSize(mLines, mBackground, mCurrentItem);
  if (mShowComparison) {
    calculateSize(mEquippedLines, mEquippedBackground, mCurrentEquippedItem);
  }

  setPosition(position, windowSize);
}
