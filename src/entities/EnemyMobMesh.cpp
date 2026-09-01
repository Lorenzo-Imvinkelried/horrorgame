#include "EnemyMob.h"
#include <cmath>
#include <algorithm>

static void AttachModel(std::vector<BoxDef>& targetList, const std::string& modelPath,
                        glm::vec3 offset, glm::vec3 scaleMultiplier,
                        const std::string& namePrefix, bool mirrorX = false)
{
    auto loadedBoxes = ModelLoader::Load(modelPath);
    if (loadedBoxes.empty()) return;

    for (auto& b : loadedBoxes) {
        glm::vec3 p = b.Pos;
        if (mirrorX) {
            p.x = -p.x;
        }

        b.Pos = p * scaleMultiplier + offset;
        b.Scale *= scaleMultiplier;

        if (!namePrefix.empty()) {
            b.Name = namePrefix + b.Name;
        }

        targetList.push_back(b);
    }
}

void EnemyMob::initMeshes() {
    m_baseBoxes.clear();

    if (m_type == EnemyType::CORRUPTED_WARRIOR) {
        // 1. Guerrero Caído
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.62f, 0.0f), glm::vec3(0.26f, 0.28f, 0.28f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.64f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.64f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.46f, 0.58f, 0.28f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(-0.32f, 1.15f, 0.0f), glm::vec3(0.16f, 0.48f, 0.16f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.32f, 1.15f, 0.0f), glm::vec3(0.16f, 0.48f, 0.16f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(-0.14f, 0.45f, 0.0f), glm::vec3(0.18f, 0.85f, 0.18f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.14f, 0.45f, 0.0f), glm::vec3(0.18f, 0.85f, 0.18f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "LEG_R" });

        // Progresión por noches: Noche 1 arranca con Armadura de Cuero; con el paso de las noches usan Hierro
        if (m_nightLevel <= 1) {
            AttachModel(m_baseBoxes, "assets/models/equipment/helm_leather.txt", glm::vec3(0.0f, 0.04f, 0.0f), glm::vec3(1.02f), "HELM_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather.txt", glm::vec3(0.0f, -0.03f, 0.0f), glm::vec3(1.02f), "ARMOR_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather_pants.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.0f), "LEG_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.02f), "BOOT_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather_gloves.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.04f), "GAUNTLET_");
        } else if (m_nightLevel == 2) {
            AttachModel(m_baseBoxes, "assets/models/equipment/helm_iron.txt", glm::vec3(0.0f, 0.06f, 0.0f), glm::vec3(1.02f), "HELM_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_plate.txt", glm::vec3(0.0f, -0.03f, 0.0f), glm::vec3(1.02f), "ARMOR_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather_pants.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.0f), "LEG_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.02f), "BOOT_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_leather_gloves.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.04f), "GAUNTLET_");
        } else {
            AttachModel(m_baseBoxes, "assets/models/equipment/helm_iron.txt", glm::vec3(0.0f, 0.06f, 0.0f), glm::vec3(1.02f), "HELM_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_plate.txt", glm::vec3(0.0f, -0.03f, 0.0f), glm::vec3(1.02f), "ARMOR_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_greaves.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.0f), "LEG_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.02f), "BOOT_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_gauntlets.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.04f), "GAUNTLET_");
        }
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_shield.txt", glm::vec3(-0.35f, 0.92f, 0.12f), glm::vec3(1.05f), "SHIELD_");
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_shortsword.txt", glm::vec3(0.32f, 0.90f, 0.08f), glm::vec3(1.05f), "SWORD_");
    }
    else if (m_type == EnemyType::BERSERKER_WARRIOR) {
        // 2. Guerrero Berserker Bárbaro (Set Berserker y Dos Hachas como el jugador)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.62f, 0.0f), glm::vec3(0.28f, 0.28f, 0.28f), glm::vec3(0.0f), glm::vec3(0.72f, 0.52f, 0.38f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.64f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.15f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.64f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.15f, 0.05f), "EYE_R" });

        // Torso musculoso con tatuaje de sangre
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.18f, 0.0f), glm::vec3(0.54f, 0.62f, 0.32f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.25f, 0.17f), glm::vec3(0.40f, 0.35f, 0.04f), glm::vec3(0.0f), glm::vec3(0.85f, 0.08f, 0.08f), "TATTOO" });

        m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.18f, 0.0f), glm::vec3(0.18f, 0.50f, 0.18f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.36f, 1.18f, 0.0f), glm::vec3(0.18f, 0.50f, 0.18f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(-0.16f, 0.45f, 0.0f), glm::vec3(0.20f, 0.85f, 0.20f), glm::vec3(0.0f), glm::vec3(0.38f, 0.26f, 0.15f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.16f, 0.45f, 0.0f), glm::vec3(0.20f, 0.85f, 0.20f), glm::vec3(0.0f), glm::vec3(0.38f, 0.26f, 0.15f), "LEG_R" });

        // Equipado con el set Berserker
        AttachModel(m_baseBoxes, "assets/models/equipment/helm_berserker.txt", glm::vec3(0.0f, 0.04f, 0.0f), glm::vec3(1.04f), "HELM_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.06f), "ARMOR_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker_pants.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.04f), "LEG_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.04f), "BOOT_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker_gauntlets.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.06f), "GAUNTLET_");

        // Dos Hachas de Asalto Berserker perfectamente erguidas en puños
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_berserker_onehand_axe.txt", glm::vec3(0.36f, 0.92f, 0.08f), glm::vec3(1.10f), "AXE_R_");
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_berserker_onehand_axe.txt", glm::vec3(-0.36f, 0.92f, 0.08f), glm::vec3(1.10f), "AXE_L_", true);
    }
    else if (m_type == EnemyType::DEATH_KNIGHT) {
        // 3. Caballero de la Muerte (Set Gótico de Acero Negro y Colosal Claymore)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.68f, 0.0f), glm::vec3(0.30f, 0.32f, 0.30f), glm::vec3(0.0f), glm::vec3(0.12f, 0.12f, 0.15f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.07f, 1.70f, 0.16f), glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.15f, 0.85f, 1.0f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.07f, 1.70f, 0.16f), glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.15f, 0.85f, 1.0f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.20f, 0.0f), glm::vec3(0.56f, 0.66f, 0.34f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, -0.20f), glm::vec3(0.55f, 0.95f, 0.06f), glm::vec3(0.0f), glm::vec3(0.08f, 0.12f, 0.24f), "CAPE" });

        m_baseBoxes.push_back({ glm::vec3(-0.34f, 1.18f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.16f, 0.16f, 0.20f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.34f, 1.18f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.16f, 0.16f, 0.20f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(-0.16f, 0.45f, 0.0f), glm::vec3(0.22f, 0.88f, 0.22f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.16f, 0.45f, 0.0f), glm::vec3(0.22f, 0.88f, 0.22f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "LEG_R" });

        // Equipado con el set del Caballero de la Muerte
        AttachModel(m_baseBoxes, "assets/models/equipment/helm_death_knight.txt", glm::vec3(0.0f, 0.08f, 0.0f), glm::vec3(1.08f), "HELM_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_death_knight.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.08f), "ARMOR_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_death_knight_greaves.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.06f), "LEG_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_death_knight_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.06f), "BOOT_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_death_knight_gauntlets.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.08f), "GAUNTLET_");
        
        // Gran Mandoble Colosal posicionado exactamente como en el player (sin deformaciones)
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_deathknight_greatsword.txt", glm::vec3(0.34f, 0.92f, 0.08f), glm::vec3(1.15f), "CLAYMORE_");
    }
    else if (m_type == EnemyType::SHADOW_ASSASSIN) {
        // 4. Asesino de las Sombras (Set de Sombras y Dagas)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.58f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.60f, 0.13f), glm::vec3(0.04f, 0.02f, 0.02f), glm::vec3(0.0f), glm::vec3(0.25f, 1.0f, 0.25f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.60f, 0.13f), glm::vec3(0.04f, 0.02f, 0.02f), glm::vec3(0.0f), glm::vec3(0.25f, 1.0f, 0.25f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.42f, 0.58f, 0.24f), glm::vec3(0.0f), glm::vec3(0.12f, 0.12f, 0.14f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.12f, 0.0f), glm::vec3(0.12f, 0.46f, 0.12f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.12f, 0.0f), glm::vec3(0.12f, 0.46f, 0.12f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.82f, 0.14f), glm::vec3(0.0f), glm::vec3(0.08f, 0.08f, 0.10f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.82f, 0.14f), glm::vec3(0.0f), glm::vec3(0.08f, 0.08f, 0.10f), "LEG_R" });

        // Equipado con el set de Sombras
        AttachModel(m_baseBoxes, "assets/models/equipment/helm_shadow.txt", glm::vec3(0.0f, 0.02f, 0.0f), glm::vec3(1.02f), "HELM_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_shadow.txt", glm::vec3(0.0f, -0.02f, 0.0f), glm::vec3(1.02f), "ARMOR_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_shadow_pants.txt", glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(1.0f), "LEG_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_shadow_boots.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "BOOT_");
        AttachModel(m_baseBoxes, "assets/models/equipment/armor_shadow_gloves.txt", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.02f), "GAUNTLET_");

        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_shadow_dagger.txt", glm::vec3(0.28f, 0.88f, 0.08f), glm::vec3(1.0f), "DAGGER_R_");
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_shadow_dagger.txt", glm::vec3(-0.28f, 0.88f, 0.08f), glm::vec3(1.0f), "DAGGER_L_", true);
    }
    else if (m_type == EnemyType::NEUTRAL_GIANT) {
        // Gigante Ancestral: Se construye con el modelo idéntico al player pero escalado uniformemente
        float giantScale = 1.60f;

        auto bodyBoxes = ModelLoader::Load("assets/models/equipment/player_body.txt");
        if (bodyBoxes.empty()) {
            bodyBoxes = ModelLoader::Load("assets/models/player.txt");
        }

        int giantVariant = rand() % 3;

        for (auto& b : bodyBoxes) {
            b.Pos *= giantScale;
            b.Scale *= giantScale;
            // Tono de piel de gigante ancestral (tono titánico / pétreo)
            if (b.Name.find("HEAD") != std::string::npos || b.Name.find("HAND") != std::string::npos || b.Name.find("NECK") != std::string::npos) {
                b.Color = glm::vec3(0.52f, 0.45f, 0.38f);
            } else if (giantVariant == 0 && (b.Name.find("TORSO") != std::string::npos || b.Name.find("ARM") != std::string::npos)) {
                // Variante Primitiva: Torso descubierto (color piel de gigante)
                b.Color = glm::vec3(0.48f, 0.42f, 0.35f);
            }
            m_baseBoxes.push_back(b);
        }

        // Ojos incandescentes del gigante
        m_baseBoxes.push_back({ glm::vec3(-0.06f * giantScale, 1.54f * giantScale, 0.14f * giantScale), glm::vec3(0.04f * giantScale, 0.03f * giantScale, 0.02f * giantScale), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f * giantScale, 1.54f * giantScale, 0.14f * giantScale), glm::vec3(0.04f * giantScale, 0.03f * giantScale, 0.02f * giantScale), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_R" });

        // Empuña el Hacha del Verdugo a escala colosal exactamente en su mano derecha
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_executioner_axe.txt", glm::vec3(0.28f, 0.76f, 0.08f) * giantScale, glm::vec3(giantScale * 1.05f), "CLUB_AXE_");

        if (giantVariant == 0) {
            // Variante 0: Gigante Primitivo (Sin armadura, con taparrabos y cuernos colosales de hueso/roca en la cabeza)
            m_baseBoxes.push_back({ glm::vec3(-0.16f * giantScale, 1.70f * giantScale, 0.0f), glm::vec3(0.08f * giantScale, 0.26f * giantScale, 0.08f * giantScale), glm::vec3(0.0f, 0.0f, -0.45f), glm::vec3(0.55f, 0.50f, 0.42f), "HORN_L" });
            m_baseBoxes.push_back({ glm::vec3( 0.16f * giantScale, 1.70f * giantScale, 0.0f), glm::vec3(0.08f * giantScale, 0.26f * giantScale, 0.08f * giantScale), glm::vec3(0.0f, 0.0f,  0.45f), glm::vec3(0.55f, 0.50f, 0.42f), "HORN_R" });
            // Cinturón de pelaje
            m_baseBoxes.push_back({ glm::vec3(0.0f, 0.95f * giantScale, 0.0f), glm::vec3(0.48f * giantScale, 0.16f * giantScale, 0.30f * giantScale), glm::vec3(0.0f), glm::vec3(0.32f, 0.22f, 0.14f), "PELT_BELT" });
        } else if (giantVariant == 1) {
            // Variante 1: Titán de Hierro (Armadura de Hierro y Yelmo de Hierro perfectamente escalados)
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_plate.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_ARMOR_");
            AttachModel(m_baseBoxes, "assets/models/equipment/helm_iron.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_HELM_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_iron_greaves.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_LEG_");
        } else {
            // Variante 2: Gigante Berserker (Arneses de batalla berserker y Yelmo de Calavera Berserker)
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_ARMOR_");
            AttachModel(m_baseBoxes, "assets/models/equipment/helm_berserker.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_HELM_");
            AttachModel(m_baseBoxes, "assets/models/equipment/armor_berserker_pants.txt", glm::vec3(0.0f), glm::vec3(giantScale), "GIANT_LEG_");
        }
    }
    else if (m_type == EnemyType::DARK_MAGE) {
        // Mago Sombrío
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.28f, 0.30f, 0.28f), glm::vec3(0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.65f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(0.90f, 0.15f, 0.95f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.65f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(0.90f, 0.15f, 0.95f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.05f, 0.0f), glm::vec3(0.44f, 0.85f, 0.32f), glm::vec3(0.0f), glm::vec3(0.16f, 0.10f, 0.22f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.45f, 0.0f), glm::vec3(0.48f, 0.75f, 0.36f), glm::vec3(0.0f), glm::vec3(0.14f, 0.08f, 0.20f), "ROBE_SKIRT" });

        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.15f, 0.08f), glm::vec3(0.14f, 0.45f, 0.14f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.15f, 0.0f), glm::vec3(0.14f, 0.45f, 0.14f), glm::vec3(0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 1.15f, 0.15f), glm::vec3(0.04f, 1.50f, 0.04f), glm::vec3(0.0f), glm::vec3(0.35f, 0.22f, 0.12f), "STAFF" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 1.95f, 0.15f), glm::vec3(0.12f, 0.16f, 0.12f), glm::vec3(0.0f), glm::vec3(0.45f, 0.30f, 0.15f), "STAFF_TOP" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 2.08f, 0.15f), glm::vec3(0.14f, 0.14f, 0.14f), glm::vec3(0.0f), glm::vec3(1.0f, 0.2f, 0.9f), "ORB" });

        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.35f, 0.0f), glm::vec3(0.14f, 0.70f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.14f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.35f, 0.0f), glm::vec3(0.14f, 0.70f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.14f), "LEG_R" });
    }
    else if (m_type == EnemyType::SKELETON_ARCHER) {
        // 5. Arquero Esqueleto (Calavera, Costillas, Columna, Pelvis, Carcaj y Arco de Caza)
        glm::vec3 colBone(0.88f, 0.85f, 0.78f);
        glm::vec3 colDarkBone(0.78f, 0.74f, 0.68f);
        glm::vec3 colEyeGlow(1.0f, 0.75f, 0.15f);

        // Cráneo
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.62f, 0.0f), glm::vec3(0.24f, 0.22f, 0.24f), glm::vec3(0.0f), colBone, "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.50f, 0.04f), glm::vec3(0.18f, 0.10f, 0.18f), glm::vec3(0.0f), colDarkBone, "HEAD" });
        // Ojos de fuego espectral ámbar
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.62f, 0.12f), glm::vec3(0.045f, 0.045f, 0.02f), glm::vec3(0.0f), colEyeGlow, "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.62f, 0.12f), glm::vec3(0.045f, 0.045f, 0.02f), glm::vec3(0.0f), colEyeGlow, "EYE_R" });

        // Capucha andrajosa de arquero
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.68f, -0.02f), glm::vec3(0.26f, 0.12f, 0.26f), glm::vec3(0.0f), glm::vec3(0.25f, 0.20f, 0.15f), "HEAD" });

        // Columna vertebral y Caja torácica (Costillar descarnado)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, -0.02f), glm::vec3(0.10f, 0.58f, 0.10f), glm::vec3(0.0f), colDarkBone, "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.34f, 0.02f), glm::vec3(0.38f, 0.14f, 0.24f), glm::vec3(0.0f), colBone, "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.20f, 0.02f), glm::vec3(0.34f, 0.12f, 0.22f), glm::vec3(0.0f), colBone, "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.06f, 0.02f), glm::vec3(0.30f, 0.10f, 0.20f), glm::vec3(0.0f), colBone, "TORSO" });
        // Pelvis ósea
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.88f, 0.0f), glm::vec3(0.32f, 0.14f, 0.20f), glm::vec3(0.0f), colDarkBone, "TORSO" });

        // Carcaj de cuero a la espalda con flechas
        m_baseBoxes.push_back({ glm::vec3(0.10f, 1.25f, -0.16f), glm::vec3(0.14f, 0.55f, 0.14f), glm::vec3(0.0f, 0.0f, 0.25f), glm::vec3(0.35f, 0.22f, 0.14f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.14f, 1.56f, -0.16f), glm::vec3(0.10f, 0.16f, 0.10f), glm::vec3(0.0f, 0.0f, 0.25f), glm::vec3(0.85f, 0.20f, 0.18f), "TORSO" });

        // Brazo izquierdo (Sostiene el arco)
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.22f, 0.0f), glm::vec3(0.09f, 0.44f, 0.09f), glm::vec3(0.0f), colBone, "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 0.88f, 0.12f), glm::vec3(0.08f, 0.38f, 0.08f), glm::vec3(0.0f), colBone, "ARM_L" });

        // Brazo derecho (Tensa la cuerda)
        m_baseBoxes.push_back({ glm::vec3(0.28f, 1.22f, 0.0f), glm::vec3(0.09f, 0.44f, 0.09f), glm::vec3(0.0f), colBone, "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.28f, 0.88f, 0.06f), glm::vec3(0.08f, 0.38f, 0.08f), glm::vec3(0.0f), colBone, "ARM_R" });

        // Piernas esqueléticas (Fémur, tibia y pie)
        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.10f, 0.85f, 0.10f), glm::vec3(0.0f), colBone, "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.04f, 0.06f), glm::vec3(0.10f, 0.08f, 0.18f), glm::vec3(0.0f), colDarkBone, "LEG_L" });

        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.10f, 0.85f, 0.10f), glm::vec3(0.0f), colBone, "LEG_R" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.04f, 0.06f), glm::vec3(0.10f, 0.08f, 0.18f), glm::vec3(0.0f), colDarkBone, "LEG_R" });

        // Arco de Caza empuñado en la mano izquierda
        AttachModel(m_baseBoxes, "assets/models/equipment/weapon_hunting_bow.txt", glm::vec3(-0.28f, 0.88f, 0.18f), glm::vec3(1.10f), "BOW_");
    }
    else if (m_type == EnemyType::TREANT) {
        // Árbol Viviente
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.50f, 0.0f), glm::vec3(0.85f, 2.80f, 0.85f), glm::vec3(0.0f), glm::vec3(0.32f, 0.22f, 0.12f), "TRUNK_BASE" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 3.20f, 0.0f), glm::vec3(2.20f, 1.40f, 2.20f), glm::vec3(0.0f), glm::vec3(0.16f, 0.38f, 0.14f), "LEAVES_LOWER" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 4.20f, 0.0f), glm::vec3(1.60f, 1.20f, 1.60f), glm::vec3(0.0f), glm::vec3(0.20f, 0.44f, 0.18f), "LEAVES_UPPER" });

        m_baseBoxes.push_back({ glm::vec3(-0.22f, 2.10f, 0.44f), glm::vec3(0.10f, 0.08f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.65f, 0.10f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.22f, 2.10f, 0.44f), glm::vec3(0.10f, 0.08f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.65f, 0.10f), "EYE_R" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.44f), glm::vec3(0.32f, 0.10f, 0.04f), glm::vec3(0.0f), glm::vec3(0.12f, 0.06f, 0.04f), "MOUTH" });

        m_baseBoxes.push_back({ glm::vec3(-0.65f, 2.00f, 0.10f), glm::vec3(0.30f, 1.60f, 0.30f), glm::vec3(0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.65f, 2.00f, 0.10f), glm::vec3(0.30f, 1.60f, 0.30f), glm::vec3(0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "ARM_R" });

        m_baseBoxes.push_back({ glm::vec3(-0.35f, 0.40f, 0.0f), glm::vec3(0.35f, 0.80f, 0.35f), glm::vec3(0.0f), glm::vec3(0.26f, 0.16f, 0.08f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.35f, 0.40f, 0.0f), glm::vec3(0.35f, 0.80f, 0.35f), glm::vec3(0.0f), glm::vec3(0.26f, 0.16f, 0.08f), "LEG_R" });
    }
    else if (m_type == EnemyType::VAMPIRE) {
        // Vampiro Sanguinario
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), glm::vec3(0.0f), glm::vec3(0.85f, 0.85f, 0.88f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.66f, 0.13f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.66f, 0.13f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_R" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.58f, 0.13f), glm::vec3(0.08f, 0.04f, 0.02f), glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), "FANGS" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.10f, -0.10f), glm::vec3(0.55f, 0.95f, 0.08f), glm::vec3(0.0f), glm::vec3(0.10f, 0.06f, 0.12f), "CAPE_BACK" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.10f, -0.05f), glm::vec3(0.50f, 0.90f, 0.04f), glm::vec3(0.0f), glm::vec3(0.65f, 0.05f, 0.08f), "CAPE_LINING" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.38f, 0.55f, 0.24f), glm::vec3(0.0f), glm::vec3(0.15f, 0.12f, 0.18f), "TORSO" });

        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.15f, 0.0f), glm::vec3(0.12f, 0.48f, 0.12f), glm::vec3(0.0f), glm::vec3(0.12f, 0.08f, 0.14f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 0.88f, 0.08f), glm::vec3(0.08f, 0.14f, 0.08f), glm::vec3(0.0f), glm::vec3(0.80f, 0.10f, 0.10f), "CLAW_L" });

        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.15f, 0.0f), glm::vec3(0.12f, 0.48f, 0.12f), glm::vec3(0.0f), glm::vec3(0.12f, 0.08f, 0.14f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 0.88f, 0.08f), glm::vec3(0.08f, 0.14f, 0.08f), glm::vec3(0.0f), glm::vec3(0.80f, 0.10f, 0.10f), "CLAW_R" });

        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.85f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.12f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.85f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.12f), "LEG_R" });
    }
}
