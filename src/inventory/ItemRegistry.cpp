#include "ItemRegistry.h"
#include <iostream>

ItemRegistry& ItemRegistry::Get() {
    static ItemRegistry instance;
    return instance;
}

ItemRegistry::ItemRegistry() {
    // El índice 0 siempre es el objeto INVALID / EMPTY
    m_dummyInvalid.id = INVALID_ITEM_ID;
    m_dummyInvalid.stringId = "none";
    m_dummyInvalid.name = "Vacio";
    m_dummyInvalid.description = "";
    m_dummyInvalid.maxStack = 0;
    m_dummyInvalid.weight = 0.0f;
    m_dummyInvalid.value = 0;

    m_definitions.push_back(m_dummyInvalid);
    m_stringToId["none"] = INVALID_ITEM_ID;

    InitDefaultItems();
}

ItemId ItemRegistry::RegisterItem(ItemDefinition def) {
    if (def.stringId.empty()) {
        std::cerr << "[ItemRegistry] Error: Intento de registrar un item sin stringId." << std::endl;
        return INVALID_ITEM_ID;
    }

    auto it = m_stringToId.find(def.stringId);
    if (it != m_stringToId.end()) {
        // Ya existe: actualizar definición manteniendo el mismo ID
        m_definitions[it->second.value] = def;
        m_definitions[it->second.value].id = it->second;
        return it->second;
    }

    // Nuevo ID numérico secuencial (índice en el vector contiguo)
    ItemNumericId newNumericId = static_cast<ItemNumericId>(m_definitions.size());
    ItemId newId(newNumericId);
    def.id = newId;

    m_definitions.push_back(def);
    m_stringToId[def.stringId] = newId;

    return newId;
}

const ItemDefinition& ItemRegistry::Get(ItemId id) const {
    if (id.value > 0 && id.value < m_definitions.size()) {
        return m_definitions[id.value];
    }
    return m_dummyInvalid;
}

ItemId ItemRegistry::FindId(const std::string& stringId) const {
    auto it = m_stringToId.find(stringId);
    if (it != m_stringToId.end()) {
        return it->second;
    }
    return INVALID_ITEM_ID;
}

const ItemDefinition* ItemRegistry::FindByString(const std::string& stringId) const {
    ItemId id = FindId(stringId);
    if (id.IsValid()) {
        return &m_definitions[id.value];
    }
    return nullptr;
}

bool ItemRegistry::HasItem(ItemId id) const noexcept {
    return id.value > 0 && id.value < m_definitions.size();
}

