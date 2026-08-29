#include "ItemManager.h"
#include <iostream>
#include <memory>
#include "utils/Random.h"
#include "WeaponsFactory.h"
#include "ItemFactory.h"
#include "../systems/StoneSystem.h"
#include "Config.h"
#include "utils/TinyJson.h"

ItemManager::ItemManager() {
    loadTemplates();
}

void ItemManager::loadTemplates() {
    if (mLoaded) return;

    // 1. Cargar Atlases
    json::Value rootAtl = json::parseFile("assets/data/atlases.json");
    if (rootAtl.type == json::Type::Object) {
        auto rootObj = rootAtl.asObject();
        if (rootObj.count("atlases") && rootObj.at("atlases").type == json::Type::Object) {
            auto atlObj = rootObj.at("atlases").asObject();
            for (const auto& pair : atlObj) {
                if (pair.second.type == json::Type::Object) {
                    auto infoObj = pair.second.asObject();
                    AtlasInfo info;
                    if (infoObj.count("path")) info.path = infoObj.at("path").asString();
                    if (infoObj.count("cell_size")) info.cellSize = infoObj.at("cell_size").asInt();
                    if (infoObj.count("default_scale")) info.defaultScale = (float)infoObj.at("default_scale").asDouble();
                    if (infoObj.count("type")) info.type = infoObj.at("type").asString();
                    if (infoObj.count("offset") && infoObj.at("offset").type == json::Type::Array) {
                        auto offArr = infoObj.at("offset").asArray();
                        if (offArr.size() >= 2) {
                            info.offset.x = (float)offArr[0].asDouble();
                            info.offset.y = (float)offArr[1].asDouble();
                        }
                    }
                    if (infoObj.count("guard_offset") && infoObj.at("guard_offset").type == json::Type::Array) {
                        auto gOffArr = infoObj.at("guard_offset").asArray();
                        if (gOffArr.size() >= 2) {
                            info.guardOffset.x = (float)gOffArr[0].asDouble();
                            info.guardOffset.y = (float)gOffArr[1].asDouble();
                        }
                    }
                    mAtlases[pair.first] = info;
                }
            }
        }
    } else {
        std::cerr << "[ItemManager] ERROR: No se pudo cargar o parsear assets/data/atlases.json\n";
    }

    // Helper para parsear stats de una plantilla procedural
    auto parseProcedural = [&](const json::Object& itemObj) -> ProceduralTemplate {
        ProceduralTemplate t;
        if (itemObj.count("id")) t.id = itemObj.at("id").asString();
        if (itemObj.count("name")) t.name = itemObj.at("name").asString();
        if (itemObj.count("atlas")) t.atlas = itemObj.at("atlas").asString();
        if (itemObj.count("grid_pos") && itemObj.at("grid_pos").type == json::Type::Array) {
            auto posArr = itemObj.at("grid_pos").asArray();
            if (posArr.size() >= 2) {
                t.gridPos.x = posArr[0].asInt();
                t.gridPos.y = posArr[1].asInt();
            }
        }
        if (itemObj.count("grip")) {
            std::string gripStr = itemObj.at("grip").asString();
            t.grip = (gripStr == "TwoHanded") ? GripType::TwoHanded : GripType::OneHanded;
        }
        if (itemObj.count("slot")) {
            std::string slotStr = itemObj.at("slot").asString();
            if (slotStr == "Head") t.slot = EquipmentSlot::Head;
            else if (slotStr == "Cape") t.slot = EquipmentSlot::Cape;
            else if (slotStr == "Chest") t.slot = EquipmentSlot::Chest;
            else if (slotStr == "Hands") t.slot = EquipmentSlot::Hands;
            else if (slotStr == "Legs") t.slot = EquipmentSlot::Legs;
            else if (slotStr == "Feet") t.slot = EquipmentSlot::Feet;
            else if (slotStr == "Ring" || slotStr == "Ring1") t.slot = EquipmentSlot::Ring1;
            else if (slotStr == "Ring2") t.slot = EquipmentSlot::Ring2;
        }
        
        // Atributos base (leídos de la raíz o de la sub-estructura "stats")
        auto getInt = [&](const std::string& key) -> int {
            if (itemObj.count(key)) return itemObj.at(key).asInt();
            if (itemObj.count("stats") && itemObj.at("stats").type == json::Type::Object) {
                auto s = itemObj.at("stats").asObject();
                if (s.count(key)) return s.at(key).asInt();
            }
            return 0;
        };

        auto getDouble = [&](const std::string& key, double defaultVal = 0.0) -> double {
            if (itemObj.count(key)) return itemObj.at(key).asDouble();
            if (itemObj.count("stats") && itemObj.at("stats").type == json::Type::Object) {
                auto s = itemObj.at("stats").asObject();
                if (s.count(key)) return s.at(key).asDouble();
            }
            return defaultVal;
        };

        t.physicalDamage = getInt("physical_damage");
        if (t.physicalDamage == 0) t.physicalDamage = getInt("physicalDamage");
        t.attackSpeed = (float)getDouble("attack_speed", 1.0);
        if (t.attackSpeed == 1.0f) t.attackSpeed = (float)getDouble("attackSpeed", 1.0);
        t.range = (float)getDouble("range", 50.0);
        t.defense = getInt("defense");
        t.vitality = getInt("vitality");
        t.strength = getInt("strength");
        t.agility = getInt("dexterity");
        if (t.agility == 0) t.agility = getInt("agility");
        t.intelligence = getInt("intelligence");
        t.hpBonus = getInt("hpBonus");
        t.mpBonus = getInt("mpBonus");
        
        t.critChance = (float)getDouble("critChance");
        if (t.critChance == 0.f) t.critChance = (float)getDouble("criticalChance");
        t.critDamage = (float)getDouble("critDamage");
        t.lifestealPercent = (float)getDouble("lifestealPercent");
        t.armorPenetration = (float)getDouble("armorPenetration");
        t.cooldownReductionPercent = (float)getDouble("cooldownReductionPercent");
        
        t.attackPercent = (float)getDouble("attackPercent");
        if (t.attackPercent == 0.f) t.attackPercent = (float)getDouble("physicalAttackPercent");
        if (t.attackPercent == 0.f) t.attackPercent = (float)getDouble("atkPercent");
        t.defensePercent = (float)getDouble("defensePercent");
        if (t.defensePercent == 0.f) t.defensePercent = (float)getDouble("defPercent");
        t.hpPercent = (float)getDouble("hpPercent");
        if (t.hpPercent == 0.f) t.hpPercent = (float)getDouble("maxHpPercent");
        t.mpPercent = (float)getDouble("mpPercent");
        if (t.mpPercent == 0.f) t.mpPercent = (float)getDouble("maxMpPercent");
        t.strengthPercent = (float)getDouble("strengthPercent");
        if (t.strengthPercent == 0.f) t.strengthPercent = (float)getDouble("strPercent");
        t.agilityPercent = (float)getDouble("agilityPercent");
        if (t.agilityPercent == 0.f) t.agilityPercent = (float)getDouble("agiPercent");
        if (t.agilityPercent == 0.f) t.agilityPercent = (float)getDouble("dexPercent");
        t.intelligencePercent = (float)getDouble("intelligencePercent");
        if (t.intelligencePercent == 0.f) t.intelligencePercent = (float)getDouble("intPercent");
        t.vitalityPercent = (float)getDouble("vitalityPercent");
        if (t.vitalityPercent == 0.f) t.vitalityPercent = (float)getDouble("vitPercent");
        t.physicalDamageBonus = (float)getDouble("physicalDamageBonus");
        if (t.physicalDamageBonus == 0.f) t.physicalDamageBonus = (float)getDouble("physicalDamagePercent");
        
        if (itemObj.count("scale")) t.scale = (float)itemObj.at("scale").asDouble();
        if (itemObj.count("offset") && itemObj.at("offset").type == json::Type::Array) {
            auto offArr = itemObj.at("offset").asArray();
            if (offArr.size() >= 2) {
                t.offset.x = (float)offArr[0].asDouble();
                t.offset.y = (float)offArr[1].asDouble();
            }
        }

        return t;
    };

    // Helper para parsear stats de un item unico fijo
    auto parseUnique = [&](const std::string& key, const json::Object& itemObj, ItemType type) {
        FixedUniqueTemplate t;
        t.type = type;
        if (itemObj.count("id")) t.id = itemObj.at("id").asString();
        else t.id = key;
        if (itemObj.count("name")) t.name = itemObj.at("name").asString();
        if (itemObj.count("quality")) t.quality = itemObj.at("quality").asString();
        if (itemObj.count("texturePath")) t.texturePath = itemObj.at("texturePath").asString();
        if (itemObj.count("atlas")) t.atlas = itemObj.at("atlas").asString();
        if (itemObj.count("grid_pos") && itemObj.at("grid_pos").type == json::Type::Array) {
            auto posArr = itemObj.at("grid_pos").asArray();
            if (posArr.size() >= 2) {
                t.gridPos.x = posArr[0].asInt();
                t.gridPos.y = posArr[1].asInt();
            }
        }
        if (itemObj.count("grip")) {
            std::string gripStr = itemObj.at("grip").asString();
            t.grip = (gripStr == "TwoHanded") ? GripType::TwoHanded : GripType::OneHanded;
        }
        if (itemObj.count("slot")) {
            std::string slotStr = itemObj.at("slot").asString();
            if (slotStr == "Head") t.slot = EquipmentSlot::Head;
            else if (slotStr == "Cape") t.slot = EquipmentSlot::Cape;
            else if (slotStr == "Chest") t.slot = EquipmentSlot::Chest;
            else if (slotStr == "Hands") t.slot = EquipmentSlot::Hands;
            else if (slotStr == "Legs") t.slot = EquipmentSlot::Legs;
            else if (slotStr == "Feet") t.slot = EquipmentSlot::Feet;
            else if (slotStr == "Ring" || slotStr == "Ring1") t.slot = EquipmentSlot::Ring1;
            else if (slotStr == "Ring2") t.slot = EquipmentSlot::Ring2;
            else if (slotStr == "OffHand") t.slot = EquipmentSlot::OffHand;
            else if (slotStr == "MainHand") t.slot = EquipmentSlot::MainHand;
        }
        if (itemObj.count("scale")) t.scale = (float)itemObj.at("scale").asDouble();
        if (itemObj.count("offset") && itemObj.at("offset").type == json::Type::Array) {
            auto offArr = itemObj.at("offset").asArray();
            if (offArr.size() >= 2) {
                t.offset.x = (float)offArr[0].asDouble();
                t.offset.y = (float)offArr[1].asDouble();
            }
        }
        if (itemObj.count("guard_offset") && itemObj.at("guard_offset").type == json::Type::Array) {
            auto gOffArr = itemObj.at("guard_offset").asArray();
            if (gOffArr.size() >= 2) {
                t.guardOffset.x = (float)gOffArr[0].asDouble();
                t.guardOffset.y = (float)gOffArr[1].asDouble();
            }
        } else if (itemObj.count("guardOffset") && itemObj.at("guardOffset").type == json::Type::Array) {
            auto gOffArr = itemObj.at("guardOffset").asArray();
            if (gOffArr.size() >= 2) {
                t.guardOffset.x = (float)gOffArr[0].asDouble();
                t.guardOffset.y = (float)gOffArr[1].asDouble();
            }
        }

        if (itemObj.count("value")) t.stats.value = itemObj.at("value").asInt();

        // Stats fijos
        if (itemObj.count("stats") && itemObj.at("stats").type == json::Type::Object) {
            auto sObj = itemObj.at("stats").asObject();
            if (sObj.count("physicalDamage")) t.stats.physicalDamage = sObj.at("physicalDamage").asInt();
            if (sObj.count("attackSpeed")) t.stats.attackSpeed = (float)sObj.at("attackSpeed").asDouble();
            if (sObj.count("range")) t.stats.range = (float)sObj.at("range").asDouble();
            if (sObj.count("defense")) t.stats.defense = sObj.at("defense").asInt();
            
            if (sObj.count("strength")) t.stats.strength = sObj.at("strength").asInt();
            if (sObj.count("dexterity")) t.stats.agility = sObj.at("dexterity").asInt();
            else if (sObj.count("agility")) t.stats.agility = sObj.at("agility").asInt();
            if (sObj.count("intelligence")) t.stats.intelligence = sObj.at("intelligence").asInt();
            if (sObj.count("vitality")) t.stats.vitality = sObj.at("vitality").asInt();
            if (sObj.count("hpBonus")) t.stats.hpBonus = sObj.at("hpBonus").asInt();
            if (sObj.count("mpBonus")) t.stats.mpBonus = sObj.at("mpBonus").asInt();

            if (sObj.count("critChance")) t.stats.critChance = (float)sObj.at("critChance").asDouble();
            else if (sObj.count("criticalChance")) t.stats.critChance = (float)sObj.at("criticalChance").asDouble();
            if (sObj.count("critDamage")) t.stats.critDamage = (float)sObj.at("critDamage").asDouble();
            if (sObj.count("lifestealPercent")) t.stats.lifestealPercent = (float)sObj.at("lifestealPercent").asDouble();
            if (sObj.count("stunChance")) t.stats.stunChance = (float)sObj.at("stunChance").asDouble();
            if (sObj.count("stunDuration")) t.stats.stunDuration = (float)sObj.at("stunDuration").asDouble();
            if (sObj.count("bleedFlat")) t.stats.bleedFlat = sObj.at("bleedFlat").asInt();
            if (sObj.count("bleedPercent")) t.stats.bleedPercent = (float)sObj.at("bleedPercent").asDouble();
            if (sObj.count("bleedDurationFlat")) t.stats.bleedDurationFlat = (float)sObj.at("bleedDurationFlat").asDouble();
            if (sObj.count("bleedDurationPercent")) t.stats.bleedDurationPercent = (float)sObj.at("bleedDurationPercent").asDouble();
            if (sObj.count("slowMovePercent")) t.stats.slowMovePercent = (float)sObj.at("slowMovePercent").asDouble();
            if (sObj.count("slowMoveDuration")) t.stats.slowMoveDuration = (float)sObj.at("slowMoveDuration").asDouble();
            if (sObj.count("slowAttackPercent")) t.stats.slowAttackPercent = (float)sObj.at("slowAttackPercent").asDouble();
            if (sObj.count("slowAttackDuration")) t.stats.slowAttackDuration = (float)sObj.at("slowAttackDuration").asDouble();
            if (sObj.count("aoeRadius")) t.stats.aoeRadius = (float)sObj.at("aoeRadius").asDouble();
            if (sObj.count("aoeDamagePercent")) t.stats.aoeDamagePercent = (float)sObj.at("aoeDamagePercent").asDouble();
            if (sObj.count("moveSpeedBonus")) t.stats.moveSpeedBonus = (float)sObj.at("moveSpeedBonus").asDouble();
            
            if (sObj.count("damageReduction")) t.stats.damageReduction = (float)sObj.at("damageReduction").asDouble();
            if (sObj.count("tenacity")) t.stats.tenacity = (float)sObj.at("tenacity").asDouble();
            if (sObj.count("critAvoidance")) t.stats.critAvoidance = (float)sObj.at("critAvoidance").asDouble();
            if (sObj.count("antiArmorPenPercent")) t.stats.antiArmorPenPercent = (float)sObj.at("antiArmorPenPercent").asDouble();
            if (sObj.count("antiArmorPenFlat")) t.stats.antiArmorPenFlat = sObj.at("antiArmorPenFlat").asInt();
            if (sObj.count("manaStealPercent")) t.stats.manaStealPercent = (float)sObj.at("manaStealPercent").asDouble();
            if (sObj.count("xpBonusPercent")) t.stats.xpBonusPercent = (float)sObj.at("xpBonusPercent").asDouble();
            if (sObj.count("cooldownReductionPercent")) t.stats.cooldownReductionPercent = (float)sObj.at("cooldownReductionPercent").asDouble();
            if (sObj.count("armorPenetration")) t.stats.armorPenetration = (float)sObj.at("armorPenetration").asDouble();
            if (sObj.count("executeDamagePercent")) t.stats.executeDamagePercent = sObj.at("executeDamagePercent").asInt();
            if (sObj.count("executeHealthThresholdPercent")) t.stats.executeHealthThresholdPercent = sObj.at("executeHealthThresholdPercent").asInt();
            if (sObj.count("trueDamagePercent")) t.stats.trueDamagePercent = sObj.at("trueDamagePercent").asInt();
            if (sObj.count("accuracy")) t.stats.accuracy = sObj.at("accuracy").asInt();
            if (sObj.count("evasion")) t.stats.evasion = sObj.at("evasion").asInt();
            if (sObj.count("thornsPercent")) t.stats.thornsPercent = (float)sObj.at("thornsPercent").asDouble();
            if (sObj.count("hpRegenPercent")) t.stats.hpRegenPercent = (float)sObj.at("hpRegenPercent").asDouble();
            if (sObj.count("mpRegenPercent")) t.stats.mpRegenPercent = (float)sObj.at("mpRegenPercent").asDouble();
            if (sObj.count("blockChance")) t.stats.blockChance = (float)sObj.at("blockChance").asDouble();
            if (sObj.count("blockValuePercent")) t.stats.blockValuePercent = (float)sObj.at("blockValuePercent").asDouble();

            if (sObj.count("attackPercent")) t.stats.attackPercent = (float)sObj.at("attackPercent").asDouble();
            else if (sObj.count("physicalAttackPercent")) t.stats.attackPercent = (float)sObj.at("physicalAttackPercent").asDouble();
            else if (sObj.count("atkPercent")) t.stats.attackPercent = (float)sObj.at("atkPercent").asDouble();

            if (sObj.count("defensePercent")) t.stats.defensePercent = (float)sObj.at("defensePercent").asDouble();
            else if (sObj.count("defPercent")) t.stats.defensePercent = (float)sObj.at("defPercent").asDouble();

            if (sObj.count("hpPercent")) t.stats.hpPercent = (float)sObj.at("hpPercent").asDouble();
            else if (sObj.count("maxHpPercent")) t.stats.hpPercent = (float)sObj.at("maxHpPercent").asDouble();

            if (sObj.count("mpPercent")) t.stats.mpPercent = (float)sObj.at("mpPercent").asDouble();
            else if (sObj.count("maxMpPercent")) t.stats.mpPercent = (float)sObj.at("maxMpPercent").asDouble();

            if (sObj.count("strengthPercent")) t.stats.strengthPercent = (float)sObj.at("strengthPercent").asDouble();
            else if (sObj.count("strPercent")) t.stats.strengthPercent = (float)sObj.at("strPercent").asDouble();

            if (sObj.count("agilityPercent")) t.stats.agilityPercent = (float)sObj.at("agilityPercent").asDouble();
            else if (sObj.count("agiPercent")) t.stats.agilityPercent = (float)sObj.at("agiPercent").asDouble();
            else if (sObj.count("dexPercent")) t.stats.agilityPercent = (float)sObj.at("dexPercent").asDouble();

            if (sObj.count("intelligencePercent")) t.stats.intelligencePercent = (float)sObj.at("intelligencePercent").asDouble();
            else if (sObj.count("intPercent")) t.stats.intelligencePercent = (float)sObj.at("intPercent").asDouble();

            if (sObj.count("vitalityPercent")) t.stats.vitalityPercent = (float)sObj.at("vitalityPercent").asDouble();
            else if (sObj.count("vitPercent")) t.stats.vitalityPercent = (float)sObj.at("vitPercent").asDouble();

            if (sObj.count("physicalDamageBonus")) t.stats.physicalDamageBonus = (float)sObj.at("physicalDamageBonus").asDouble();
            else if (sObj.count("physicalDamagePercent")) t.stats.physicalDamageBonus = (float)sObj.at("physicalDamagePercent").asDouble();
            
            if (sObj.count("value")) t.stats.value = sObj.at("value").asInt();
        }
        mFixedUniques[t.id] = t;
    };

    auto loadFile = [&](const std::string& filepath, ItemType typeDefault) {
        json::Value rootVal = json::parseFile(filepath);
        if (rootVal.type != json::Type::Object) {
            std::cerr << "[ItemManager] WARNING: No se pudo cargar o parsear " << filepath << "\n";
            return;
        }
        auto obj = rootVal.asObject();
        
        // Procedural archetypes
        if (obj.count("procedural_archetypes") && obj.at("procedural_archetypes").type == json::Type::Array) {
            for (const auto& val : obj.at("procedural_archetypes").asArray()) {
                if (val.type == json::Type::Object) {
                    auto t = parseProcedural(val.asObject());
                    if (typeDefault == ItemType::Weapon) {
                        mWeaponTemplates.push_back(t);
                    } else if (typeDefault == ItemType::Armor) {
                        if (t.slot == EquipmentSlot::Ring1 || t.slot == EquipmentSlot::Ring2) {
                            mRingTemplates.push_back(t);
                        } else {
                            mArmorTemplates.push_back(t);
                        }
                    } else if (typeDefault == ItemType::Stone) {
                        mStoneTemplates.push_back(t);
                    }
                }
            }
        }
        
        // Fixed Uniques
        if (obj.count("fixed_uniques") && obj.at("fixed_uniques").type == json::Type::Array) {
            for (const auto& val : obj.at("fixed_uniques").asArray()) {
                if (val.type == json::Type::Object) {
                    auto o = val.asObject();
                    ItemType resolvedType = typeDefault;
                    if (typeDefault == ItemType::Armor) {
                        std::string slotStr = o.count("slot") ? o.at("slot").asString() : "";
                        if (slotStr == "Ring1" || slotStr == "Ring2" || slotStr == "Ring") {
                            resolvedType = ItemType::Ring;
                        }
                    }
                    parseUnique(o.count("id") ? o.at("id").asString() : "", o, resolvedType);
                }
            }
        }
    };

    // Cargar los archivos JSON modulares
    loadFile("assets/data/items_weapons.json", ItemType::Weapon);
    loadFile("assets/data/items_armor.json", ItemType::Armor);
    loadFile("assets/data/items_materials.json", ItemType::Misc);
    loadFile("assets/data/items_consumables.json", ItemType::Potion);
    loadFile("assets/data/items_stones.json", ItemType::Stone); // [NEW]

    mLoaded = true;
    std::cout << "[ItemManager] Carga modular exitosa (" 
              << mWeaponTemplates.size() << " arquetipos de armas, "
              << mArmorTemplates.size() << " arquetipos de armaduras, "
              << mRingTemplates.size() << " arquetipos de anillos, "
              << mStoneTemplates.size() << " arquetipos de gemas, " // [NEW]
              << mFixedUniques.size() << " items unicos/fijos)\n";
}

