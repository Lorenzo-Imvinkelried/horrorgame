#include "MobManager.h"
#include "Player.h"
#include "ChunkManager.h"
#include "ScentSystem.h"
#include "WindSystem.h"
#include "ParticleSystem.h"
#include "combat/DamageNumberSystem.h"
#include "world/ItemDropSystem.h"
#include "ProjectileSystem.h"
#include "WorldGenerator.h"
#include "Config.h"
#include "ui/UIRenderer.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

MobManager::MobManager() {}
MobManager::~MobManager() {}

void MobManager::Init(glm::vec3 playerPos, int monsterCount) {
    // 1. Passive & Hostile Forest Deer (Fawns, Adults, Alphas, Demonic)
    m_passiveMobs.clear();
    for (int i = 0; i < 8; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = 20.0f + (rand() % 50);
        float dx = playerPos.x + cos(angle) * dist;
        float dz = playerPos.z + sin(angle) * dist;
        float dy = WorldGenerator::GetHeight(dx, dz);
        DeerSize size = (i == 0) ? DeerSize::DEMONIC : ((i == 1 || i == 5) ? DeerSize::ALPHA : ((i % 2 == 0) ? DeerSize::FAWN : DeerSize::ADULT));
        m_passiveMobs.push_back(std::make_unique<PassiveMob>(glm::vec3(dx, dy, dz), size));
    }

    // 2. Enemy Mobs (Berserkers, Death Knights, Shadow Assassins, Skeleton Archers, Dark Mages, Vampires, Treants, Giants)
    m_enemyMobs.clear();
    EnemyType initialTypes[] = {
        EnemyType::BERSERKER_WARRIOR,
        EnemyType::DEATH_KNIGHT,
        EnemyType::SHADOW_ASSASSIN,
        EnemyType::SKELETON_ARCHER,
        EnemyType::CORRUPTED_WARRIOR,
        EnemyType::DARK_MAGE,
        EnemyType::VAMPIRE,
        EnemyType::TREANT,
        EnemyType::NEUTRAL_GIANT
    };
    for (int i = 0; i < 9; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = 45.0f + (rand() % 50);
        float ex = playerPos.x + cos(angle) * dist;
        float ez = playerPos.z + sin(angle) * dist;
        float ey = WorldGenerator::GetHeight(ex, ez);
        if (ey > 1.5f) {
            m_enemyMobs.push_back(std::make_unique<EnemyMob>(glm::vec3(ex, ey, ez), initialTypes[i % 9], 1));
        }
    }

    // 3. Lake Water Monsters (Water Lurkers)
    m_waterMonsters.clear();
    for (int attempts = 0; attempts < 100 && m_waterMonsters.size() < 4; ++attempts) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = 20.0f + (rand() % 90);
        float wx = playerPos.x + cos(angle) * dist;
        float wz = playerPos.z + sin(angle) * dist;
        float wy = WorldGenerator::GetHeight(wx, wz);
        if (wy < Config::Water::Level) {
            m_waterMonsters.push_back(std::make_unique<WaterMonster>(glm::vec3(wx, Config::Water::Level - 0.6f, wz)));
        }
    }

    // 4. Night Shadow Lurkers (Monsters)
    m_monsters.clear();
    SpawnNightMonsters(playerPos, monsterCount);
}

void MobManager::SpawnNightMonsters(glm::vec3 playerPos, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = Config::Gameplay::MonsterSpawnMinRadius + (rand() % (int)(Config::Gameplay::MonsterSpawnMaxRadius - Config::Gameplay::MonsterSpawnMinRadius));
        float mx = playerPos.x + cos(angle) * dist;
        float mz = playerPos.z + sin(angle) * dist;
        float my = WorldGenerator::GetHeight(mx, mz);
        m_monsters.push_back(std::make_unique<Monster>(glm::vec3(mx, my + 1.0f, mz)));
    }
}

void MobManager::DespawnNightMonsters() {
    m_monsters.clear();
}

