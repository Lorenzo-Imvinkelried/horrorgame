#include "EnemyMob.h"

void EnemyMob::initMeshes() {
    m_baseBoxes.clear();

    if (m_type == EnemyType::CORRUPTED_WARRIOR) {
        // 1. Guerrero Caído (Espada y Escudo)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.26f, 0.28f, 0.28f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.66f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3(0.06f, 1.66f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.48f, 0.60f, 0.28f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.88f, 0.0f), glm::vec3(0.44f, 0.12f, 0.26f), glm::vec3(0.0f), glm::vec3(0.45f, 0.08f, 0.08f), "BELT" });

        m_baseBoxes.push_back({ glm::vec3(-0.32f, 1.15f, 0.0f), glm::vec3(0.16f, 0.48f, 0.16f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.12f, 0.15f), glm::vec3(0.08f, 0.58f, 0.42f), glm::vec3(0.0f), glm::vec3(0.18f, 0.18f, 0.22f), "SHIELD" });

        m_baseBoxes.push_back({ glm::vec3(0.32f, 1.15f, 0.0f), glm::vec3(0.16f, 0.48f, 0.16f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.25f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.34f, 0.96f, 0.08f), glm::vec3(0.06f, 0.16f, 0.06f), glm::vec3(0.0f), glm::vec3(0.35f, 0.25f, 0.15f), "SWORD_HILT" });
        m_baseBoxes.push_back({ glm::vec3(0.34f, 1.05f, 0.08f), glm::vec3(0.06f, 0.04f, 0.24f), glm::vec3(0.0f), glm::vec3(0.30f, 0.30f, 0.35f), "SWORD_GUARD" });
        m_baseBoxes.push_back({ glm::vec3(0.34f, 1.45f, 0.08f), glm::vec3(0.04f, 0.78f, 0.12f), glm::vec3(0.0f), glm::vec3(0.72f, 0.72f, 0.78f), "SWORD_BLADE" });

        m_baseBoxes.push_back({ glm::vec3(-0.14f, 0.45f, 0.0f), glm::vec3(0.18f, 0.85f, 0.18f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3(0.14f, 0.45f, 0.0f), glm::vec3(0.18f, 0.85f, 0.18f), glm::vec3(0.0f), glm::vec3(0.20f, 0.20f, 0.24f), "LEG_R" });
    }
    else if (m_type == EnemyType::BERSERKER_WARRIOR) {
        // 2. Guerrero Berserker Bárbaro (Doble Hacha y Cuernos)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.68f, 0.0f), glm::vec3(0.28f, 0.28f, 0.28f), glm::vec3(0.0f), glm::vec3(0.72f, 0.52f, 0.38f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.70f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.15f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.70f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.15f, 0.05f), "EYE_R" });
        m_baseBoxes.push_back({ glm::vec3(-0.20f, 1.88f, -0.05f), glm::vec3(0.08f, 0.32f, 0.08f), glm::vec3(0.0f, 0.0f, 0.35f), glm::vec3(0.9f, 0.85f, 0.65f), "HELM_HORN_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.20f, 1.88f, -0.05f), glm::vec3(0.08f, 0.32f, 0.08f), glm::vec3(0.0f, 0.0f, -0.35f), glm::vec3(0.9f, 0.85f, 0.65f), "HELM_HORN_R" });

        // Torso musculoso con tatuaje de sangre
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.20f, 0.0f), glm::vec3(0.56f, 0.65f, 0.34f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.25f, 0.18f), glm::vec3(0.42f, 0.35f, 0.04f), glm::vec3(0.0f), glm::vec3(0.85f, 0.08f, 0.08f), "TATTOO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.85f, 0.0f), glm::vec3(0.52f, 0.18f, 0.32f), glm::vec3(0.0f), glm::vec3(0.42f, 0.28f, 0.15f), "BELT_FUR" });

        // Brazo Izquierdo con Gran Hacha
        m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.20f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.38f, 0.95f, 0.12f), glm::vec3(0.06f, 0.65f, 0.06f), glm::vec3(0.0f), glm::vec3(0.35f, 0.22f, 0.12f), "AXE_L_HILT" });
        m_baseBoxes.push_back({ glm::vec3(-0.48f, 1.25f, 0.12f), glm::vec3(0.24f, 0.38f, 0.04f), glm::vec3(0.0f), glm::vec3(0.65f, 0.65f, 0.70f), "AXE_L_BLADE" });

        // Brazo Derecho con Gran Hacha
        m_baseBoxes.push_back({ glm::vec3(0.36f, 1.20f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.68f, 0.48f, 0.34f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.38f, 0.95f, 0.12f), glm::vec3(0.06f, 0.65f, 0.06f), glm::vec3(0.0f), glm::vec3(0.35f, 0.22f, 0.12f), "AXE_R_HILT" });
        m_baseBoxes.push_back({ glm::vec3(0.48f, 1.25f, 0.12f), glm::vec3(0.24f, 0.38f, 0.04f), glm::vec3(0.0f), glm::vec3(0.65f, 0.65f, 0.70f), "AXE_R_BLADE" });

        // Piernas
        m_baseBoxes.push_back({ glm::vec3(-0.16f, 0.45f, 0.0f), glm::vec3(0.20f, 0.85f, 0.20f), glm::vec3(0.0f), glm::vec3(0.38f, 0.26f, 0.15f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.16f, 0.45f, 0.0f), glm::vec3(0.20f, 0.85f, 0.20f), glm::vec3(0.0f), glm::vec3(0.38f, 0.26f, 0.15f), "LEG_R" });
    }
    else if (m_type == EnemyType::DEATH_KNIGHT) {
        // 3. Caballero de la Muerte (Gran Mandoble a dos manos y Acero Negro)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.72f, 0.0f), glm::vec3(0.32f, 0.34f, 0.32f), glm::vec3(0.0f), glm::vec3(0.12f, 0.12f, 0.15f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.07f, 1.74f, 0.17f), glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.15f, 0.85f, 1.0f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.07f, 1.74f, 0.17f), glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.15f, 0.85f, 1.0f), "EYE_R" });

        // Armadura de placas góticas
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.22f, 0.0f), glm::vec3(0.60f, 0.70f, 0.36f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(-0.36f, 1.48f, 0.0f), glm::vec3(0.24f, 0.20f, 0.24f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.28f), "PAULDRON_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.36f, 1.48f, 0.0f), glm::vec3(0.24f, 0.20f, 0.24f), glm::vec3(0.0f), glm::vec3(0.22f, 0.22f, 0.28f), "PAULDRON_R" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, -0.18f), glm::vec3(0.55f, 0.95f, 0.06f), glm::vec3(0.0f), glm::vec3(0.08f, 0.12f, 0.24f), "CAPE" });

        // Brazos y Gran Mandoble Claymore
        m_baseBoxes.push_back({ glm::vec3(-0.34f, 1.18f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.16f, 0.16f, 0.20f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.34f, 1.18f, 0.0f), glm::vec3(0.18f, 0.52f, 0.18f), glm::vec3(0.0f), glm::vec3(0.16f, 0.16f, 0.20f), "ARM_R" });

        m_baseBoxes.push_back({ glm::vec3(0.22f, 0.92f, 0.22f), glm::vec3(0.08f, 0.32f, 0.08f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "CLAYMORE_HILT" });
        m_baseBoxes.push_back({ glm::vec3(0.22f, 1.08f, 0.22f), glm::vec3(0.08f, 0.06f, 0.38f), glm::vec3(0.0f), glm::vec3(0.28f, 0.32f, 0.40f), "CLAYMORE_GUARD" });
        m_baseBoxes.push_back({ glm::vec3(0.22f, 1.70f, 0.22f), glm::vec3(0.06f, 1.20f, 0.22f), glm::vec3(0.0f), glm::vec3(0.60f, 0.75f, 0.90f), "CLAYMORE_BLADE" });

        // Piernas de acero
        m_baseBoxes.push_back({ glm::vec3(-0.16f, 0.45f, 0.0f), glm::vec3(0.22f, 0.88f, 0.22f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.16f, 0.45f, 0.0f), glm::vec3(0.22f, 0.88f, 0.22f), glm::vec3(0.0f), glm::vec3(0.14f, 0.14f, 0.18f), "LEG_R" });
    }
    else if (m_type == EnemyType::SHADOW_ASSASSIN) {
        // 4. Asesino de las Sombras (Dagas Venenosas Gemelas)
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.62f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.74f, -0.04f), glm::vec3(0.28f, 0.14f, 0.28f), glm::vec3(0.0f), glm::vec3(0.08f, 0.08f, 0.10f), "HOOD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.64f, 0.13f), glm::vec3(0.04f, 0.02f, 0.02f), glm::vec3(0.0f), glm::vec3(0.25f, 1.0f, 0.25f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.64f, 0.13f), glm::vec3(0.04f, 0.02f, 0.02f), glm::vec3(0.0f), glm::vec3(0.25f, 1.0f, 0.25f), "EYE_R" });

        // Torso ajustado de cuero
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.42f, 0.58f, 0.24f), glm::vec3(0.0f), glm::vec3(0.12f, 0.12f, 0.14f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.88f, 0.0f), glm::vec3(0.44f, 0.10f, 0.26f), glm::vec3(0.0f), glm::vec3(0.25f, 0.18f, 0.12f), "BELT" });

        // Dagas Venenosas
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.12f, 0.0f), glm::vec3(0.12f, 0.46f, 0.12f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 0.90f, 0.15f), glm::vec3(0.04f, 0.42f, 0.08f), glm::vec3(0.0f), glm::vec3(0.20f, 0.95f, 0.35f), "DAGGER_L" });

        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.12f, 0.0f), glm::vec3(0.12f, 0.46f, 0.12f), glm::vec3(0.0f), glm::vec3(0.10f, 0.10f, 0.12f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 0.90f, 0.15f), glm::vec3(0.04f, 0.42f, 0.08f), glm::vec3(0.0f), glm::vec3(0.20f, 0.95f, 0.35f), "DAGGER_R" });

        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.82f, 0.14f), glm::vec3(0.0f), glm::vec3(0.08f, 0.08f, 0.10f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.82f, 0.14f), glm::vec3(0.0f), glm::vec3(0.08f, 0.08f, 0.10f), "LEG_R" });
    }
    else if (m_type == EnemyType::SKELETON_ARCHER) {
        // 5. Arquero Esqueleto (Arco Largo y Flechas)
        glm::vec3 boneCol = glm::vec3(0.85f, 0.82f, 0.72f);
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), glm::vec3(0.0f), boneCol, "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.66f, 0.13f), glm::vec3(0.03f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.2f, 1.0f, 0.4f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.66f, 0.13f), glm::vec3(0.03f, 0.03f, 0.02f), glm::vec3(0.0f), glm::vec3(0.2f, 1.0f, 0.4f), "EYE_R" });

        // Costillas y Carcaj
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.38f, 0.55f, 0.20f), glm::vec3(0.0f), boneCol, "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.25f, -0.15f), glm::vec3(0.16f, 0.65f, 0.14f), glm::vec3(-0.25f, 0.0f, 0.0f), glm::vec3(0.42f, 0.28f, 0.14f), "QUIVER" });

        // Brazo Izquierdo sosteniendo el Arco
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.15f, 0.0f), glm::vec3(0.10f, 0.48f, 0.10f), glm::vec3(0.0f), boneCol, "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.32f, 1.15f, 0.35f), glm::vec3(0.04f, 1.10f, 0.06f), glm::vec3(0.0f), glm::vec3(0.45f, 0.30f, 0.15f), "BOW_BODY" });
        m_baseBoxes.push_back({ glm::vec3(-0.32f, 1.15f, 0.18f), glm::vec3(0.02f, 1.05f, 0.02f), glm::vec3(0.0f), glm::vec3(0.85f, 0.85f, 0.85f), "BOW_STRING" });

        // Brazo Derecho tensando la cuerda y Flecha
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.15f, 0.0f), glm::vec3(0.10f, 0.48f, 0.10f), glm::vec3(0.0f), boneCol, "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3( 0.0f, 1.20f, 0.25f), glm::vec3(0.03f, 0.03f, 0.85f), glm::vec3(0.0f), glm::vec3(0.92f, 0.82f, 0.55f), "ARROW_NOCKED" });

        // Piernas de hueso
        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.12f, 0.85f, 0.12f), glm::vec3(0.0f), boneCol, "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.12f, 0.85f, 0.12f), glm::vec3(0.0f), boneCol, "LEG_R" });
    }
    else if (m_type == EnemyType::NEUTRAL_GIANT) {
        // Gigante Ancestral
        m_baseBoxes.push_back({ glm::vec3(0.0f, 2.10f, 0.0f), glm::vec3(0.38f, 0.38f, 0.38f), glm::vec3(0.0f), glm::vec3(0.38f, 0.36f, 0.32f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.10f, 2.12f, 0.19f), glm::vec3(0.06f, 0.05f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.10f, 2.12f, 0.19f), glm::vec3(0.06f, 0.05f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.45f, 0.0f), glm::vec3(0.78f, 0.95f, 0.48f), glm::vec3(0.0f), glm::vec3(0.32f, 0.30f, 0.26f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.15f, 1.60f, 0.25f), glm::vec3(0.30f, 0.35f, 0.08f), glm::vec3(0.0f), glm::vec3(0.20f, 0.38f, 0.15f), "MOSS" });

        m_baseBoxes.push_back({ glm::vec3(-0.52f, 1.35f, 0.0f), glm::vec3(0.26f, 0.88f, 0.26f), glm::vec3(0.0f), glm::vec3(0.34f, 0.32f, 0.28f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.52f, 1.35f, 0.0f), glm::vec3(0.26f, 0.88f, 0.26f), glm::vec3(0.0f), glm::vec3(0.34f, 0.32f, 0.28f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.58f, 1.45f, 0.35f), glm::vec3(0.22f, 1.50f, 0.22f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "CLUB" });

        m_baseBoxes.push_back({ glm::vec3(-0.24f, 0.55f, 0.0f), glm::vec3(0.30f, 1.10f, 0.30f), glm::vec3(0.0f), glm::vec3(0.30f, 0.28f, 0.24f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.24f, 0.55f, 0.0f), glm::vec3(0.30f, 1.10f, 0.30f), glm::vec3(0.0f), glm::vec3(0.30f, 0.28f, 0.24f), "LEG_R" });
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