std::shared_ptr<Item> ItemManager::createRandomWeapon(int level) {
    loadTemplates();

    if (mWeaponTemplates.empty()) {
        // Fallback total
        auto item = std::make_shared<Item>("fallback_weapon", "Espada Fallback", ItemType::Weapon, "assets/items/weapons/weapons-base.png");
        item->stats.physicalDamage = 10;
        return item;
    }

    int idx = Random::Int(0, static_cast<int>(mWeaponTemplates.size() - 1));
    const auto& t = mWeaponTemplates[idx];

    auto item = std::make_shared<Item>();
    item->type = ItemType::Weapon;
    item->id = "proc_weapon_" + std::to_string(Random::Int(1000, 9999));
    item->gripType = t.grip;
    item->slotType = EquipmentSlot::MainHand;
    item->name = t.name;

    item->stats.physicalDamage = t.physicalDamage;
    item->stats.attackSpeed = t.attackSpeed;
    item->stats.range = t.range;
    item->stats.value = 10;

    if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->texturePath = a.path;
        item->textureRect = sf::IntRect({t.gridPos.x * a.cellSize, t.gridPos.y * a.cellSize}, {a.cellSize, a.cellSize});
        item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
    } else {
        item->texturePath = "assets/items/weapons/weapons-base.png";
        item->textureRect = sf::IntRect({t.gridPos.x * 16, t.gridPos.y * 16}, {16, 16});
        item->scale = (t.scale >= 0.f) ? t.scale : 1.5f;
    }

    item->offset = t.offset;

    ItemQuality quality = WeaponsFactory::rollQuality(level);
    WeaponsFactory::scaleWeapon(*item, level, quality);

    return item;
}

