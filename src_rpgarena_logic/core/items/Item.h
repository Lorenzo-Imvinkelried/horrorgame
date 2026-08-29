// Item.h
#pragma once
#include <string>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cctype>
#include <vector>
#include <memory>

enum class EquipmentSlot {
  Head = 0,
  Cape = 1,
  Chest = 2,
  Hands = 3,
  MainHand = 4,
  Legs = 5,
  OffHand = 6,
  Ring1 = 7,
  Feet = 8,
  Ring2 = 9,
  SubWeapon1 = 10,
  SubWeapon2 = 11,
  Count = 12,
  None = -1
};

enum class ItemType {
    Weapon,
    Armor,
    Ring,
    Potion,
    Misc,
    Stone
};

// [NEW] Enum Class for Quality
enum class ItemQuality {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary,
    Mythic,
    Godly
};

enum class GripType {
    OneHanded,
    TwoHanded
};

// --- HELPER FUNCTIONS FORWARD DECLARATION ---
// (Not strictly needed if we define them inline below)

// --- STRUCTS ---

struct ItemStats {
    int physicalDamage = 0;
    float attackSpeed = 0.f; 
    float critChance = 0.f;  
    float critDamage = 0.f;  // Nuevo
    float range = 0.f;       
    
    int defense = 0;         // Nuevo
    
    int strength = 0;        // Nuevo
    int agility = 0;       // Nuevo
    int intelligence = 0;    // Nuevo
    int vitality = 0;        // Nuevo
    
    int hpBonus = 0;         // Nuevo
    int mpBonus = 0;         // Nuevo
    
    float armorPenetration = 0.f; 
    
    // [NEW] Execute Stats
    int executeDamagePercent = 0;       // % Daño extra
    int executeHealthThresholdPercent = 0; // % Vida del enemigo para activar

    // [NEW] True Damage
    int trueDamagePercent = 0;

    // [NEW] Accuracy & Evasion
    int accuracy = 0;
    int evasion = 0;

    // [NEW STATS]
    float tenacity = 0.f;              // % Stun duration reduction
    float damageReduction = 0.f;       // % Damage Mitigation
    float critAvoidance = 0.f;         // % Crit Chance reduction for attacker
    float antiArmorPenPercent = 0.f;   // % ArPen reduction
    int   antiArmorPenFlat = 0;        // Flat ArPen reduction
    float manaStealPercent = 0.f;      // % Mana Steal
    float xpBonusPercent = 0.f;        // % XP Gain

    // [NEW SPECIAL STATS]
    float lifestealPercent = 0.f;      // % Robo de Vida
    float cooldownReductionPercent = 0.f; // % Reducción de Enfriamiento
    float stunChance = 0.f;            // % Probabilidad de Stun
    float stunDuration = 0.f;          // Segundos de Stun
    int   bleedFlat = 0;               // Daño plano de sangrado por tick
    float bleedPercent = 0.f;          // % Daño de ataque como sangrado por tick
    float bleedDurationFlat = 0.f;     // Duración de sangrado plano en segundos
    float bleedDurationPercent = 0.f;  // Duración de sangrado % en segundos
    float slowMovePercent = 0.f;       // % Ralentización de movimiento
    float slowMoveDuration = 0.f;      // Duración de ralentización de movimiento
    float slowAttackPercent = 0.f;     // % Ralentización de ataque
    float slowAttackDuration = 0.f;    // Duración de ralentización de ataque
    float aoeRadius = 0.f;             // Radio de AoE en píxeles
    float aoeDamagePercent = 0.f;      // % Daño transmitido por AoE
    float moveSpeedBonus = 0.f;        // [NEW] Bono de velocidad de movimiento
    float thornsPercent = 0.f;
    float hpRegenPercent = 0.f;
    float mpRegenPercent = 0.f;
    float blockChance = 0.f;
    float blockValuePercent = 0.f;