void ItemRegistry::InitDefaultItems() {
    // -------------------------------------------------------------------------
    // 1. MATERIALES Y RECURSOS
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "wood_log";
        item.name = "Tronco de Madera";
        item.description = "Madera maciza talada en el bosque. Util para construir refugios.";
        item.category = ItemCategory::MATERIAL;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 99;
        item.weight = 1.2f;
        item.value = 2;
        item.iconId = "wood";
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "stone_rock";
        item.name = "Piedra Rugosa";
        item.description = "Bloque de piedra natural para construcciones pesadas y herramientas.";
        item.category = ItemCategory::MATERIAL;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 99;
        item.weight = 2.0f;
        item.value = 2;
        item.iconId = "stone";
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "beast_pelt";
        item.name = "Piel de Bestia";
        item.description = "Piel gruesa curtida de animales del bosque. Otorga +40 EXP de artesanía.";
        item.category = ItemCategory::MATERIAL;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 50;
        item.weight = 0.8f;
        item.value = 15;
        item.iconId = "pelt";
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "hunting_arrow";
        item.name = "Flecha de Caza";
        item.description = "Flechas afiladas con punta de hierro y plumas de bosque.";
        item.category = ItemCategory::MATERIAL;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 99;
        item.weight = 0.05f;
        item.value = 1;
        item.iconId = "arrow";
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // 2. CONSUMIBLES Y POCIONES
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "potion_health";
        item.name = "Medicina / Venda";
        item.description = "Ungüento medicinal y vendas estériles. Restaura +45 de Vida.";
        item.category = ItemCategory::CONSUMABLE;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 10;
        item.weight = 0.25f;
        item.value = 12;
        item.iconId = "pot_hp";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::RESTORE_HP, 45.0f, 0.0f });
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "potion_mana";
        item.name = "Eter Corrupto";
        item.description = "Destilado de energía arcana luminosa. Restaura +35 de Mana.";
        item.category = ItemCategory::CONSUMABLE;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 10;
        item.weight = 0.25f;
        item.value = 15;
        item.iconId = "pot_mp";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::RESTORE_MP, 35.0f, 0.0f });
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "raw_meat";
        item.name = "Carne Cruda";
        item.description = "Carne fresca obtenida de la caza. Restaura +25 de Vida.";
        item.category = ItemCategory::CONSUMABLE;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 20;
        item.weight = 0.5f;
        item.value = 5;
        item.iconId = "meat";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::RESTORE_HP, 25.0f, 0.0f });
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "blood_vial";
        item.name = "Vial de Sangre";
        item.description = "Frasco con sangre fresca coagulada. Cebo o reactivo de brujeria.";
        item.category = ItemCategory::CONSUMABLE;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 20;
        item.weight = 0.3f;
        item.value = 20;
        item.iconId = "blood";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::RESTORE_HP, 15.0f, 0.0f });
        item.effects.push_back({ ItemEffectType::BUFF_ATTACK, 4.0f, 30.0f }); // Buff de furia 30s
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "arcane_scroll";
        item.name = "Pergamino Arcano";
        item.description = "Inscripción en papiro antiguo. Desbloquea sabiduría y otorga +100 EXP.";
        item.category = ItemCategory::SCROLL;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 5;
        item.weight = 0.1f;
        item.value = 60;
        item.iconId = "scroll";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::GRANT_EXP, 100.0f, 0.0f });
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // 3. EQUIPAMIENTO Y ARMAS
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "cursed_sword";
        item.name = "Espada Maldita";
        item.description = "Hoja forjada con acero umbrio. Arma de una mano: +8 ATQ / +5% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 3.2f;
        item.value = 120;
        item.iconId = "sword";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 8;
        item.equipStats.critChance = 5;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "frost_claymore";
        item.name = "Mandoble de Escarcha";
        item.description = "Gran mandoble a dos manos imbuido con escarcha mortal: +18 ATQ / +8 DEF.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 6.5f;
        item.value = 280;
        item.iconId = "claymore";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true; // Ocupa MAIN_HAND y bloquea OFF_HAND
        item.equipStats.attackPower = 18;
        item.equipStats.defense = 8;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "hunting_bow";
        item.name = "Arco Recurvo del Explorador";
        item.description = "Arco largo tensado a dos manos: +12 ATQ / +15% CRIT a distancia.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 2.4f;
        item.value = 180;
        item.iconId = "bow";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 12;
        item.equipStats.critChance = 15;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_shield";
        item.name = "Escudo de Hierro";
        item.description = "Escudo pesado de guardia reforzado: +6 DEF / +10 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 4.0f;
        item.value = 85;
        item.iconId = "shield";
        item.equipSlot = EquipSlot::OFF_HAND;
        item.isTwoHanded = false;
        item.equipStats.defense = 6;
        item.equipStats.maxHpBonus = 10;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_ring";
        item.name = "Anillo de Sombras";
        item.description = "Sortija de plata oscura con obsidiana: +12% CRIT / +8% EVA.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 0.05f;
        item.value = 150;
        item.iconId = "ring";
        item.equipSlot = EquipSlot::RING_1;
        item.equipStats.critChance = 12;
        item.equipStats.evasion = 8;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "vampiric_ring";
        item.name = "Anillo de Sifon";
        item.description = "Joya carmesi bendecida por vampiros: +8 ATQ / +10% CRIT / +15 MP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 0.05f;
        item.value = 220;
        item.iconId = "ring_vamp";
        item.equipSlot = EquipSlot::RING_1;
        item.equipStats.attackPower = 8;
        item.equipStats.critChance = 10;
        item.equipStats.maxMpBonus = 15;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "ancient_amulet";
        item.name = "Amuleto Antiguo";
        item.description = "Reliquia con grabados protectores de los antiguos: +15 HP / +20 MP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 0.1f;
        item.value = 160;
        item.iconId = "amulet";
        item.equipSlot = EquipSlot::AMULET;
        item.equipStats.maxHpBonus = 15;
        item.equipStats.maxMpBonus = 20;
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // 4. SET DE ARMADURA Y EQUIPO LEGENDARIO DE DRAGON (BOSS LOOT)
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "dragon_helm";
        item.name = "Yelmo Draconico Ancestral";
        item.description = "Casco forjado con craneo y cuernos de dragon: +18 DEF / +60 HP / +8 ATQ / +5% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 3.5f;
        item.value = 850;
        item.iconId = "helm_dragon";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 18;
        item.equipStats.maxHpBonus = 60;
        item.equipStats.attackPower = 8;
        item.equipStats.critChance = 5;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_chest";
        item.name = "Coraza de Placas Draconicas";
        item.description = "Pesada armadura forjada con escamas impenetrables: +30 DEF / +100 HP / +10 ATQ / +12% EVA.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 7.8f;
        item.value = 1250;
        item.iconId = "chest_dragon";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 30;
        item.equipStats.maxHpBonus = 100;
        item.equipStats.attackPower = 10;
        item.equipStats.evasion = 12;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_scale_ring";
        item.name = "Sello Igneo del Dragon";
        item.description = "Sortija tallada en gema volcanica draconica: +14 ATQ / +15% CRIT / +35 MP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 0.05f;
        item.value = 780;
        item.iconId = "ring_dragon";
        item.equipSlot = EquipSlot::RING_1;
        item.equipStats.attackPower = 14;
        item.equipStats.critChance = 15;
        item.equipStats.maxMpBonus = 35;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_bone_bow";
        item.name = "Gran Arco de Hueso de Dragon";
        item.description = "Arco colosal tensado con tendones de Wyvern: +26 ATQ / +20% CRIT a distancia.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 3.2f;
        item.value = 950;
        item.iconId = "bow";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 26;
        item.equipStats.critChance = 20;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_heart";
        item.name = "Corazon de Fuego Ancestral";
        item.description = "Organo llameante de dragon que pulsa con calor eterno. Restaura +120 HP y +80 MP al instante.";
        item.category = ItemCategory::CONSUMABLE;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 5;
        item.weight = 1.0f;
        item.value = 500;
        item.iconId = "heart_dragon";
        item.isUsable = true;
        item.effects.push_back({ ItemEffectType::RESTORE_HP, 120.0f, 0.0f });
        item.effects.push_back({ ItemEffectType::RESTORE_MP, 80.0f, 0.0f });
        item.effects.push_back({ ItemEffectType::BUFF_ATTACK, 12.0f, 60.0f }); // Buff de furia +12 ATQ por 60s
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_scale";
        item.name = "Escama de Dragon Carmesi";
        item.description = "Escama legendaria tan dura como el diamante. Material mitico de artesania.";
        item.category = ItemCategory::MATERIAL;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 50;
        item.weight = 0.5f;
        item.value = 150;
        item.iconId = "scale_dragon";
        RegisterItem(item);
    }

    std::cout << "[ItemRegistry] Catalogo inicializado con " << GetTotalRegistered() << " definiciones de objetos." << std::endl;
}