std::shared_ptr<Item> ItemManager::createRandomArmor(EquipmentSlot slot, int level) {
    loadTemplates();

    std::vector<ProceduralTemplate> matching;
    for (const auto& t : mArmorTemplates) {
        if (t.slot == slot) {
            matching.push_back(t);
        }
    }

    if (matching.empty() && !mArmorTemplates.empty()) {
        matching = mArmorTemplates;
    }

    if (matching.empty()) {
        auto item = std::make_shared<Item>("fallback_armor", "Casco Fallback", ItemType::Armor, "assets/items/weapons/weapons-base.png");
        item->slotType = slot;
        item->stats.defense = 5;
        return item;
    }

    int idx = Random::Int(0, static_cast<int>(matching.size() - 1));
    const auto& t = matching[idx];

    auto item = std::make_shared<Item>();
    item->type = ItemType::Armor;
    item->id = "proc_armor_" + std::to_string(Random::Int(1000, 9999));
    item->slotType = slot;
    item->name = t.name;

    item->stats.defense = t.defense;
    item->stats.vitality = t.vitality;
    item->stats.strength = t.strength;
    item->stats.agility = t.agility;
    item->stats.intelligence = t.intelligence;
    item->stats.hpBonus = t.hpBonus;
    item->stats.mpBonus = t.mpBonus;
    item->stats.value = 15;

    if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->texturePath = a.path;
        item->textureRect = sf::IntRect({t.gridPos.x * a.cellSize, t.gridPos.y * a.cellSize}, {a.cellSize, a.cellSize});
        item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
    } else {
        item->texturePath = "assets/items/weapons/weapons-base.png";
        item->textureRect = sf::IntRect({t.gridPos.x * 16, t.gridPos.y * 16}, {16, 16});
        item->scale = (t.scale >= 0.f) ? t.scale : 1.5f;
    }

    item->offset = t.offset;

    ItemQuality quality = ItemFactory::rollQuality(level);
    ItemFactory::scaleArmor(*item, level, quality);

    return item;
}