    // [NEW PERCENTAGE STATS FOR STONES AND EQUIPMENT]
    float attackPercent = 0.f;            // % Atq Físico multiplicativo (ej: +12% sobre el ataque plano)
    float defensePercent = 0.f;           // % Defensa multiplicativa
    float hpPercent = 0.f;                // % Vida Maxima multiplicativa
    float mpPercent = 0.f;                // % Mana Maximo multiplicativo
    float strengthPercent = 0.f;          // % Fuerza multiplicativa
    float agilityPercent = 0.f;           // % Agilidad multiplicativa
    float intelligencePercent = 0.f;      // % Inteligencia multiplicativa
    float vitalityPercent = 0.f;          // % Vitalidad multiplicativa
    float physicalDamageBonus = 0.f;      // % Bonus Daño Físico aditivo directo (ej: base 100% -> 120%)

    int value = 0;           
};

struct Item {
    std::string id;          // Ej: "sword_01"
    std::string name;        // Ej: "Espada Corta"
    ItemType type;
    std::string texturePath; // La ruta al archivo png
    sf::IntRect textureRect; // [NUEVO] Para sprite sheets (0,0,0,0) = full texture
    float scale = 1.0f;      // [NUEVO] Escala visual (1.0 = normal)
    sf::Vector2f offset = {0.f, 0.f}; // [NUEVO] Ajuste fino de posición (x, y)
    sf::Vector2f guardOffset = {0.f, 0.f}; // [NUEVO] Ajuste fino de posición en guardia (x, y)
    
    ItemStats stats;

    ItemQuality quality = ItemQuality::Common; // [MODIFIED] Enum instead of string
    GripType gripType = GripType::OneHanded; // [NEW] 1-hand or 2-hand

    sf::Vector2i overlayGridCoords = {-1, -1};    // [NUEVO] Coordenadas en grilla para el overlay (Col, Row). -1 si no tiene.
    EquipmentSlot slotType = EquipmentSlot::None;

    int level = 1;           // [NUEVO] Nivel del item
    int fortificationLevel = 0; // [NEW] Enhancement Level (+1, +2...)
    int basePhysicalDamage = 0; // [NEW] Base damage before fortification
    int baseDefense = 0;        // [NEW] Base defense before fortification
    int stackCount = 1;         // [NEW] Count for stackable items

    sf::Vector2i atlasIndex = {-1, -1}; // [NEW] Coordinates inside stones atlas
    int maxSockets = 0;                 // [NEW] Maximum sockets allowed
    std::vector<std::shared_ptr<Item>> socketedStones; // [NEW] List of socketed stones

    // [CULTIVO SYSTEM FIELDS]
    int cultivoLevel = 0;
    int cultivoExp = 0;
    bool cultivoLocked = false;
    std::vector<std::string> cultivoSelectedStats;
    ItemStats cultivoBonusStats;

    // Constructor
    Item() = default; 
    Item(std::string _id, std::string _name, ItemType _type, std::string _path)
        : id(_id), name(_name), type(_type), texturePath(_path), textureRect({0,0}, {0,0}), scale(1.0f), offset({0.f, 0.f}), quality(ItemQuality::Common), gripType(GripType::OneHanded), overlayGridCoords({-1,-1}), slotType(EquipmentSlot::None), level(1), fortificationLevel(0), basePhysicalDamage(0), baseDefense(0), stackCount(1), atlasIndex({-1,-1}), maxSockets(0) {}

    bool isShield() const {
        if (slotType != EquipmentSlot::OffHand && slotType != EquipmentSlot::MainHand) {
            return false;
        }
        if (stats.blockChance > 0.f || stats.blockValuePercent > 0.f) return true;
        std::string idL = id, nameL = name, texL = texturePath;
        for (auto& c : idL) c = std::tolower(c);
        for (auto& c : nameL) c = std::tolower(c);
        for (auto& c : texL) c = std::tolower(c);
        return idL.find("escudo") != std::string::npos || idL.find("shield") != std::string::npos ||
               nameL.find("escudo") != std::string::npos || nameL.find("shield") != std::string::npos ||
               texL.find("escudo") != std::string::npos || texL.find("shield") != std::string::npos;
    }
};

// --- HELPER IMPLEMENTATIONS ---

