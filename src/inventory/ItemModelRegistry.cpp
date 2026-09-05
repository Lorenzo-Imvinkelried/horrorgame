#include "ItemModelRegistry.h"
#include "ModelLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

ItemModelRegistry& ItemModelRegistry::Get() {
    static ItemModelRegistry s_instance;
    return s_instance;
}

ItemModelRegistry::ItemModelRegistry() {}

ItemModelRegistry::~ItemModelRegistry() {
    for (auto& [id, mesh] : m_meshes) {
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    }
    if (m_defaultMesh.vao) glDeleteVertexArrays(1, &m_defaultMesh.vao);
    if (m_defaultMesh.vbo) glDeleteBuffers(1, &m_defaultMesh.vbo);
}

void ItemModelRegistry::registerBoxes(const std::string& stringId, const std::vector<BoxDef>& rawBoxes) {
    if (rawBoxes.empty()) return;

    // Calculate bounding box to normalize to [-0.45, 0.45]
    glm::vec3 minB(1e9f), maxB(-1e9f);
    for (const auto& b : rawBoxes) {
        glm::vec3 half = b.Scale * 0.5f;
        minB = glm::min(minB, b.Pos - half);
        maxB = glm::max(maxB, b.Pos + half);
    }

    glm::vec3 center = (minB + maxB) * 0.5f;
    glm::vec3 size = maxB - minB;
    float maxDim = std::max({size.x, size.y, size.z, 0.001f});
    float targetDim = 0.85f;
    float scaleFactor = targetDim / maxDim;

    std::vector<BoxDef> normalizedBoxes;
    normalizedBoxes.reserve(rawBoxes.size());

    for (const auto& b : rawBoxes) {
        BoxDef nb = b;
        nb.Pos = (b.Pos - center) * scaleFactor;
        nb.Scale = b.Scale * scaleFactor;
        normalizedBoxes.push_back(nb);
    }

    std::vector<float> vertices;
    ModelLoader::GenerateMesh(normalizedBoxes, vertices);

    if (vertices.empty()) return;

    ItemMesh mesh;
    mesh.vertexCount = vertices.size() / 11;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Layout: Pos (0), Color (1), UV (2), Normal (3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    m_meshes[stringId] = mesh;
}

void ItemModelRegistry::Init() {
    if (m_initialized) return;
    buildAllItemModels();
    m_initialized = true;
}

void ItemModelRegistry::buildAllItemModels() {
    // 1. WEAPONS
    registerBoxes("steel_shortsword", ModelLoader::Load("assets/models/equipment/weapon_shortsword.txt"));
    registerBoxes("iron_greatsword", ModelLoader::Load("assets/models/equipment/weapon_greatsword.txt"));
    registerBoxes("deathknight_greatsword", ModelLoader::Load("assets/models/equipment/weapon_deathknight_greatsword.txt"));
    registerBoxes("berserker_axe", ModelLoader::Load("assets/models/equipment/weapon_berserker_axe.txt"));
    registerBoxes("iron_shield", ModelLoader::Load("assets/models/equipment/weapon_shield.txt"));

    // Nuevas Armas: Hachas de 1 Mano, Hachas de 2 Manos, Espadas y Dagas
    registerBoxes("iron_hatchet", ModelLoader::Load("assets/models/equipment/weapon_iron_hatchet.txt"));
    registerBoxes("berserker_onehand_axe", ModelLoader::Load("assets/models/equipment/weapon_berserker_onehand_axe.txt"));
    registerBoxes("executioner_axe", ModelLoader::Load("assets/models/equipment/weapon_executioner_axe.txt"));
    registerBoxes("paladin_longsword", ModelLoader::Load("assets/models/equipment/weapon_paladin_longsword.txt"));
    registerBoxes("dragonslayer_greatsword", ModelLoader::Load("assets/models/equipment/weapon_dragonslayer_greatsword.txt"));
    registerBoxes("shadow_dagger", ModelLoader::Load("assets/models/equipment/weapon_shadow_dagger.txt"));

    // Cursed sword (Dark obsidian with violet blade)
    {
        auto boxes = ModelLoader::Load("assets/models/equipment/weapon_shortsword.txt");
        for (auto& b : boxes) b.Color = glm::vec3(0.20f, 0.12f, 0.32f);
        registerBoxes("cursed_sword", boxes);
    }
    // Frost claymore (Glowing icy blue)
    {
        auto boxes = ModelLoader::Load("assets/models/equipment/weapon_greatsword.txt");
        for (auto& b : boxes) b.Color = glm::vec3(0.35f, 0.75f, 1.0f);
        registerBoxes("frost_claymore", boxes);
    }

    // Hunting Bow (Uses official hunting bow model matching the skeleton archer)
    registerBoxes("hunting_bow", ModelLoader::Load("assets/models/equipment/weapon_hunting_bow.txt"));
    // Dragon Bone Bow (Crimson)
    {
        std::vector<BoxDef> bow;
        bow.push_back({glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.10f, 0.35f, 0.10f), glm::vec3(0.85f, 0.20f, 0.20f), glm::vec3(0), "GRIP"});
        bow.push_back({glm::vec3(0.08f, 0.32f, 0.0f), glm::vec3(0.08f, 0.40f, 0.08f), glm::vec3(0.95f, 0.30f, 0.25f), glm::vec3(0, 0, -0.25f), "LIMB_TOP"});
        bow.push_back({glm::vec3(0.08f, -0.32f, 0.0f), glm::vec3(0.08f, 0.40f, 0.08f), glm::vec3(0.95f, 0.30f, 0.25f), glm::vec3(0, 0, 0.25f), "LIMB_BOT"});
        bow.push_back({glm::vec3(0.22f, 0.65f, 0.0f), glm::vec3(0.07f, 0.30f, 0.07f), glm::vec3(1.0f, 0.85f, 0.25f), glm::vec3(0, 0, -0.55f), "TIP_TOP"});
        bow.push_back({glm::vec3(0.22f, -0.65f, 0.0f), glm::vec3(0.07f, 0.30f, 0.07f), glm::vec3(1.0f, 0.85f, 0.25f), glm::vec3(0, 0, 0.55f), "TIP_BOT"});
        bow.push_back({glm::vec3(0.32f, 0.0f, 0.0f), glm::vec3(0.02f, 1.50f, 0.02f), glm::vec3(1.0f, 0.45f, 0.20f), glm::vec3(0), "STRING"});
        registerBoxes("dragon_bone_bow", bow);
    }

    // 2. ARMORS & HELMETS
    auto leatherArmor = ModelLoader::Load("assets/models/equipment/armor_leather.txt");
    registerBoxes("leather_armor", leatherArmor);
    registerBoxes("armor_leather", leatherArmor);

    registerBoxes("iron_armor", ModelLoader::Load("assets/models/equipment/armor_iron_plate.txt"));
    registerBoxes("deathknight_armor", ModelLoader::Load("assets/models/equipment/armor_death_knight.txt"));

    auto leatherCap = ModelLoader::Load("assets/models/equipment/helm_leather.txt");
    registerBoxes("leather_cap", leatherCap);
    registerBoxes("helm_leather", leatherCap);

    registerBoxes("iron_helm", ModelLoader::Load("assets/models/equipment/helm_iron.txt"));
    registerBoxes("deathknight_helm", ModelLoader::Load("assets/models/equipment/helm_death_knight.txt"));

    auto leatherGloves = ModelLoader::Load("assets/models/equipment/armor_leather_gloves.txt");
    registerBoxes("leather_gloves", leatherGloves);
    registerBoxes("armor_leather_gloves", leatherGloves);

    registerBoxes("iron_gauntlets", ModelLoader::Load("assets/models/equipment/armor_iron_gauntlets.txt"));
    registerBoxes("deathknight_gauntlets", ModelLoader::Load("assets/models/equipment/armor_death_knight_gauntlets.txt"));

    auto leatherPants = ModelLoader::Load("assets/models/equipment/armor_leather_pants.txt");
    registerBoxes("leather_pants", leatherPants);
    registerBoxes("armor_leather_pants", leatherPants);

    registerBoxes("iron_greaves", ModelLoader::Load("assets/models/equipment/armor_iron_greaves.txt"));
    registerBoxes("deathknight_greaves", ModelLoader::Load("assets/models/equipment/armor_death_knight_greaves.txt"));

    auto leatherBoots = ModelLoader::Load("assets/models/equipment/armor_leather_boots.txt");
    registerBoxes("leather_boots", leatherBoots);
    registerBoxes("armor_leather_boots", leatherBoots);

    registerBoxes("iron_boots", ModelLoader::Load("assets/models/equipment/armor_iron_boots.txt"));
    registerBoxes("deathknight_boots", ModelLoader::Load("assets/models/equipment/armor_death_knight_boots.txt"));

    // SET BERSERKER (Bárbaro)
    registerBoxes("berserker_helm", ModelLoader::Load("assets/models/equipment/helm_berserker.txt"));
    registerBoxes("berserker_armor", ModelLoader::Load("assets/models/equipment/armor_berserker.txt"));
    registerBoxes("berserker_pants", ModelLoader::Load("assets/models/equipment/armor_berserker_pants.txt"));
    registerBoxes("berserker_boots", ModelLoader::Load("assets/models/equipment/armor_berserker_boots.txt"));
    registerBoxes("berserker_gauntlets", ModelLoader::Load("assets/models/equipment/armor_berserker_gauntlets.txt"));

    // SET SHADOW ASSASSIN (Sombras)
    registerBoxes("shadow_hood", ModelLoader::Load("assets/models/equipment/helm_shadow.txt"));
    registerBoxes("shadow_garb", ModelLoader::Load("assets/models/equipment/armor_shadow.txt"));
    registerBoxes("shadow_pants", ModelLoader::Load("assets/models/equipment/armor_shadow_pants.txt"));
    registerBoxes("shadow_boots", ModelLoader::Load("assets/models/equipment/armor_shadow_boots.txt"));
    registerBoxes("shadow_gloves", ModelLoader::Load("assets/models/equipment/armor_shadow_gloves.txt"));

    // SET DRAGONSCALE (Dragón)
    registerBoxes("dragon_helm", ModelLoader::Load("assets/models/equipment/helm_dragon.txt"));
    registerBoxes("dragon_armor", ModelLoader::Load("assets/models/equipment/armor_dragon.txt"));
    registerBoxes("dragon_chest", ModelLoader::Load("assets/models/equipment/armor_dragon.txt"));
    registerBoxes("dragon_pants", ModelLoader::Load("assets/models/equipment/armor_dragon_pants.txt"));
    registerBoxes("dragon_boots", ModelLoader::Load("assets/models/equipment/armor_dragon_boots.txt"));
    registerBoxes("dragon_gauntlets", ModelLoader::Load("assets/models/equipment/armor_dragon_gauntlets.txt"));

    // 3. CONSUMABLES & MEDICINE
    // Venda / Medicina (Rollo de vendas de lino estéril con cruz medicinal roja y ungüento)
    {
        std::vector<BoxDef> venda;
        // Rollo cilíndrico de lino blanco estéril
        venda.push_back({glm::vec3(0, -0.05f, 0), glm::vec3(0.56f, 0.45f, 0.56f), glm::vec3(0), glm::vec3(0.95f, 0.94f, 0.92f), "BANDAGE_ROLL"});
        // Núcleo interior hueco
        venda.push_back({glm::vec3(0, -0.05f, 0), glm::vec3(0.22f, 0.46f, 0.22f), glm::vec3(0), glm::vec3(0.76f, 0.76f, 0.74f), "HOLLOW_CORE"});
        // Tira de venda desplegada
        venda.push_back({glm::vec3(0.28f, -0.15f, 0.10f), glm::vec3(0.04f, 0.26f, 0.36f), glm::vec3(0), glm::vec3(0.98f, 0.98f, 0.96f), "FLAP"});
        // Cruz medicinal roja
        venda.push_back({glm::vec3(0.0f, -0.05f, 0.29f), glm::vec3(0.10f, 0.28f, 0.03f), glm::vec3(0), glm::vec3(0.95f, 0.12f, 0.15f), "CROSS_V"});
        venda.push_back({glm::vec3(0.0f, -0.05f, 0.29f), glm::vec3(0.28f, 0.10f, 0.03f), glm::vec3(0), glm::vec3(0.95f, 0.12f, 0.15f), "CROSS_H"});
        // Ungüento herbal verde curativo
        venda.push_back({glm::vec3(0.0f, 0.18f, 0.0f), glm::vec3(0.40f, 0.06f, 0.40f), glm::vec3(0), glm::vec3(0.28f, 0.72f, 0.22f), "HERB_SALVE"});
        registerBoxes("potion_health", venda);
        registerBoxes("venda", venda);
        registerBoxes("bandage", venda);
    }
    // Éter / Eter Corrupto / Mana (Frasco alquímico con éter azul celeste brillante pulsante)
    {
        std::vector<BoxDef> eter;
        // Núcleo de éter cian luminoso
        eter.push_back({glm::vec3(0, -0.08f, 0), glm::vec3(0.46f, 0.62f, 0.46f), glm::vec3(0), glm::vec3(0.18f, 0.85f, 1.0f), "ETHER_CORE"});
        // Brillo interior de maná ultrabrillante
        eter.push_back({glm::vec3(0, -0.08f, 0), glm::vec3(0.26f, 0.42f, 0.26f), glm::vec3(0), glm::vec3(0.70f, 0.98f, 1.0f), "MANA_GLOW"});
        // Frasco de cristal alquímico
        eter.push_back({glm::vec3(0, -0.08f, 0), glm::vec3(0.50f, 0.66f, 0.50f), glm::vec3(0), glm::vec3(0.85f, 0.95f, 1.0f), "GLASS"});
        // Cuello de cristal
        eter.push_back({glm::vec3(0, 0.32f, 0), glm::vec3(0.22f, 0.20f, 0.22f), glm::vec3(0), glm::vec3(0.80f, 0.92f, 1.0f), "NECK"});
        // Anillo de oro místico
        eter.push_back({glm::vec3(0, 0.42f, 0), glm::vec3(0.26f, 0.06f, 0.26f), glm::vec3(0), glm::vec3(0.95f, 0.80f, 0.25f), "GOLD_RING"});
        // Tapón de corcho
        eter.push_back({glm::vec3(0, 0.48f, 0), glm::vec3(0.20f, 0.12f, 0.20f), glm::vec3(0), glm::vec3(0.55f, 0.38f, 0.22f), "CORK"});
        registerBoxes("potion_mana", eter);
        registerBoxes("eter", eter);
        registerBoxes("aether", eter);
    }
    // Vial de Sangre (Tubo de ensayo alquímico con sangre fresca carmesí y coágulo)
    {
        std::vector<BoxDef> vial;
        // Sangre fresca carmesí intensa
        vial.push_back({glm::vec3(0, -0.06f, 0), glm::vec3(0.20f, 0.70f, 0.20f), glm::vec3(0), glm::vec3(0.85f, 0.06f, 0.10f), "BLOOD"});
        // Coágulo oscuro inferior
        vial.push_back({glm::vec3(0, -0.36f, 0), glm::vec3(0.19f, 0.14f, 0.19f), glm::vec3(0), glm::vec3(0.45f, 0.02f, 0.05f), "CLOT"});
        // Tubo de ensayo de cristal
        vial.push_back({glm::vec3(0, 0.0f, 0), glm::vec3(0.26f, 0.86f, 0.26f), glm::vec3(0), glm::vec3(0.88f, 0.96f, 1.0f), "GLASS"});
        // Borde superior de cristal
        vial.push_back({glm::vec3(0, 0.44f, 0), glm::vec3(0.30f, 0.06f, 0.30f), glm::vec3(0), glm::vec3(0.90f, 0.98f, 1.0f), "RIM"});
        // Tapón de cera/corcho
        vial.push_back({glm::vec3(0, 0.49f, 0), glm::vec3(0.22f, 0.12f, 0.22f), glm::vec3(0), glm::vec3(0.60f, 0.40f, 0.22f), "STOPPER"});
        registerBoxes("blood_vial", vial);
        registerBoxes("vial", vial);
    }
    // Carne Cruda (Corte de chuleta gruesa con hueso blanco y veta de grasa)
    {
        std::vector<BoxDef> meat;
        // Músculo de carne roja fresca
        meat.push_back({glm::vec3(0, 0, 0), glm::vec3(0.68f, 0.24f, 0.46f), glm::vec3(0), glm::vec3(0.85f, 0.16f, 0.18f), "STEAK"});
        // Veta magra marmoleada
        meat.push_back({glm::vec3(-0.06f, 0.01f, 0.04f), glm::vec3(0.42f, 0.25f, 0.22f), glm::vec3(0), glm::vec3(0.95f, 0.28f, 0.30f), "MARBLING"});
        // Hueso blanco limpio
        meat.push_back({glm::vec3(0.36f, 0.02f, 0.0f), glm::vec3(0.28f, 0.16f, 0.16f), glm::vec3(0), glm::vec3(0.96f, 0.96f, 0.92f), "BONE"});
        // Médula ósea central
        meat.push_back({glm::vec3(0.49f, 0.02f, 0.0f), glm::vec3(0.04f, 0.10f, 0.10f), glm::vec3(0), glm::vec3(0.85f, 0.40f, 0.35f), "MARROW"});
        // Grasa perimetral
        meat.push_back({glm::vec3(-0.10f, 0.0f, 0.23f), glm::vec3(0.55f, 0.22f, 0.06f), glm::vec3(0), glm::vec3(0.98f, 0.92f, 0.82f), "FAT"});
        registerBoxes("raw_meat", meat);
        registerBoxes("carne", meat);
        registerBoxes("meat", meat);
    }
    // Dragon Heart (Corazón pulsante dorado y carmesí)
    {
        std::vector<BoxDef> heart;
        heart.push_back({glm::vec3(0, 0, 0), glm::vec3(0.55f, 0.60f, 0.55f), glm::vec3(0), glm::vec3(0.85f, 0.10f, 0.10f), "CORE"});
        heart.push_back({glm::vec3(0.18f, 0.22f, 0), glm::vec3(0.38f, 0.38f, 0.45f), glm::vec3(0), glm::vec3(0.95f, 0.15f, 0.15f), "LOBE_R"});
        heart.push_back({glm::vec3(-0.18f, 0.22f, 0), glm::vec3(0.38f, 0.38f, 0.45f), glm::vec3(0), glm::vec3(0.95f, 0.15f, 0.15f), "LOBE_L"});
        heart.push_back({glm::vec3(0, 0.42f, 0), glm::vec3(0.20f, 0.25f, 0.20f), glm::vec3(0), glm::vec3(1.0f, 0.75f, 0.20f), "AORTA"});
        registerBoxes("dragon_heart", heart);
    }

    // 4. MATERIALS & ARTIFACTS
    // Tronco de Madera (Tronco cilíndrico de roble con corteza texturada y anillos aserrados)
    {
        std::vector<BoxDef> log;
        // Corteza de roble marrón texturada
        log.push_back({glm::vec3(0, 0, 0), glm::vec3(0.42f, 0.95f, 0.42f), glm::vec3(0), glm::vec3(0.45f, 0.28f, 0.14f), "BARK"});
        // Placas rugosas de corteza
        log.push_back({glm::vec3(0.02f, 0.15f, 0.20f), glm::vec3(0.24f, 0.45f, 0.08f), glm::vec3(0), glm::vec3(0.35f, 0.20f, 0.10f), "BARK_TEXTURE"});
        // Nudo de rama cortada
        log.push_back({glm::vec3(0.20f, -0.10f, 0.0f), glm::vec3(0.12f, 0.16f, 0.12f), glm::vec3(0), glm::vec3(0.32f, 0.18f, 0.08f), "BRANCH_KNOT"});
        // Anillos de madera aserrada superior
        log.push_back({glm::vec3(0, 0.48f, 0), glm::vec3(0.38f, 0.03f, 0.38f), glm::vec3(0), glm::vec3(0.82f, 0.68f, 0.46f), "RINGS_TOP"});
        // Núcleo del tronco
        log.push_back({glm::vec3(0, 0.49f, 0), glm::vec3(0.20f, 0.03f, 0.20f), glm::vec3(0), glm::vec3(0.70f, 0.52f, 0.32f), "CORE_TOP"});
        // Madera aserrada inferior
        log.push_back({glm::vec3(0, -0.48f, 0), glm::vec3(0.38f, 0.03f, 0.38f), glm::vec3(0), glm::vec3(0.82f, 0.68f, 0.46f), "RINGS_BOT"});
        registerBoxes("wood_log", log);
        registerBoxes("tronco", log);
        registerBoxes("wood", log);
    }
    // Stone Rock
    {
        std::vector<BoxDef> rock;
        rock.push_back({glm::vec3(0, 0, 0), glm::vec3(0.65f, 0.55f, 0.60f), glm::vec3(0.1f, 0.2f, 0), glm::vec3(0.55f, 0.55f, 0.58f), "CORE"});
        rock.push_back({glm::vec3(0.18f, 0.12f, 0.15f), glm::vec3(0.40f, 0.35f, 0.42f), glm::vec3(-0.2f, 0.3f, 0), glm::vec3(0.68f, 0.68f, 0.72f), "SHARD_1"});
        rock.push_back({glm::vec3(-0.15f, -0.10f, 0.12f), glm::vec3(0.38f, 0.32f, 0.38f), glm::vec3(0.3f, -0.2f, 0), glm::vec3(0.42f, 0.42f, 0.46f), "SHARD_2"});
        registerBoxes("stone_rock", rock);
    }
    // Piel de Bestia (Piel gruesa enrollada con correas de cuero y hebillas de bronce)
    {
        std::vector<BoxDef> pelt;
        // Rollo de piel de oso/lobo castaño
        pelt.push_back({glm::vec3(0, 0, 0), glm::vec3(0.45f, 0.88f, 0.38f), glm::vec3(0), glm::vec3(0.52f, 0.34f, 0.20f), "FUR_ROLL"});
        // Borde esponjoso de pelaje claro
        pelt.push_back({glm::vec3(0.0f, 0.0f, 0.18f), glm::vec3(0.42f, 0.82f, 0.08f), glm::vec3(0), glm::vec3(0.72f, 0.54f, 0.36f), "FUR_TRIM"});
        // Correa de cuero izquierda
        pelt.push_back({glm::vec3(-0.22f, 0.0f, 0.0f), glm::vec3(0.10f, 0.92f, 0.42f), glm::vec3(0), glm::vec3(0.22f, 0.14f, 0.08f), "STRAP_1"});
        // Hebilla de bronce izquierda
        pelt.push_back({glm::vec3(-0.22f, 0.0f, 0.22f), glm::vec3(0.12f, 0.10f, 0.05f), glm::vec3(0), glm::vec3(0.90f, 0.75f, 0.25f), "BUCKLE_1"});
        // Correa de cuero derecha
        pelt.push_back({glm::vec3(0.22f, 0.0f, 0.0f), glm::vec3(0.10f, 0.92f, 0.42f), glm::vec3(0), glm::vec3(0.22f, 0.14f, 0.08f), "STRAP_2"});
        // Hebilla de bronce derecha
        pelt.push_back({glm::vec3(0.22f, 0.0f, 0.22f), glm::vec3(0.12f, 0.10f, 0.05f), glm::vec3(0), glm::vec3(0.90f, 0.75f, 0.25f), "BUCKLE_2"});
        registerBoxes("beast_pelt", pelt);
        registerBoxes("piel", pelt);
        registerBoxes("pelt", pelt);
    }
    // Hunting Arrow
    {
        std::vector<BoxDef> arrow;
        arrow.push_back({glm::vec3(0, 0, 0), glm::vec3(0.05f, 1.25f, 0.05f), glm::vec3(0), glm::vec3(0.65f, 0.50f, 0.30f), "SHAFT"});
        arrow.push_back({glm::vec3(0, 0.65f, 0), glm::vec3(0.18f, 0.24f, 0.06f), glm::vec3(0), glm::vec3(0.85f, 0.85f, 0.90f), "BROADHEAD"});
        arrow.push_back({glm::vec3(0, -0.55f, 0), glm::vec3(0.22f, 0.28f, 0.02f), glm::vec3(0), glm::vec3(0.95f, 0.20f, 0.20f), "FLETCHING_1"});
        arrow.push_back({glm::vec3(0, -0.55f, 0), glm::vec3(0.02f, 0.28f, 0.22f), glm::vec3(0), glm::vec3(0.95f, 0.20f, 0.20f), "FLETCHING_2"});
        registerBoxes("hunting_arrow", arrow);
    }
    // Arcane Scroll
    {
        std::vector<BoxDef> scroll;
        scroll.push_back({glm::vec3(0, 0, 0), glm::vec3(0.35f, 0.85f, 0.35f), glm::vec3(0, 0, 0.78f), glm::vec3(0.92f, 0.88f, 0.75f), "PARCHMENT"});
        scroll.push_back({glm::vec3(0, 0, 0), glm::vec3(0.12f, 0.88f, 0.37f), glm::vec3(0, 0, 0.78f), glm::vec3(0.75f, 0.15f, 0.20f), "RIBBON"});
        scroll.push_back({glm::vec3(0, 0, 0.20f), glm::vec3(0.18f, 0.18f, 0.06f), glm::vec3(0), glm::vec3(0.95f, 0.80f, 0.20f), "SEAL"});
        registerBoxes("arcane_scroll", scroll);
    }
    // Shadow Ring
    {
        std::vector<BoxDef> ring;
        ring.push_back({glm::vec3(0, 0, 0), glm::vec3(0.55f, 0.22f, 0.55f), glm::vec3(0), glm::vec3(0.18f, 0.18f, 0.22f), "BAND"});
        ring.push_back({glm::vec3(0, 0.18f, 0), glm::vec3(0.24f, 0.20f, 0.24f), glm::vec3(0), glm::vec3(0.75f, 0.25f, 0.95f), "GEM"});
        registerBoxes("shadow_ring", ring);
    }
    // Vampiric Ring
    {
        std::vector<BoxDef> ring;
        ring.push_back({glm::vec3(0, 0, 0), glm::vec3(0.55f, 0.22f, 0.55f), glm::vec3(0), glm::vec3(0.95f, 0.80f, 0.25f), "BAND"});
        ring.push_back({glm::vec3(0, 0.18f, 0), glm::vec3(0.25f, 0.20f, 0.25f), glm::vec3(0), glm::vec3(0.95f, 0.08f, 0.12f), "RUBY"});
        registerBoxes("vampiric_ring", ring);
    }
    // Ancient Amulet
    {
        std::vector<BoxDef> amulet;
        amulet.push_back({glm::vec3(0, -0.15f, 0), glm::vec3(0.55f, 0.55f, 0.10f), glm::vec3(0), glm::vec3(0.95f, 0.82f, 0.25f), "MEDALLION"});
        amulet.push_back({glm::vec3(0, -0.15f, 0.06f), glm::vec3(0.22f, 0.22f, 0.04f), glm::vec3(0), glm::vec3(0.25f, 0.85f, 0.95f), "RUNE_EYE"});
        amulet.push_back({glm::vec3(0, 0.25f, 0), glm::vec3(0.45f, 0.45f, 0.04f), glm::vec3(0), glm::vec3(0.85f, 0.70f, 0.20f), "CHAIN"});
        registerBoxes("ancient_amulet", amulet);
    }

    // Dragon Scale
    {
        std::vector<BoxDef> scale;
        scale.push_back({glm::vec3(0, 0, 0), glm::vec3(0.50f, 0.65f, 0.12f), glm::vec3(0), glm::vec3(0.85f, 0.12f, 0.12f), "SCALE_MAIN"});
        scale.push_back({glm::vec3(0, 0, 0.04f), glm::vec3(0.32f, 0.45f, 0.10f), glm::vec3(0), glm::vec3(1.00f, 0.40f, 0.15f), "SCALE_CORE"});
        registerBoxes("dragon_scale", scale);
    }
    // Dragon Scale Ring
    {
        std::vector<BoxDef> ring;
        ring.push_back({glm::vec3(0, 0, 0), glm::vec3(0.55f, 0.22f, 0.55f), glm::vec3(0), glm::vec3(0.95f, 0.80f, 0.20f), "BAND"});
        ring.push_back({glm::vec3(0, 0.18f, 0), glm::vec3(0.28f, 0.22f, 0.28f), glm::vec3(0), glm::vec3(0.95f, 0.15f, 0.10f), "DRAGON_GEM"});
        registerBoxes("dragon_scale_ring", ring);
    }

    // Default Fallback: Adventurer Sack
    {
        std::vector<BoxDef> sack;
        sack.push_back({glm::vec3(0, -0.10f, 0), glm::vec3(0.65f, 0.65f, 0.65f), glm::vec3(0), glm::vec3(0.60f, 0.45f, 0.28f), "SACK_BODY"});
        sack.push_back({glm::vec3(0, 0.30f, 0), glm::vec3(0.35f, 0.22f, 0.35f), glm::vec3(0), glm::vec3(0.50f, 0.35f, 0.22f), "SACK_TOP"});
        sack.push_back({glm::vec3(0, 0.22f, 0), glm::vec3(0.40f, 0.08f, 0.40f), glm::vec3(0), glm::vec3(0.85f, 0.75f, 0.35f), "ROPE"});
        registerBoxes("default_sack", sack);
    }
}

