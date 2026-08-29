#include "EnemyMob.h"
#include "WorldGenerator.h"
#include "Player.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

GLuint EnemyMob::s_hpBarVAO = 0;
GLuint EnemyMob::s_hpBarVBO = 0;

void EnemyMob::initHpBarMesh() {
    if (s_hpBarVAO != 0) return;

    float quadVertices[] = {
        -0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.06f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &s_hpBarVAO);
    glGenBuffers(1, &s_hpBarVBO);
    glBindVertexArray(s_hpBarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_hpBarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

EnemyMob::EnemyMob(glm::vec3 spawnPos, EnemyType type, int nightLevel)
    : m_type(type)
    , m_state(EnemyState::IDLE)
    , m_scale(1.0f)
    , m_pos(spawnPos)
    , m_spawnOrigin(spawnPos)
    , m_targetPos(spawnPos)
    , m_yaw((float)(rand() % 360) * 0.01745f)
    , m_speed(0.0f)
    , m_animTimer((float)(rand() % 100) * 0.1f)
    , m_stateTimer(2.0f + (rand() % 100) * 0.02f)
    , m_attackCooldown(0.0f)
    , m_attackAnimProgress(0.0f)
    , m_isEnraged(false)
    , m_isAwakened(false)
    , m_nightLevel(nightLevel)
    , m_maxHp(100)
    , m_currentHp(100)
    , m_defense(5)
    , m_evasion(10)
    , m_hitFlashTimer(0.0f)
    , m_showHpBarTimer(0.0f)
    , m_deathTimer(0.0f)
    , m_eyePulse(0.0f)
    , m_VAO(0)
    , m_VBO(0)
    , m_vertexCount(0)
{
    float nightScale = 1.0f + (m_nightLevel - 1) * 0.25f;

    if (m_type == EnemyType::CORRUPTED_WARRIOR) {
        m_scale = 1.05f;
        m_maxHp = (int)(140 * nightScale);
        m_defense = 8 + (m_nightLevel - 1) * 2;
        m_evasion = 8;
    } else if (m_type == EnemyType::NEUTRAL_GIANT) {
        m_scale = 2.75f;
        m_maxHp = (int)(480 * nightScale);
        m_defense = 14 + (m_nightLevel - 1) * 2;
        m_evasion = 3;
    } else if (m_type == EnemyType::DARK_MAGE) {
        m_scale = 1.0f;
        m_maxHp = (int)(85 * nightScale);
        m_defense = 3;
        m_evasion = 12;
    } else if (m_type == EnemyType::TREANT) {
        m_scale = 1.95f;
        m_maxHp = (int)(380 * nightScale);
        m_defense = 15 + (m_nightLevel - 1) * 2;
        m_evasion = 2;
    } else if (m_type == EnemyType::VAMPIRE) {
        m_scale = 1.10f;
        m_maxHp = (int)(175 * nightScale);
        m_defense = 6 + (m_nightLevel - 1) * 2;
        m_evasion = 18;
    }
    m_currentHp = m_maxHp;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    initHpBarMesh();
    initMeshes();
    updateModelMesh();
}

EnemyMob::~EnemyMob() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

std::string EnemyMob::GetName() const {
    if (m_type == EnemyType::CORRUPTED_WARRIOR) return "Guerrero Caido";
    if (m_type == EnemyType::NEUTRAL_GIANT) return "Gigante Ancestral";
    if (m_type == EnemyType::DARK_MAGE) return "Mago Sombrio";
    if (m_type == EnemyType::TREANT) return "Arbol Viviente";
    if (m_type == EnemyType::VAMPIRE) return "Vampiro Sanguinario";
    return "Enemigo";
}

int EnemyMob::GetLevel() const {
    int base = 3;
    if (m_type == EnemyType::CORRUPTED_WARRIOR) base = 4;
    else if (m_type == EnemyType::NEUTRAL_GIANT) base = 7;
    else if (m_type == EnemyType::DARK_MAGE) base = 3;
    else if (m_type == EnemyType::TREANT) base = 8;
    else if (m_type == EnemyType::VAMPIRE) base = 6;
    return base + (m_nightLevel - 1) * 2;
}

int EnemyMob::GetExpReward() const {
    float nightScale = 1.0f + (m_nightLevel - 1) * 0.25f;
    int base = 80;
    if (m_type == EnemyType::CORRUPTED_WARRIOR) base = 110;
    else if (m_type == EnemyType::NEUTRAL_GIANT) base = 320;
    else if (m_type == EnemyType::DARK_MAGE) base = 100;
    else if (m_type == EnemyType::TREANT) base = 280;
    else if (m_type == EnemyType::VAMPIRE) base = 190;
    return (int)(base * nightScale);
}

float EnemyMob::GetRadius() const {
    if (m_type == EnemyType::NEUTRAL_GIANT) return 2.2f;
    if (m_type == EnemyType::TREANT) return 1.8f;
    return 0.85f;
}

void EnemyMob::pickWanderTarget() {
    float wanderRadius = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 35.0f : 20.0f;
    float angle = (float)(rand() % 360) * 0.01745f;
    float dist = 4.0f + (float)(rand() % (int)wanderRadius);
    m_targetPos.x = m_spawnOrigin.x + cos(angle) * dist;
    m_targetPos.z = m_spawnOrigin.z + sin(angle) * dist;
    m_targetPos.y = WorldGenerator::GetHeight(m_targetPos.x, m_targetPos.z);
}

void EnemyMob::initMeshes() {
    m_baseBoxes.clear();

    if (m_type == EnemyType::CORRUPTED_WARRIOR) {
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
    else if (m_type == EnemyType::NEUTRAL_GIANT) {
        m_baseBoxes.push_back({ glm::vec3(0.0f, 2.10f, 0.0f), glm::vec3(0.38f, 0.38f, 0.38f), glm::vec3(0.0f), glm::vec3(0.38f, 0.36f, 0.32f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.10f, 2.12f, 0.19f), glm::vec3(0.06f, 0.05f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3(0.10f, 2.12f, 0.19f), glm::vec3(0.06f, 0.05f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.75f, 0.20f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.45f, 0.0f), glm::vec3(0.78f, 0.95f, 0.48f), glm::vec3(0.0f), glm::vec3(0.32f, 0.30f, 0.26f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.15f, 1.60f, 0.25f), glm::vec3(0.30f, 0.35f, 0.08f), glm::vec3(0.0f), glm::vec3(0.20f, 0.38f, 0.15f), "MOSS" });

        m_baseBoxes.push_back({ glm::vec3(-0.52f, 1.35f, 0.0f), glm::vec3(0.26f, 0.88f, 0.26f), glm::vec3(0.0f), glm::vec3(0.34f, 0.32f, 0.28f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(0.52f, 1.35f, 0.0f), glm::vec3(0.26f, 0.88f, 0.26f), glm::vec3(0.0f), glm::vec3(0.34f, 0.32f, 0.28f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.58f, 1.45f, 0.35f), glm::vec3(0.22f, 1.50f, 0.22f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "CLUB" });

        m_baseBoxes.push_back({ glm::vec3(-0.24f, 0.55f, 0.0f), glm::vec3(0.30f, 1.10f, 0.30f), glm::vec3(0.0f), glm::vec3(0.30f, 0.28f, 0.24f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3(0.24f, 0.55f, 0.0f), glm::vec3(0.30f, 1.10f, 0.30f), glm::vec3(0.0f), glm::vec3(0.30f, 0.28f, 0.24f), "LEG_R" });
    }
    else if (m_type == EnemyType::DARK_MAGE) {
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.28f, 0.30f, 0.28f), glm::vec3(0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.65f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(0.90f, 0.15f, 0.95f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3(0.06f, 1.65f, 0.14f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(0.90f, 0.15f, 0.95f), "EYE_R" });

        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.05f, 0.0f), glm::vec3(0.44f, 0.85f, 0.32f), glm::vec3(0.0f), glm::vec3(0.16f, 0.10f, 0.22f), "TORSO" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 0.45f, 0.0f), glm::vec3(0.48f, 0.75f, 0.36f), glm::vec3(0.0f), glm::vec3(0.14f, 0.08f, 0.20f), "ROBE_SKIRT" });

        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.15f, 0.08f), glm::vec3(0.14f, 0.45f, 0.14f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(0.28f, 1.15f, 0.0f), glm::vec3(0.14f, 0.45f, 0.14f), glm::vec3(0.0f), glm::vec3(0.18f, 0.12f, 0.24f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 1.15f, 0.15f), glm::vec3(0.04f, 1.50f, 0.04f), glm::vec3(0.0f), glm::vec3(0.35f, 0.22f, 0.12f), "STAFF" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 1.95f, 0.15f), glm::vec3(0.12f, 0.16f, 0.12f), glm::vec3(0.0f), glm::vec3(0.45f, 0.30f, 0.15f), "STAFF_TOP" });
        m_baseBoxes.push_back({ glm::vec3(0.32f, 2.08f, 0.15f), glm::vec3(0.14f, 0.14f, 0.14f), glm::vec3(0.0f), glm::vec3(1.0f, 0.2f, 0.9f), "ORB" });

        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.35f, 0.0f), glm::vec3(0.14f, 0.70f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.14f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3(0.12f, 0.35f, 0.0f), glm::vec3(0.14f, 0.70f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.14f), "LEG_R" });
    }
    else if (m_type == EnemyType::TREANT) {
        // Living Ancient Tree
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.50f, 0.0f), glm::vec3(0.85f, 2.80f, 0.85f), glm::vec3(0.0f), glm::vec3(0.32f, 0.22f, 0.12f), "TRUNK_BASE" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 3.20f, 0.0f), glm::vec3(2.20f, 1.40f, 2.20f), glm::vec3(0.0f), glm::vec3(0.16f, 0.38f, 0.14f), "LEAVES_LOWER" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 4.20f, 0.0f), glm::vec3(1.60f, 1.20f, 1.60f), glm::vec3(0.0f), glm::vec3(0.20f, 0.44f, 0.18f), "LEAVES_UPPER" });

        // Glowing Tree Eyes and Mouth
        m_baseBoxes.push_back({ glm::vec3(-0.22f, 2.10f, 0.44f), glm::vec3(0.10f, 0.08f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.65f, 0.10f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.22f, 2.10f, 0.44f), glm::vec3(0.10f, 0.08f, 0.04f), glm::vec3(0.0f), glm::vec3(0.95f, 0.65f, 0.10f), "EYE_R" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.44f), glm::vec3(0.32f, 0.10f, 0.04f), glm::vec3(0.0f), glm::vec3(0.12f, 0.06f, 0.04f), "MOUTH" });

        // Heavy Branch Arms
        m_baseBoxes.push_back({ glm::vec3(-0.65f, 2.00f, 0.10f), glm::vec3(0.30f, 1.60f, 0.30f), glm::vec3(0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.65f, 2.00f, 0.10f), glm::vec3(0.30f, 1.60f, 0.30f), glm::vec3(0.0f), glm::vec3(0.28f, 0.18f, 0.10f), "ARM_R" });

        // Root Legs
        m_baseBoxes.push_back({ glm::vec3(-0.35f, 0.40f, 0.0f), glm::vec3(0.35f, 0.80f, 0.35f), glm::vec3(0.0f), glm::vec3(0.26f, 0.16f, 0.08f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.35f, 0.40f, 0.0f), glm::vec3(0.35f, 0.80f, 0.35f), glm::vec3(0.0f), glm::vec3(0.26f, 0.16f, 0.08f), "LEG_R" });
    }
    else if (m_type == EnemyType::VAMPIRE) {
        // Blood Vampire
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.65f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), glm::vec3(0.0f), glm::vec3(0.85f, 0.85f, 0.88f), "HEAD" });
        m_baseBoxes.push_back({ glm::vec3(-0.06f, 1.66f, 0.13f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.06f, 1.66f, 0.13f), glm::vec3(0.04f, 0.03f, 0.03f), glm::vec3(0.0f), glm::vec3(1.0f, 0.05f, 0.05f), "EYE_R" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.58f, 0.13f), glm::vec3(0.08f, 0.04f, 0.02f), glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f), "FANGS" });

        // Velvet Crimson & Black Cloak
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.10f, -0.10f), glm::vec3(0.55f, 0.95f, 0.08f), glm::vec3(0.0f), glm::vec3(0.10f, 0.06f, 0.12f), "CAPE_BACK" });
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.10f, -0.05f), glm::vec3(0.50f, 0.90f, 0.04f), glm::vec3(0.0f), glm::vec3(0.65f, 0.05f, 0.08f), "CAPE_LINING" });

        // Torso & Noble Garments
        m_baseBoxes.push_back({ glm::vec3(0.0f, 1.15f, 0.0f), glm::vec3(0.38f, 0.55f, 0.24f), glm::vec3(0.0f), glm::vec3(0.15f, 0.12f, 0.18f), "TORSO" });

        // Left & Right Arms with Sharp Claws
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 1.15f, 0.0f), glm::vec3(0.12f, 0.48f, 0.12f), glm::vec3(0.0f), glm::vec3(0.12f, 0.08f, 0.14f), "ARM_L" });
        m_baseBoxes.push_back({ glm::vec3(-0.28f, 0.88f, 0.08f), glm::vec3(0.08f, 0.14f, 0.08f), glm::vec3(0.0f), glm::vec3(0.80f, 0.10f, 0.10f), "CLAW_L" });

        m_baseBoxes.push_back({ glm::vec3( 0.28f, 1.15f, 0.0f), glm::vec3(0.12f, 0.48f, 0.12f), glm::vec3(0.0f), glm::vec3(0.12f, 0.08f, 0.14f), "ARM_R" });
        m_baseBoxes.push_back({ glm::vec3( 0.28f, 0.88f, 0.08f), glm::vec3(0.08f, 0.14f, 0.08f), glm::vec3(0.0f), glm::vec3(0.80f, 0.10f, 0.10f), "CLAW_R" });

        // Legs
        m_baseBoxes.push_back({ glm::vec3(-0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.85f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.12f), "LEG_L" });
        m_baseBoxes.push_back({ glm::vec3( 0.12f, 0.45f, 0.0f), glm::vec3(0.14f, 0.85f, 0.14f), glm::vec3(0.0f), glm::vec3(0.10f, 0.08f, 0.12f), "LEG_R" });
    }
}