// Helper: String -> Enum
inline ItemQuality stringToQuality(const std::string& str) {
    std::string q = str;
    std::transform(q.begin(), q.end(), q.begin(), [](unsigned char c){ return std::tolower(c); });

    if (q == "uncommon" || q == "green") return ItemQuality::Uncommon;
    if (q == "rare" || q == "blue") return ItemQuality::Rare;
    if (q == "epic" || q == "orange") return ItemQuality::Epic;
    if (q == "legendary" || q == "yellow" || q == "gold") return ItemQuality::Legendary;
    if (q == "mythic" || q == "violet" || q == "purple") return ItemQuality::Mythic;
    if (q == "godly" || q == "red" || q == "artifact") return ItemQuality::Godly;

    return ItemQuality::Common;
}

// Helper: Enum -> String (internal/debug)
inline std::string qualityToString(ItemQuality q) {
    switch(q) {
        case ItemQuality::Uncommon: return "uncommon";
        case ItemQuality::Rare: return "rare";
        case ItemQuality::Epic: return "epic";
        case ItemQuality::Legendary: return "legendary";
        case ItemQuality::Mythic: return "mythic";
        case ItemQuality::Godly: return "godly";
        default: return "common";
    }
}

// Helper: Get Color
inline sf::Color getQualityColor(ItemQuality q) {
    switch(q) {
        case ItemQuality::Uncommon: return sf::Color(0, 255, 0);       // Pure Green
        case ItemQuality::Rare: return sf::Color(0, 100, 255);         // Electric Blue
        case ItemQuality::Epic: return sf::Color(255, 140, 0);         // Fire Orange
        case ItemQuality::Legendary: return sf::Color(255, 235, 0);    // Bright Yellow
        case ItemQuality::Mythic: return sf::Color(220, 0, 255);       // Neon Violet
        case ItemQuality::Godly: return sf::Color(255, 0, 0);          // Pure Red
        default: return sf::Color::Transparent; // Common
    }
}

// Helper: Get Display Name
inline std::string getQualityDisplayName(ItemQuality q) {
    switch(q) {
        case ItemQuality::Godly: return "Godly";
        case ItemQuality::Mythic: return "Mythic";
        case ItemQuality::Legendary: return "Legendary";
        case ItemQuality::Epic: return "Epic";
        case ItemQuality::Rare: return "Rare";
        case ItemQuality::Uncommon: return "Uncommon";
        default: return "Basic";
    }
}

class ItemAuraRenderer {
public:
    static inline void drawAura(sf::RenderTarget& target, const sf::Sprite& sprite, int fortificationLevel, float zoom = 1.0f, sf::RenderStates states = sf::RenderStates::Default) {
        if (fortificationLevel < 6) return;

        sf::Color auraColor;
        if (fortificationLevel >= 12) {
            auraColor = sf::Color(255, 100, 130, 200);   // +12: Rojo-Rosa claro radiante
        } else if (fortificationLevel >= 9) {
            auraColor = sf::Color(255, 180, 50, 190);   // +9: Naranja-Dorado claro brillante
        } else {
            auraColor = sf::Color(255, 255, 200, 180);  // +6: Blanco-Dorado claro brillante
        }

        sf::Sprite auraSprite = sprite;
        auraSprite.setColor(auraColor);

        // Escalado proporcional al tamaño visual real del sprite para mantener la escala del pixel art
        sf::Vector2f spScale = sprite.getScale();
        float scaleFactor = std::abs(spScale.x);
        if (scaleFactor < 0.001f) scaleFactor = 1.0f;

        float step = 1.0f * scaleFactor;
        sf::Vector2f offsets[] = {
            {-step, 0.f}, {step, 0.f}, {0.f, -step}, {0.f, step},
            {-step, -step}, {step, -step}, {-step, step}, {step, step}
        };

        sf::RenderStates auraStates = states;
        auraStates.blendMode = sf::BlendAdd; // Mezcla aditiva para luz clara brillante sin bordes oscuros

        for (const auto& off : offsets) {
            sf::Sprite temp = auraSprite;
            temp.move(off);
            target.draw(temp, auraStates);
        }
    }
};