std::shared_ptr<Item> ItemManager::createRandomRing(int level) {
    loadTemplates();

    if (mRingTemplates.empty()) {
        return nullptr;
    }

    int idx = Random::Int(0, static_cast<int>(mRingTemplates.size() - 1));
    const auto& t = mRingTemplates[idx];

    auto item = std::make_shared<Item>();
    item->type = ItemType::Ring;
    item->id = "proc_ring_" + std::to_string(Random::Int(1000, 9999));
    item->slotType = EquipmentSlot::Ring1;
    item->name = t.name;

    item->stats.vitality = t.vitality;
    item->stats.strength = t.strength;
    item->stats.agility = t.agility;
    item->stats.intelligence = t.intelligence;
    item->stats.hpBonus = t.hpBonus;
    item->stats.mpBonus = t.mpBonus;
    item->stats.value = 20;

    if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->texturePath = a.path;
        item->textureRect = sf::IntRect({t.gridPos.x * a.cellSize, t.gridPos.y * a.cellSize}, {a.cellSize, a.cellSize});
        item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
    } else {
        item->texturePath = "assets/items/weapons/weapons-base.png";
        item->textureRect = sf::IntRect({t.gridPos.x * 16, t.gridPos.y * 16}, {16, 16});
        item->scale = (t.scale >= 0.f) ? t.scale : 1.5f;
    }

    item->offset = t.offset;

    ItemQuality quality = ItemFactory::rollQuality(level);
    ItemFactory::scaleRing(*item, level, quality);

    return item;
}