void EnemyMob::Update(float deltaTime, glm::vec3 playerPos, Player* player, ParticleSystem& particles, DamageNumberSystem& damageNumbers, ProjectileSystem& projectiles) {
    if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
    if (m_showHpBarTimer > 0.0f) m_showHpBarTimer -= deltaTime;
    if (m_attackCooldown > 0.0f) m_attackCooldown -= deltaTime;
    m_eyePulse += deltaTime * 5.0f;

    if (m_state == EnemyState::DEAD) {
        m_deathTimer += deltaTime;
        updateModelMesh();
        return;
    }

    float distToPlayer = glm::distance(glm::vec2(m_pos.x, m_pos.z), glm::vec2(playerPos.x, playerPos.z));

    // =========================================================================
    // 1. CORRUPTED WARRIOR & VAMPIRE & TREANT CHASE LOGIC
    // =========================================================================
    if (m_type == EnemyType::CORRUPTED_WARRIOR || m_type == EnemyType::VAMPIRE) {
        if (distToPlayer < 28.0f && m_state != EnemyState::CHASE) {
            m_state = EnemyState::CHASE;
        }

        if (m_state == EnemyState::CHASE && distToPlayer > 35.0f) {
            m_state = EnemyState::IDLE;
            m_stateTimer = 3.0f;
            m_speed = 0.0f;
        }

        if (m_state == EnemyState::CHASE) {
            glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
            float d2D = glm::length(toP);
            if (d2D > 0.001f) toP /= d2D;

            float targetYaw = atan2(toP.x, toP.y);
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 6.0f * deltaTime;

            m_speed = (m_type == EnemyType::VAMPIRE) ? 7.6f : 5.4f;

            if (d2D > 2.0f) {
                m_pos.x += toP.x * m_speed * deltaTime;
                m_pos.z += toP.y * m_speed * deltaTime;
                m_animTimer += deltaTime * ((m_type == EnemyType::VAMPIRE) ? 10.0f : 8.0f);
            } else {
                m_speed = 0.0f;
                // Attack Player
                if (m_attackCooldown <= 0.0f && player != nullptr) {
                    m_attackCooldown = (m_type == EnemyType::VAMPIRE) ? 1.4f : 1.6f;
                    int baseDmg = (m_type == EnemyType::VAMPIRE) ? (24 + rand() % 8) : (20 + rand() % 6);
                    int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));

                    player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                    // Vampire Lifesteal Drain Mechanic!
                    if (m_type == EnemyType::VAMPIRE) {
                        m_currentHp = std::min(m_maxHp, m_currentHp + scaledDmg);
                        damageNumbers.SpawnDamage(m_pos + glm::vec3(0, 2.0f, 0), scaledDmg, true); // Green heal text

                        // Blood Siphon Particle Stream from Player to Vampire
                        for (int i = 0; i < 24; ++i) {
                            float t = (float)i / 24.0f;
                            glm::vec3 sPos = glm::mix(playerPos + glm::vec3(0, 1.2f, 0), m_pos + glm::vec3(0, 1.4f, 0), t);
                            particles.SpawnParticle(sPos, glm::vec3(0, 0.4f, 0), glm::vec4(0.85f, 0.05f, 0.05f, 0.9f), 0.12f, 0.5f, 0.0f);
                        }
                    }

                    // Slash particles
                    glm::vec3 hitPos = playerPos + glm::vec3(0.0f, 1.2f, 0.0f);
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.2f, (rand()%100/50.0f - 1.0f)*3.0f);
                        particles.SpawnParticle(hitPos, pVel, glm::vec4(0.85f, 0.05f, 0.05f, 1.0f), 0.14f, 0.8f, -9.8f);
                    }
                }
            }

            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            updateModelMesh();
            return;
        }
    }

    // =========================================================================
    // 2. LIVING TREANT LOGIC (Camouflaged until player gets near or attacks)
    // =========================================================================
    if (m_type == EnemyType::TREANT) {
        if (!m_isAwakened) {
            // Dormant Tree State
            if (distToPlayer < 7.5f) {
                m_isAwakened = true;
                // Awakening ground shake particles
                for (int i = 0; i < 28; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.4f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
                    particles.SpawnParticle(m_pos + glm::vec3(0, 0.4f, 0), pVel, glm::vec4(0.38f, 0.28f, 0.12f, 1.0f), 0.18f, 0.8f, -9.8f);
                }
            }
            m_speed = 0.0f;
            updateModelMesh();
            return;
        } else {
            // Awakened Treant Pursuit
            glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
            float d2D = glm::length(toP);
            if (d2D > 0.001f) toP /= d2D;

            float targetYaw = atan2(toP.x, toP.y);
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 2.5f * deltaTime;

            m_speed = 2.4f;

            if (d2D > 2.8f) {
                m_pos.x += toP.x * m_speed * deltaTime;
                m_pos.z += toP.y * m_speed * deltaTime;
                m_animTimer += deltaTime * 4.0f;
            } else {
                m_speed = 0.0f;
                // Colossal Branch Smash Attack
                if (m_attackCooldown <= 0.0f && player != nullptr) {
                    m_attackCooldown = 2.2f;
                    int baseDmg = 32 + (rand() % 10);
                    int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));
                    player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                    // Wooden shockwave particles
                    glm::vec3 slamPos = m_pos + glm::vec3(toP.x * 1.5f, 0.2f, toP.y * 1.5f);
                    for (int i = 0; i < 30; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.4f)*3.8f, (rand()%100/50.0f - 1.0f)*3.5f);
                        particles.SpawnParticle(slamPos, pVel, glm::vec4(0.42f, 0.30f, 0.15f, 1.0f), 0.20f, 0.9f, -9.8f);
                    }
                }
            }

            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            updateModelMesh();
            return;
        }
    }

    // =========================================================================
    // 3. NEUTRAL GIANT LOGIC
    // =========================================================================
    if (m_type == EnemyType::NEUTRAL_GIANT) {
        if (m_isEnraged) {
            glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
            float d2D = glm::length(toP);
            if (d2D > 0.001f) toP /= d2D;

            float targetYaw = atan2(toP.x, toP.y);
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 3.0f * deltaTime;

            m_speed = 3.8f;

            if (d2D > 3.4f) {
                m_pos.x += toP.x * m_speed * deltaTime;
                m_pos.z += toP.y * m_speed * deltaTime;
                m_animTimer += deltaTime * 5.0f;
            } else {
                m_speed = 0.0f;
                // Giant Colossal Club Attack
                if (m_attackCooldown <= 0.0f && player != nullptr) {
                    m_attackCooldown = 2.4f;
                    int baseDmg = 35 + (rand() % 12);
                    int scaledDmg = (int)(baseDmg * (1.0f + (m_nightLevel - 1) * 0.25f));
                    player->TakeDamage(scaledDmg, damageNumbers, nullptr, false);

                    // Ground shockwave particles
                    glm::vec3 slamPos = m_pos + glm::vec3(toP.x * 2.0f, 0.2f, toP.y * 2.0f);
                    for (int i = 0; i < 35; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
                        particles.SpawnParticle(slamPos, pVel, glm::vec4(0.55f, 0.45f, 0.30f, 1.0f), 0.22f, 1.0f, -9.8f);
                    }
                }
            }

            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            updateModelMesh();
            return;
        }
    }

    // =========================================================================
    // 4. DARK MAGE (Ranged Kiter)
    // =========================================================================
    if (m_type == EnemyType::DARK_MAGE) {
        if (distToPlayer < 24.0f) {
            glm::vec2 toP = glm::vec2(playerPos.x - m_pos.x, playerPos.z - m_pos.z);
            float d2D = glm::length(toP);
            if (d2D > 0.001f) toP /= d2D;

            float targetYaw = atan2(toP.x, toP.y);
            float diff = targetYaw - m_yaw;
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            m_yaw += diff * 4.0f * deltaTime;

            // Kite away if player is too close (< 8m)
            if (distToPlayer < 8.0f) {
                m_speed = 3.2f;
                m_pos.x -= toP.x * m_speed * deltaTime;
                m_pos.z -= toP.y * m_speed * deltaTime;
                m_animTimer += deltaTime * 6.0f;
            } else {
                m_speed = 0.0f;
            }

            // Cast Magic Fireball at Range!
            if (m_attackCooldown <= 0.0f && distToPlayer <= 25.0f) {
                glm::vec3 staffOrbPos = m_pos + glm::vec3(0.32f, 2.08f, 0.15f);
                glm::vec3 playerChest = playerPos + glm::vec3(0.0f, 1.1f, 0.0f);

                int mageDmg = (int)(16 * (1.0f + (m_nightLevel - 1) * 0.25f));
                projectiles.Spawn(staffOrbPos, playerChest, 12.0f, mageDmg, glm::vec4(0.95f, 0.15f, 0.90f, 1.0f));

                for (int i = 0; i < 14; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f + 0.2f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f);
                    particles.SpawnParticle(staffOrbPos, pVel, glm::vec4(1.0f, 0.4f, 1.0f, 1.0f), 0.14f, 0.6f, 0.0f);
                }

                m_attackCooldown = 2.4f;
            }

            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            updateModelMesh();
            return;
        }
    }

    // =========================================================================
    // Peaceful Wander & Idle State
    // =========================================================================
    switch (m_state) {
        case EnemyState::IDLE: {
            m_speed = 0.0f;
            m_stateTimer -= deltaTime;
            m_animTimer += deltaTime;
            if (m_stateTimer <= 0.0f) {
                pickWanderTarget();
                m_state = EnemyState::WANDER;
            }
            break;
        }

        case EnemyState::WANDER: {
            glm::vec2 toTarget(m_targetPos.x - m_pos.x, m_targetPos.z - m_pos.z);
            float dist = glm::length(toTarget);

            if (dist < 1.5f) {
                m_state = EnemyState::IDLE;
                m_stateTimer = 3.0f + (rand() % 100) * 0.04f;
            } else {
                glm::vec2 moveDir = glm::normalize(toTarget);
                float targetYaw = atan2(moveDir.x, moveDir.y);

                float diff = targetYaw - m_yaw;
                while (diff > 3.14159f) diff -= 6.28318f;
                while (diff < -3.14159f) diff += 6.28318f;
                m_yaw += diff * 3.5f * deltaTime;

                m_speed = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 1.0f : 1.3f;
                m_pos.x += sin(m_yaw) * m_speed * deltaTime;
                m_pos.z += cos(m_yaw) * m_speed * deltaTime;
                m_animTimer += deltaTime * 5.0f;
            }
            break;
        }

        default:
            break;
    }

    m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
    updateModelMesh();
}