const ItemMesh* ItemModelRegistry::GetMesh(const std::string& stringId) const {
    auto it = m_meshes.find(stringId);
    if (it != m_meshes.end()) {
        return &it->second;
    }
    auto itDef = m_meshes.find("default_sack");
    if (itDef != m_meshes.end()) {
        return &itDef->second;
    }
    return nullptr;
}

void ItemModelRegistry::RenderItemInSlot(const std::string& stringId,
                                        int vpX, int vpY, int vpW, int vpH,
                                        bool isHovered, float globalTime,
                                        GLuint shaderProgram)
{
    if (vpW <= 4 || vpH <= 4) return;

    auto it = m_meshes.find(stringId);
    if (it == m_meshes.end()) {
        it = m_meshes.find("default_sack");
        if (it == m_meshes.end()) return;
    }

    const ItemMesh& mesh = it->second;
    if (mesh.vao == 0 || mesh.vertexCount == 0) return;

    glEnable(GL_SCISSOR_TEST);
    glScissor(vpX, vpY, vpW, vpH);
    glViewport(vpX, vpY, vpW, vpH);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Setup 3D camera
    glm::mat4 proj = glm::perspective(glm::radians(34.0f), (float)vpW / (float)vpH, 0.1f, 20.0f);
    glm::vec3 camPos(0.0f, 0.15f, 2.15f);
    glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
    glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

    // Interactive MU-style Rotation:
    // If hovered: continuous 360 degree spin around Y axis with slight pitch bob
    // If idle: pleasing static 3D isometric display angle
    float rotY = isHovered ? (globalTime * 3.6f) : 0.48f;
    float rotX = isHovered ? (sin(globalTime * 2.5f) * 0.22f + 0.18f) : 0.28f;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(glGetUniformLocation(shaderProgram, "u_ViewPos"), camPos.x, camPos.y, camPos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_LightDir"), 0.40f, -0.80f, 0.45f);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_LightColor"), 1.0f, 0.98f, 0.92f);
    glUniform3f(glGetUniformLocation(shaderProgram, "u_AmbientColor"), 0.45f, 0.45f, 0.50f);

    // CRITICAL: u_IsDebug = 3 instructs ps1.frag to passthrough vertex colors directly without texture discard
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_Snap"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);

    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.vertexCount);
    glBindVertexArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
}