std::shared_ptr<Item> ItemManager::createRandomStone(int level) {
    loadTemplates();

    if (mStoneTemplates.empty()) {
        return nullptr;
    }

    int idx = Random::Int(0, static_cast<int>(mStoneTemplates.size() - 1));
    const auto& t = mStoneTemplates[idx];

    auto item = std::make_shared<Item>();
    item->type = ItemType::Stone;
    item->id = "proc_stone_" + std::to_string(Random::Int(1000, 9999));
    item->slotType = EquipmentSlot::None;
    item->name = t.name;

    item->stats.strength = t.strength;
    item->stats.agility = t.agility;
    item->stats.intelligence = t.intelligence;
    item->stats.vitality = t.vitality;
    item->stats.hpBonus = t.hpBonus;
    item->stats.mpBonus = t.mpBonus;
    item->stats.physicalDamage = t.physicalDamage;
    item->stats.defense = t.defense;
    item->stats.critChance = t.critChance;
    item->stats.critDamage = t.critDamage;
    item->stats.lifestealPercent = t.lifestealPercent;
    item->stats.armorPenetration = t.armorPenetration;
    item->stats.cooldownReductionPercent = t.cooldownReductionPercent;

    item->stats.attackPercent = t.attackPercent;
    item->stats.defensePercent = t.defensePercent;
    item->stats.hpPercent = t.hpPercent;
    item->stats.mpPercent = t.mpPercent;
    item->stats.strengthPercent = t.strengthPercent;
    item->stats.agilityPercent = t.agilityPercent;
    item->stats.intelligencePercent = t.intelligencePercent;
    item->stats.vitalityPercent = t.vitalityPercent;
    item->stats.physicalDamageBonus = t.physicalDamageBonus;

    item->atlasIndex = t.gridPos;

    if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->texturePath = a.path;
        item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
        item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
    } else {
        item->texturePath = "assets/ui/stones_system/atlas_stones.png";
        item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
        item->scale = 1.0f;
    }

    item->offset = t.offset;

    ItemQuality quality = ItemFactory::rollQuality(level);
    StoneSystem::scaleStone(*item, level, quality);

    return item;
}