bool EnemyMob::TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers) {
    if (m_state == EnemyState::DEAD) return false;

    int effectiveDamage = std::max(1, damage - m_defense);
    m_currentHp -= effectiveDamage;
    m_hitFlashTimer = 0.20f;
    m_showHpBarTimer = 5.0f;

    // Enrage / Awaken when hit
    if (m_type == EnemyType::NEUTRAL_GIANT) {
        m_isEnraged = true;
    } else if (m_type == EnemyType::TREANT) {
        m_isAwakened = true;
    } else if (m_type == EnemyType::CORRUPTED_WARRIOR || m_type == EnemyType::VAMPIRE) {
        m_state = EnemyState::CHASE;
    }

    glm::vec3 hitPos = m_pos + glm::vec3(0.0f, 1.2f * m_scale, 0.0f);
    glm::vec4 bloodCol = (m_type == EnemyType::TREANT) ? glm::vec4(0.28f, 0.50f, 0.18f, 1.0f) : ((m_type == EnemyType::NEUTRAL_GIANT) ? glm::vec4(0.48f, 0.40f, 0.25f, 1.0f) : glm::vec4(0.80f, 0.05f, 0.05f, 1.0f));

    for (int i = 0; i < 18; ++i) {
        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.0f);
        particles.SpawnParticle(hitPos, pVel, bloodCol, 0.14f, 0.8f, -9.8f);
    }

    if (m_currentHp <= 0) {
        m_currentHp = 0;
        m_state = EnemyState::DEAD;
        m_deathTimer = 0.0f;
        for (int i = 0; i < 30; ++i) {
            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*4.0f, (rand()%100/50.0f + 0.5f)*4.5f, (rand()%100/50.0f - 1.0f)*4.0f);
            particles.SpawnParticle(hitPos, pVel, bloodCol, 0.16f, 1.0f, -9.8f);
        }
        return true; // KILLED!
    }

    return false;
}

