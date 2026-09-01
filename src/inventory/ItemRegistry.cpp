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
        item.stringId = "steel_shortsword";
        item.name = "Espada Corta de Acero";
        item.description = "Hoja ágil de acero templado. Arma de una mano: +10 ATQ / +8% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 2.5f;
        item.value = 75;
        item.iconId = "sword";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 10;
        item.equipStats.critChance = 8;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_greatsword";
        item.name = "Gran Mandoble de Hierro";
        item.description = "Pesado mandoble a dos manos con gran alcance: +22 ATQ / +5 DEF.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 6.0f;
        item.value = 160;
        item.iconId = "claymore";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 22;
        item.equipStats.defense = 5;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "deathknight_greatsword";
        item.name = "Mandoble del Caballero de la Muerte";
        item.description = "Colosal mandoble a dos manos forjado en acero negro y runas gélidas: +32 ATQ / +12 DEF / +10% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 8.5f;
        item.value = 450;
        item.iconId = "claymore";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 32;
        item.equipStats.defense = 12;
        item.equipStats.critChance = 10;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_axe";
        item.name = "Hacha de Guerra del Berserker";
        item.description = "Hacha bárbara colosal de doble filo curvado: +26 ATQ / +15% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 5.8f;
        item.value = 240;
        item.iconId = "axe";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 26;
        item.equipStats.critChance = 15;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "leather_armor";
        item.name = "Armadura de Cuero";
        item.description = "Pechera de cuero endurecido con tachones: +10 DEF / +25 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 3.5f;
        item.value = 80;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 10;
        item.equipStats.maxHpBonus = 25;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_armor";
        item.name = "Armadura de Placas de Hierro";
        item.description = "Robusta coraza de placas de acero y hombreras de caballero: +22 DEF / +60 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 7.0f;
        item.value = 210;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 22;
        item.equipStats.maxHpBonus = 60;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "deathknight_armor";
        item.name = "Armadura del Caballero de la Muerte";
        item.description = "Placas góticas de acero negro con hombreras gigantes con púas: +35 DEF / +110 HP / +8 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 9.5f;
        item.value = 520;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 35;
        item.equipStats.maxHpBonus = 110;
        item.equipStats.attackPower = 8;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_helm";
        item.name = "Yelmo de Acero";
        item.description = "Gran yelmo cerrado de caballero con visera: +8 DEF / +20 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 2.8f;
        item.value = 95;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 8;
        item.equipStats.maxHpBonus = 20;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "deathknight_helm";
        item.name = "Yelmo Sombrío de Calavera";
        item.description = "Yelmo oscuro de calavera con ojos espectrales celestes: +16 DEF / +50 HP / +5 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 3.8f;
        item.value = 320;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 16;
        item.equipStats.maxHpBonus = 50;
        item.equipStats.attackPower = 5;
        RegisterItem(item);
    }
    // SET DE CUERO (Partes restantes)
    {
        ItemDefinition item;
        item.stringId = "leather_cap";
        item.name = "Capucha de Cuero";
        item.description = "Capucha ligera de explorador: +4 DEF / +10 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 1.2f;
        item.value = 40;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 4;
        item.equipStats.maxHpBonus = 10;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "leather_gloves";
        item.name = "Guantes de Cuero";
        item.description = "Guantes flexibles de caza: +3 DEF / +2 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 0.8f;
        item.value = 35;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 3;
        item.equipStats.attackPower = 2;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "leather_pants";
        item.name = "Pantalones de Cuero";
        item.description = "Pantalones reforzados para travesías: +6 DEF / +15 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 2.0f;
        item.value = 50;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 6;
        item.equipStats.maxHpBonus = 15;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "leather_boots";
        item.name = "Botas de Cuero";
        item.description = "Botas ligeras de paso silencioso: +4 DEF / +5% Evasión.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::COMMON;
        item.maxStack = 1;
        item.weight = 1.5f;
        item.value = 45;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 4;
        item.equipStats.evasion = 5;
        RegisterItem(item);
    }
    // SET DE HIERRO / PLACAS (Partes restantes)
    {
        ItemDefinition item;
        item.stringId = "iron_gauntlets";
        item.name = "Guanteletes de Hierro";
        item.description = "Guanteletes pesados de acero remachado: +6 DEF / +4 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 2.4f;
        item.value = 90;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 6;
        item.equipStats.attackPower = 4;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_greaves";
        item.name = "Grebas de Placas";
        item.description = "Pantalones de malla y placas de hierro para las piernas: +14 DEF / +35 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 4.5f;
        item.value = 130;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 14;
        item.equipStats.maxHpBonus = 35;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "iron_boots";
        item.name = "Botas de Acero";
        item.description = "Escarpes y botas blindadas de caballero: +8 DEF / +20 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 3.2f;
        item.value = 110;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 8;
        item.equipStats.maxHpBonus = 20;
        RegisterItem(item);
    }
    // SET DEL CABALLERO DE LA MUERTE (Partes restantes)
    {
        ItemDefinition item;
        item.stringId = "deathknight_gauntlets";
        item.name = "Guanteletes del Caballero Oscuro";
        item.description = "Manoplas góticas de acero negro con garras afiladas: +12 DEF / +8 ATQ / +4% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 3.5f;
        item.value = 290;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 12;
        item.equipStats.attackPower = 8;
        item.equipStats.critChance = 4;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "deathknight_greaves";
        item.name = "Grebas del Caballero de la Muerte";
        item.description = "Grebas de placas negras imbuidas con almas heladas: +22 DEF / +65 HP / +4 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 6.2f;
        item.value = 380;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 22;
        item.equipStats.maxHpBonus = 65;
        item.equipStats.attackPower = 4;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "deathknight_boots";
        item.name = "Botas del Caballero de la Muerte";
        item.description = "Botas colosales de acero umbrío: +14 DEF / +45 HP / +4 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 4.8f;
        item.value = 340;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 14;
        item.equipStats.maxHpBonus = 45;
        item.equipStats.attackPower = 4;
        RegisterItem(item);
    }
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

    // -------------------------------------------------------------------------
    // HACHAS Y ESPADAS ADICIONALES (1 Mano y 2 Manos)
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "iron_hatchet";
        item.name = "Hacha de Mano de Hierro";
        item.description = "Hacha ligera y afilada para combate cuerpo a cuerpo veloz: +14 ATQ / +10% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::UNCOMMON;
        item.maxStack = 1;
        item.weight = 3.0f;
        item.value = 110;
        item.iconId = "axe";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 14;
        item.equipStats.critChance = 10;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_onehand_axe";
        item.name = "Hachuela Bárbara Berserker";
        item.description = "Hacha de asalto nórdica empapada en furia salvaje: +19 ATQ / +18% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 3.8f;
        item.value = 210;
        item.iconId = "axe";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 19;
        item.equipStats.critChance = 18;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "executioner_axe";
        item.name = "Gran Hacha del Verdugo";
        item.description = "Devastadora hacha de decapitación a dos manos con hojas de luna menguante: +34 ATQ / +22% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::EPIC;
        item.maxStack = 1;
        item.weight = 9.0f;
        item.value = 480;
        item.iconId = "axe";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 34;
        item.equipStats.critChance = 22;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "paladin_longsword";
        item.name = "Espada Sagrada del Paladín";
        item.description = "Espada bendita forjada en plata reluciente y oro: +18 ATQ / +6 DEF / +12% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 3.6f;
        item.value = 260;
        item.iconId = "sword";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 18;
        item.equipStats.defense = 6;
        item.equipStats.critChance = 12;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragonslayer_greatsword";
        item.name = "Espadón Matadragones";
        item.description = "Arma legendaria colosal de acero volcánico capaz de partir escamas y cráneos: +42 ATQ / +14 DEF / +16% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 11.0f;
        item.value = 750;
        item.iconId = "claymore";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = true;
        item.equipStats.attackPower = 42;
        item.equipStats.defense = 14;
        item.equipStats.critChance = 16;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_dagger";
        item.name = "Daga de las Sombras";
        item.description = "Daga silenciosa imbuida con veneno espectral: +12 ATQ / +25% CRIT / +8% Evasión.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 1.2f;
        item.value = 180;
        item.iconId = "sword";
        item.equipSlot = EquipSlot::MAIN_HAND;
        item.isTwoHanded = false;
        item.equipStats.attackPower = 12;
        item.equipStats.critChance = 25;
        item.equipStats.evasion = 8;
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // SET DEL BERSERKER (Armadura Bárbara Completa)
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "berserker_helm";
        item.name = "Yelmo Cornudo del Berserker";
        item.description = "Yelmo de guerra de hierro oscuro con cuernos de toro y forro de piel: +14 DEF / +6 ATQ / +40 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 3.4f;
        item.value = 240;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 14;
        item.equipStats.attackPower = 6;
        item.equipStats.maxHpBonus = 40;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_armor";
        item.name = "Pechera Bárbara del Berserker";
        item.description = "Arnés de cuero endurecido con hombreras espinadas y manto de oso: +28 DEF / +12 ATQ / +90 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 7.5f;
        item.value = 390;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 28;
        item.equipStats.attackPower = 12;
        item.equipStats.maxHpBonus = 90;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_pants";
        item.name = "Pantalones de Guerra Berserker";
        item.description = "Pantalones de cuero y pieles de bestia con rodilleras de bronce: +18 DEF / +5 ATQ / +50 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 4.8f;
        item.value = 260;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 18;
        item.equipStats.attackPower = 5;
        item.equipStats.maxHpBonus = 50;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_boots";
        item.name = "Botas de Asalto Berserker";
        item.description = "Botas de piel de oso con punteras de hierro remachadas: +11 DEF / +4 ATQ / +35 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 3.6f;
        item.value = 210;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 11;
        item.equipStats.attackPower = 4;
        item.equipStats.maxHpBonus = 35;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "berserker_gauntlets";
        item.name = "Brazales de Fuerza Berserker";
        item.description = "Brazales espinados envueltos en correas y pelaje: +10 DEF / +7 ATQ / +5% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 2.8f;
        item.value = 200;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 10;
        item.equipStats.attackPower = 7;
        item.equipStats.critChance = 5;
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // SET DEL ASESINO DE LAS SOMBRAS (Set Ligero y Letal)
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "shadow_hood";
        item.name = "Capucha del Cazador Sombrío";
        item.description = "Capucha con máscara facial y visor nocturno: +10 DEF / +10% CRIT / +10% Evasión.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 1.4f;
        item.value = 220;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 10;
        item.equipStats.critChance = 10;
        item.equipStats.evasion = 10;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_garb";
        item.name = "Ropaje del Asesino de las Sombras";
        item.description = "Jubón oscuro con frascos de veneno y hombreras silenciosas: +20 DEF / +8 ATQ / +12% EVA / +40 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 4.2f;
        item.value = 360;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 20;
        item.equipStats.attackPower = 8;
        item.equipStats.evasion = 12;
        item.equipStats.maxHpBonus = 40;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_pants";
        item.name = "Pantalones Sigilosos";
        item.description = "Pantalones de tela sombría reforzados para no hacer ruido: +12 DEF / +8% EVA / +30 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 2.4f;
        item.value = 210;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 12;
        item.equipStats.evasion = 8;
        item.equipStats.maxHpBonus = 30;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_boots";
        item.name = "Botas del Acechador";
        item.description = "Botas de suela amortiguada para pasos totalmente inaudibles: +8 DEF / +12% EVA / +25 HP.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 1.8f;
        item.value = 190;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 8;
        item.equipStats.evasion = 12;
        item.equipStats.maxHpBonus = 25;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "shadow_gloves";
        item.name = "Guantes de Tajo Sombrío";
        item.description = "Guantes ajustados con nudilleras de obsidiana: +7 DEF / +6 ATQ / +8% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::RARE;
        item.maxStack = 1;
        item.weight = 1.2f;
        item.value = 175;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 7;
        item.equipStats.attackPower = 6;
        item.equipStats.critChance = 8;
        RegisterItem(item);
    }

    // -------------------------------------------------------------------------
    // SET DE ESCAMAS DE DRAGÓN (Set Legendario Dracónico)
    // -------------------------------------------------------------------------
    {
        ItemDefinition item;
        item.stringId = "dragon_helm";
        item.name = "Yelmo del Dragón Carmesí";
        item.description = "Cráneo y cornamenta de dragón antiguo imbuido en fuego: +20 DEF / +60 HP / +8 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 4.2f;
        item.value = 600;
        item.iconId = "helm";
        item.equipSlot = EquipSlot::HEAD;
        item.equipStats.defense = 20;
        item.equipStats.maxHpBonus = 60;
        item.equipStats.attackPower = 8;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_armor";
        item.name = "Coraza de Escamas de Dragón";
        item.description = "Impenetrable coraza forjada con escamas de dragón y corazón magmático: +40 DEF / +140 HP / +15 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 10.5f;
        item.value = 950;
        item.iconId = "chest";
        item.equipSlot = EquipSlot::CHEST;
        item.equipStats.defense = 40;
        item.equipStats.maxHpBonus = 140;
        item.equipStats.attackPower = 15;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_pants";
        item.name = "Grebas de Fuego Dracónico";
        item.description = "Grebas de placas carmesí forjadas al calor del aliento del dragón: +25 DEF / +80 HP / +6 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 7.0f;
        item.value = 680;
        item.iconId = "pants";
        item.equipSlot = EquipSlot::LEGS;
        item.equipStats.defense = 25;
        item.equipStats.maxHpBonus = 80;
        item.equipStats.attackPower = 6;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_boots";
        item.name = "Botas de Escamas de Dragón";
        item.description = "Botas pesadas de escamas resistentes a magma y venenos: +16 DEF / +55 HP / +5 ATQ.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 5.2f;
        item.value = 580;
        item.iconId = "boots";
        item.equipSlot = EquipSlot::FEET;
        item.equipStats.defense = 16;
        item.equipStats.maxHpBonus = 55;
        item.equipStats.attackPower = 5;
        RegisterItem(item);
    }
    {
        ItemDefinition item;
        item.stringId = "dragon_gauntlets";
        item.name = "Garras del Dragón";
        item.description = "Guanteletes con garras doradas y calor ígneo abrasador: +15 DEF / +10 ATQ / +6% CRIT.";
        item.category = ItemCategory::EQUIPMENT;
        item.rarity = ItemRarity::LEGENDARY;
        item.maxStack = 1;
        item.weight = 4.0f;
        item.value = 540;
        item.iconId = "gloves";
        item.equipSlot = EquipSlot::GLOVES;
        item.equipStats.defense = 15;
        item.equipStats.attackPower = 10;
        item.equipStats.critChance = 6;
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
