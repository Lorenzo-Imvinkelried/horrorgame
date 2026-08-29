#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "Item.h"

class ItemManager {
public:
    ItemManager();

    // Carga las plantillas e información de atlases desde assets/data/item_templates.json
    void loadTemplates();

    // Métodos para crear ítems procedurales escalados
    std::shared_ptr<Item> createRandomWeapon(int level);
    std::shared_ptr<Item> createRandomArmor(EquipmentSlot slot, int level);
    std::shared_ptr<Item> createRandomRing(int level);
    std::shared_ptr<Item> createRandomStone(int level);

    struct AtlasInfo {
        std::string path;
        int cellSize = 16;
        float defaultScale = 1.0f;
        std::string type = "Misc";
        sf::Vector2f offset = {0.f, 0.f};
        sf::Vector2f guardOffset = {0.f, 0.f};
    };
    const std::map<std::string, AtlasInfo>& getAtlases();

    // Crea un ítem handcrafted/fijo basado en su ID único definido en el JSON
    std::shared_ptr<Item> createItem(const std::string& id);
    std::vector<std::string> getFixedUniqueIds();

    // Compatibilidad/Legacy
    std::shared_ptr<Item> create32x32Weapon(int index, int level = 1);

private:
    bool mLoaded = false;
    std::map<std::string, AtlasInfo> mAtlases;

    // Estructuras para plantillas base de generación procedural
    struct ProceduralTemplate {
        std::string id;
        std::string name;
        std::string atlas;
        sf::Vector2i gridPos;
        GripType grip = GripType::OneHanded;
        EquipmentSlot slot = EquipmentSlot::None;
        
        int physicalDamage = 0;
        float attackSpeed = 1.0f;
        float range = 50.0f;
        int defense = 0;
        int vitality = 0;
        int strength = 0;
        int agility = 0;
        int intelligence = 0;
        int hpBonus = 0;
        int mpBonus = 0;
        
        float critChance = 0.f;
        float critDamage = 0.f;
        float lifestealPercent = 0.f;
        float armorPenetration = 0.f;
        float cooldownReductionPercent = 0.f;

        float attackPercent = 0.f;
        float defensePercent = 0.f;
        float hpPercent = 0.f;
        float mpPercent = 0.f;
        float strengthPercent = 0.f;
        float agilityPercent = 0.f;
        float intelligencePercent = 0.f;
        float vitalityPercent = 0.f;
        float physicalDamageBonus = 0.f;

        sf::Vector2f offset = {0.f, 0.f};
        float scale = -1.f;
    };
    std::vector<ProceduralTemplate> mWeaponTemplates;
    std::vector<ProceduralTemplate> mArmorTemplates;
    std::vector<ProceduralTemplate> mRingTemplates;
    std::vector<ProceduralTemplate> mStoneTemplates;

    // Estructura para ítems legendarios/fijos (handcrafted uniques)
    struct FixedUniqueTemplate {
        std::string id;
        std::string name;
        std::string quality;
        std::string atlas;
        sf::Vector2i gridPos;
        GripType grip = GripType::OneHanded;
        EquipmentSlot slot = EquipmentSlot::None;
        ItemType type = ItemType::Misc;
        ItemStats stats;

        std::string texturePath;
        sf::Vector2f offset = {0.f, 0.f};
        sf::Vector2f guardOffset = {0.f, 0.f};
        float scale = -1.f;
    };
    std::map<std::string, FixedUniqueTemplate> mFixedUniques;
};