void MobManager::Update(float deltaTime, Player& player, ChunkManager& chunkManager, ScentSystem& scentSystem,
                        WindSystem& windSystem, ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                        ItemDropSystem& itemDropSystem, ProjectileSystem& projectiles, float globalTime,
                        float dayCycleTime, int nightCount, bool isBloodMoon, FatalErrorPopup* fatalError)
{
    bool isNight = (dayCycleTime >= 120.0f && dayCycleTime <= 228.0f);

    // Spawning / despawning shadow monsters across night transitions
    if (isNight && !m_wasNight) {
        int count = std::min(1 + (nightCount / 2), 4);
        if (isBloodMoon) count += 2;
        SpawnNightMonsters(player.Position, count);
    } else if (!isNight && m_wasNight) {
        DespawnNightMonsters();
    }
    m_wasNight = isNight;

    // 1. Birds & Critters
    m_birds.Update(deltaTime, player.Position, m_monsters);
    m_birds.CleanupDistantBirds(player.Position, 80.0f);
    m_critters.Update(deltaTime, player.Position);

    // 2. Passive Mobs (Deer)
    for (auto& deer : m_passiveMobs) {
        if (!deer->IsAlive()) continue;
        deer->Update(deltaTime, player.Position, &player, particles, damageNumbers);
    }

    // 3. Water Lurkers
    for (auto& wm : m_waterMonsters) {
        wm->Update(deltaTime, player.Position, &player, particles, damageNumbers);
    }

    // 4. Ancestral Dragon Boss
    m_dragon.Update(deltaTime, player.Position, particles, damageNumbers, &player);

    // 5. Enemy Mobs AI & Combat
    for (auto& enemy : m_enemyMobs) {
        if (!enemy->IsAlive()) continue;
        enemy->Update(deltaTime, player.Position, &player, particles, damageNumbers, projectiles);
    }

    // 6. Shadow Monsters AI & Tracking
    glm::vec2 windDir = windSystem.GetDirection();
    m_highestDangerLevel = 0.0f;

    for (auto& m : m_monsters) {
        if (m->IsDead()) continue;
        m->Update(deltaTime, player.Position, player.Front, windDir,
                  chunkManager, scentSystem, particles,
                  player.Velocity, 0, false,
                  player.IsClimbing, player.ClimbingTreePos,
                  player.HasTorchActive);

        float dist = glm::distance(player.Position, m->GetPosition());
        if (dist < 24.0f) {
            float d = 1.0f - (dist / 24.0f);
            if (d > m_highestDangerLevel) m_highestDangerLevel = d;
        }
    }

    for (auto& e : m_enemyMobs) {
        if (e->IsAlive()) {
            float dist = glm::distance(player.Position, e->GetPosition());
            if (dist < 24.0f) {
                float d = 1.0f - (dist / 24.0f);
                if (d > m_highestDangerLevel) m_highestDangerLevel = d;
            }
        }
    }
}

void MobManager::Render(GLuint shaderProgram, glm::vec3 activeCamPos, GLuint textureID, float globalTime) {
    // 1. Birds & Critters
    m_birds.Render(shaderProgram);
    m_critters.Render(shaderProgram);

    // 2. Passive Forest Animals
    glBindTexture(GL_TEXTURE_2D, textureID);
    for (auto& deer : m_passiveMobs) {
        if (!deer->IsAlive()) continue;
        deer->Render(shaderProgram);
    }

    // 3. Shadow Monsters
    for (auto& m : m_monsters) {
        if (!m->IsDead()) {
            m->Render(shaderProgram, textureID);
        }
    }

    // 4. Enemy Mobs
    for (auto& enemy : m_enemyMobs) {
        if (!enemy->IsAlive()) continue;
        glBindTexture(GL_TEXTURE_2D, textureID);
        enemy->Render(shaderProgram);
        enemy->RenderHealthBar(shaderProgram, activeCamPos);
    }

    // 5. Water Monsters
    for (auto& wm : m_waterMonsters) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        wm->Render(shaderProgram);
        wm->RenderHealthBar(shaderProgram, activeCamPos);
    }

    // 6. Dragon Boss
    glBindTexture(GL_TEXTURE_2D, textureID);
    m_dragon.Render(shaderProgram);
    m_dragon.RenderHealthBar(shaderProgram, activeCamPos);
}

void MobManager::RenderDebug(GLuint shaderProgram) {
    for (auto& m : m_monsters) {
        m->RenderDebug(shaderProgram);
    }
}