std::shared_ptr<Item> ItemManager::createItem(const std::string& id) {
    loadTemplates();

    auto it = mFixedUniques.find(id);
    if (it == mFixedUniques.end()) {
        // Fallback: check if the ID corresponds to a procedural stone archetype template
        for (const auto& t : mStoneTemplates) {
            if (t.id == id) {
                auto item = std::make_shared<Item>();
                item->id = t.id;
                item->name = t.name;
                item->type = ItemType::Stone;
                item->quality = ItemQuality::Common; // Default starting quality
                item->gripType = GripType::OneHanded;
                item->slotType = EquipmentSlot::None;
                item->atlasIndex = t.gridPos;

                if (mAtlases.count(t.atlas)) {
                    const auto& a = mAtlases.at(t.atlas);
                    item->texturePath = a.path;
                    item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
                    item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
                } else {
                    item->texturePath = "assets/ui/stones_system/atlas_stones.png";
                    item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
                    item->scale = 2.0f;
                }

                item->offset = t.offset;
                item->stats.strength = t.strength;
                item->stats.agility = t.agility;
                item->stats.intelligence = t.intelligence;
                item->stats.vitality = t.vitality;
                item->stats.hpBonus = t.hpBonus;
                item->stats.mpBonus = t.mpBonus;
                item->stats.physicalDamage = t.physicalDamage;
                item->stats.defense = t.defense;
                item->stats.critChance = t.critChance;
                item->stats.critDamage = t.critDamage;
                item->stats.lifestealPercent = t.lifestealPercent;
                item->stats.armorPenetration = t.armorPenetration;
                item->stats.cooldownReductionPercent = t.cooldownReductionPercent;
                item->stats.value = 5;

                return item;
            }
        }

        std::cerr << "[ItemManager] WARNING: No se encontro el item unico con ID: " << id << "\n";
        return nullptr;
    }

    const auto& t = it->second;

    auto item = std::make_shared<Item>();
    item->id = t.id;
    item->name = t.name;
    item->type = t.type;
    item->quality = stringToQuality(t.quality);
    item->gripType = t.grip;
    item->slotType = t.slot;

    if (!t.texturePath.empty()) {
        item->texturePath = t.texturePath;
        item->textureRect = sf::IntRect({0, 0}, {16, 16});
        item->scale = (t.scale >= 0.f) ? t.scale : 1.5f;
    } else if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->texturePath = a.path;
        if (item->type == ItemType::Stone) {
            item->atlasIndex = t.gridPos;
            item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
        } else {
            item->textureRect = sf::IntRect({t.gridPos.x * a.cellSize, t.gridPos.y * a.cellSize}, {a.cellSize, a.cellSize});
        }
        item->scale = (t.scale >= 0.f) ? t.scale : a.defaultScale;
    } else {
        item->texturePath = "assets/items/weapons/weapons-base.png";
        if (item->type == ItemType::Stone) {
            item->atlasIndex = t.gridPos;
            item->textureRect = sf::IntRect({t.gridPos.x * 6 + 1, t.gridPos.y * 6 + 1}, {5, 5});
            item->scale = 3.0f;
        } else {
            item->textureRect = sf::IntRect({t.gridPos.x * 16, t.gridPos.y * 16}, {16, 16});
            item->scale = (t.scale >= 0.f) ? t.scale : 1.5f;
        }
    }

    item->offset = t.offset;
    item->guardOffset = t.guardOffset;
    if (mAtlases.count(t.atlas)) {
        const auto& a = mAtlases.at(t.atlas);
        item->offset += a.offset;
        item->guardOffset += a.guardOffset;
    }
    item->stats = t.stats;

    item->basePhysicalDamage = item->stats.physicalDamage;
    item->baseDefense = item->stats.defense;

    // Roll Sockets
    if (item->type == ItemType::Weapon) {
        if (item->gripType == GripType::OneHanded) {
            item->maxSockets = Random::Int(0, 3);
        } else {
            item->maxSockets = Random::Int(0, 6);
        }
        item->socketedStones.resize(item->maxSockets, nullptr);
    } else if (item->type == ItemType::Armor) {
        item->maxSockets = Random::Int(0, 3);
        item->socketedStones.resize(item->maxSockets, nullptr);
    }

    return item;
}

