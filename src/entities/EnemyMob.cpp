#include "EnemyMob.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "world/StructureSystem.h"
#include <algorithm>
#include <cstdlib>

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

    switch (m_type) {
        case EnemyType::CORRUPTED_WARRIOR:
            m_scale = 1.05f;
            m_maxHp = (int)(260 * nightScale);
            m_defense = 14 + (m_nightLevel - 1) * 3;
            m_evasion = 8;
            break;
        case EnemyType::BERSERKER_WARRIOR:
            m_scale = 1.15f;
            m_maxHp = (int)(420 * nightScale);
            m_defense = 12 + (m_nightLevel - 1) * 3;
            m_evasion = 15;
            break;
        case EnemyType::DEATH_KNIGHT:
            m_scale = 1.22f;
            m_maxHp = (int)(580 * nightScale);
            m_defense = 26 + (m_nightLevel - 1) * 4;
            m_evasion = 6;
            break;
        case EnemyType::SHADOW_ASSASSIN:
            m_scale = 0.95f;
            m_maxHp = (int)(220 * nightScale);
            m_defense = 8 + (m_nightLevel - 1) * 2;
            m_evasion = 35; // 35% de esquiva de golpes del jugador
            break;
        case EnemyType::SKELETON_ARCHER:
            m_scale = 1.0f;
            m_maxHp = (int)(180 * nightScale);
            m_defense = 8 + (m_nightLevel - 1) * 2;
            m_evasion = 20;
            break;
        case EnemyType::NEUTRAL_GIANT:
            m_scale = 2.75f;
            m_maxHp = (int)(980 * nightScale);
            m_defense = 24 + (m_nightLevel - 1) * 3;
            m_evasion = 3;
            break;
        case EnemyType::DARK_MAGE:
            m_scale = 1.0f;
            m_maxHp = (int)(170 * nightScale);
            m_defense = 6 + (m_nightLevel - 1) * 2;
            m_evasion = 18;
            break;
        case EnemyType::TREANT:
            m_scale = 1.95f;
            m_maxHp = (int)(750 * nightScale);
            m_defense = 22 + (m_nightLevel - 1) * 3;
            m_evasion = 2;
            break;
        case EnemyType::VAMPIRE:
            m_scale = 1.10f;
            m_maxHp = (int)(340 * nightScale);
            m_defense = 12 + (m_nightLevel - 1) * 3;
            m_evasion = 24;
            break;
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

    updateAI(deltaTime, playerPos, player, particles, damageNumbers, projectiles);

    // Colisión sólida de la estructura: los mobs respetan muros, pilares y almenas
    glm::vec3 dummyVel(0.0f);
    StructureSystem::CheckCollision(m_pos, 0.5f * m_scale, 1.8f * m_scale, dummyVel);

    float terrainY = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
    m_pos.y = StructureSystem::GetWalkableHeight(m_pos.x, m_pos.z, m_pos.y, terrainY);
    updateModelMesh();
}

bool EnemyMob::TakeDamage(int damage, glm::vec3 hitOrigin, ParticleSystem& particles, Player* player, DamageNumberSystem& damageNumbers) {
    if (m_state == EnemyState::DEAD) return false;

    int effectiveDamage = std::max(1, damage - m_defense);
    m_currentHp -= effectiveDamage;
    m_hitFlashTimer = 0.20f;
    m_showHpBarTimer = 5.0f;

    // Enrage / Despertar según tipo
    if (m_type == EnemyType::NEUTRAL_GIANT) {
        m_isEnraged = true;
    } else if (m_type == EnemyType::TREANT) {
        m_isAwakened = true;
    } else {
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
        return true; // Asesinado
    }

    return false;
}

std::string EnemyMob::GetName() const {
    switch (m_type) {
        case EnemyType::CORRUPTED_WARRIOR: return "Guerrero Caido";
        case EnemyType::BERSERKER_WARRIOR: return "Berserker Sangriento";
        case EnemyType::DEATH_KNIGHT:      return "Caballero de la Muerte";
        case EnemyType::SHADOW_ASSASSIN:   return "Asesino de las Sombras";
        case EnemyType::SKELETON_ARCHER:   return "Arquero Esqueleto";
        case EnemyType::NEUTRAL_GIANT:     return "Gigante Ancestral";
        case EnemyType::DARK_MAGE:         return "Mago Sombrio";
        case EnemyType::TREANT:            return "Arbol Viviente";
        case EnemyType::VAMPIRE:           return "Vampiro Sanguinario";
    }
    return "Enemigo";
}

int EnemyMob::GetLevel() const {
    int base = 3;
    switch (m_type) {
        case EnemyType::CORRUPTED_WARRIOR: base = 4; break;
        case EnemyType::BERSERKER_WARRIOR: base = 6; break;
        case EnemyType::DEATH_KNIGHT:      base = 8; break;
        case EnemyType::SHADOW_ASSASSIN:   base = 5; break;
        case EnemyType::SKELETON_ARCHER:   base = 4; break;
        case EnemyType::NEUTRAL_GIANT:     base = 7; break;
        case EnemyType::DARK_MAGE:         base = 3; break;
        case EnemyType::TREANT:            base = 8; break;
        case EnemyType::VAMPIRE:           base = 6; break;
    }
    return base + (m_nightLevel - 1) * 2;
}

int EnemyMob::GetExpReward() const {
    float nightScale = 1.0f + (m_nightLevel - 1) * 0.25f;
    int base = 80;
    switch (m_type) {
        case EnemyType::CORRUPTED_WARRIOR: base = 110; break;
        case EnemyType::BERSERKER_WARRIOR: base = 160; break;
        case EnemyType::DEATH_KNIGHT:      base = 240; break;
        case EnemyType::SHADOW_ASSASSIN:   base = 135; break;
        case EnemyType::SKELETON_ARCHER:   base = 120; break;
        case EnemyType::NEUTRAL_GIANT:     base = 320; break;
        case EnemyType::DARK_MAGE:         base = 100; break;
        case EnemyType::TREANT:            base = 280; break;
        case EnemyType::VAMPIRE:           base = 190; break;
    }
    return (int)(base * nightScale);
}

float EnemyMob::GetRadius() const {
    if (m_type == EnemyType::NEUTRAL_GIANT) return 2.2f;
    if (m_type == EnemyType::TREANT) return 1.8f;
    return 0.85f;
}