void EnemyMob::updateModelMesh() {
    if (m_baseBoxes.empty() || m_VAO == 0) return;

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size());

    float legSwing = (m_speed > 0.1f) ? sin(m_animTimer) * 0.45f : 0.0f;
    float armSwing = (m_speed > 0.1f) ? sin(m_animTimer) * 0.35f : 0.0f;

    for (const auto& box : m_baseBoxes) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        glm::vec3 finalColor = box.Color;

        // Glowing red / amber / purple eyes
        if (box.Name == "EYE_L" || box.Name == "EYE_R") {
            float pulse = 1.3f + 0.5f * sin(m_eyePulse);
            if (m_type == EnemyType::NEUTRAL_GIANT && m_isEnraged) {
                finalColor = glm::vec3(1.0f, 0.25f, 0.05f) * pulse; // Enraged fiery red
            } else {
                finalColor *= pulse;
            }
        }

        // Hit Flash Overlay
        if (m_hitFlashTimer > 0.0f) {
            finalColor = glm::mix(finalColor, glm::vec3(1.0f, 0.95f, 0.95f), 0.65f);
        }

        transformedBoxes.push_back({ M, finalColor });
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_vertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
}

void EnemyMob::Render(GLuint shaderProgram) {
    if (m_VAO == 0 || m_vertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_pos);
    model = glm::rotate(model, m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_scale));

    // Death fall animation
    if (m_state == EnemyState::DEAD) {
        float fallAngle = std::min(m_deathTimer * 90.0f, 90.0f);
        model = glm::rotate(model, glm::radians(fallAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}

void EnemyMob::RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos) {
    if (m_state == EnemyState::DEAD || m_showHpBarTimer <= 0.0f || s_hpBarVAO == 0) return;

    float hpPct = std::clamp((float)m_currentHp / (float)m_maxHp, 0.0f, 1.0f);
    float barHeightOffset = (m_type == EnemyType::NEUTRAL_GIANT || m_type == EnemyType::TREANT) ? 4.8f : 2.2f;
    glm::vec3 barPos = m_pos + glm::vec3(0.0f, barHeightOffset, 0.0f);

    glm::vec3 toCam = glm::normalize(cameraPos - barPos);
    float yaw = atan2(toCam.x, toCam.z);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    // 1. Background Bar (Dark Red / Gray)
    glm::mat4 bgModel = glm::mat4(1.0f);
    bgModel = glm::translate(bgModel, barPos);
    bgModel = glm::rotate(bgModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    bgModel = glm::scale(bgModel, glm::vec3(1.2f, 1.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bgModel));

    glBindVertexArray(s_hpBarVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Foreground Health Fill (Bright Red)
    if (hpPct > 0.01f) {
        glm::mat4 fgModel = glm::mat4(1.0f);
        fgModel = glm::translate(fgModel, barPos + toCam * 0.01f);
        fgModel = glm::rotate(fgModel, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        fgModel = glm::translate(fgModel, glm::vec3(-0.6f * (1.0f - hpPct), 0.0f, 0.0f));
        fgModel = glm::scale(fgModel, glm::vec3(1.2f * hpPct, 1.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fgModel));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}