std::shared_ptr<Item> ItemManager::create32x32Weapon(int index, int level) {
    auto item = std::make_shared<Item>();
    item->type = ItemType::Weapon;
    item->id = "weapon_32x32_" + std::to_string(index);
    item->texturePath = "assets/items/weapons/32x32x10.png";
    item->scale = 1.0f;
    item->offset = {28.f, 5.f};

    for (const auto& pair : mAtlases) {
        if (pair.second.path == item->texturePath) {
            item->scale = pair.second.defaultScale;
            break;
        }
    }

    item->name = WeaponsFactory::getCustom32x32Name(index);
    if (index == 1) {
        item->textureRect = sf::IntRect({0, 0}, {32, 32});
        item->stats.physicalDamage = 24;
        item->stats.attackSpeed = 1.0f;
        item->stats.range = 80.f;
        item->stats.value = 50;
        item->gripType = GripType::TwoHanded;
    } else {
        item->textureRect = sf::IntRect({0, 32}, {32, 32});
        item->stats.physicalDamage = 36;
        item->stats.attackSpeed = 0.6f;
        item->stats.range = 100.f;
        item->stats.value = 80;
        item->gripType = GripType::TwoHanded;
    }

    ItemQuality quality = WeaponsFactory::rollQuality(level);
    WeaponsFactory::scaleWeapon(*item, level, quality);

    return item;
}

std::vector<std::string> ItemManager::getFixedUniqueIds() {
    loadTemplates();
    std::vector<std::string> ids;
    for (const auto& pair : mFixedUniques) {
        ids.push_back(pair.first);
    }
    return ids;
}

const std::map<std::string, ItemManager::AtlasInfo>& ItemManager::getAtlases() {
    loadTemplates();
    return mAtlases;
}