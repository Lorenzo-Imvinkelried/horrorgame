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
#include "combat/TargetingSystem.h"
#include "mobs/dummy/DummyMob.h"
#include "world/StructureSystem.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

MobManager::MobManager() {}
MobManager::~MobManager() {}

void MobManager::SpawnTowerGuards(const std::vector<TowerGuardSpawn>& spawns) {
    if (!Config::Gameplay::SpawnMobs) return;
    for (const auto& sp : spawns) {
        m_enemyMobs.push_back(std::make_unique<EnemyMob>(sp.pos, static_cast<EnemyType>(sp.type), sp.nightLevel));
    }
}

void MobManager::Init(glm::vec3 playerPos, int monsterCount) {
    m_passiveMobs.clear();
    m_enemyMobs.clear();
    m_waterMonsters.clear();
    m_monsters.clear();
    m_baseMobs.clear();

    // Spawn DummyMob desacoplado y polimorfico (1.000.000 HP)
    float dummyAngle = 0.785f; // Frente a la vista del jugador
    float dummyDist = 4.5f;
    float dummyX = playerPos.x + cos(dummyAngle) * dummyDist;
    float dummyZ = playerPos.z + sin(dummyAngle) * dummyDist;
    float dummyY = WorldGenerator::GetHeight(dummyX, dummyZ);
    m_baseMobs.push_back(std::make_unique<DummyMob>(glm::vec3(dummyX, dummyY, dummyZ)));

    if (!Config::Gameplay::SpawnMobs) {
        m_dragon.SetActive(false);
        std::cout << "[MobManager] Spawning de Mobs regulares DESACTIVADO (DummyMob presente)." << std::endl;
        return;
    }
    m_dragon.SetActive(true);

    // 1. Passive & Hostile Forest Deer (Fawns, Adults, Alphas, Demonic)
    for (int i = 0; i < 14; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = 20.0f + (rand() % 45);
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
        EnemyType::CORRUPTED_WARRIOR,
        EnemyType::SKELETON_ARCHER,
        EnemyType::CORRUPTED_WARRIOR,
        EnemyType::DARK_MAGE,
        EnemyType::VAMPIRE,
        EnemyType::TREANT,
        EnemyType::NEUTRAL_GIANT
    };
    for (int i = 0; i < 14; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = 28.0f + (rand() % 45);
        float ex = playerPos.x + cos(angle) * dist;
        float ez = playerPos.z + sin(angle) * dist;
        float ey = WorldGenerator::GetHeight(ex, ez);
        if (ey > Config::Water::Level + 0.6f) {
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
    if (monsterCount > 0) {
        SpawnNightMonsters(playerPos, monsterCount);
    }
}

void MobManager::SpawnNightMonsters(glm::vec3 playerPos, int count) {
    if (!Config::Gameplay::SpawnMobs) return;

    float minRad = 32.0f;
    float maxRad = 55.0f;
    int diff = std::max(10, (int)(maxRad - minRad));

    for (int i = 0; i < count; ++i) {
        float angle = (float)(rand() % 360) * 0.017453f;
        float dist = minRad + (float)(rand() % diff);
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
                        float dayCycleTime, int nightCount, bool isBloodMoon, FatalErrorPopup* fatalError,
                        TargetingSystem* targeting)
{
    float cycleNorm = fmod(dayCycleTime, 240.0f) / 240.0f;
    if (cycleNorm < 0.0f) cycleNorm += 1.0f;
    bool isNight = (cycleNorm >= 0.50f && cycleNorm <= 0.88f);

    if (!Config::Gameplay::SpawnMobs) {
        m_passiveMobs.clear();
        m_enemyMobs.clear();
        m_waterMonsters.clear();
        m_monsters.clear();
        m_dragon.SetActive(false);

        for (auto& mob : m_baseMobs) {
            mob->Update(deltaTime, player.Position, &player, particles, damageNumbers, projectiles);
        }

        m_birds.Update(deltaTime, player.Position, m_monsters);
        m_birds.CleanupDistantBirds(player.Position, 80.0f);
        m_critters.Update(deltaTime, player.Position);
        return;
    }

    // 0. Base Mobs Polimorficos (DummyMob, etc.)
    for (auto& mob : m_baseMobs) {
        mob->Update(deltaTime, player.Position, &player, particles, damageNumbers, projectiles);
    }

    // Spawning / despawning shadow monsters across night transitions
    if (isNight && !m_wasNight) {
        int count = std::min(2 + (nightCount / 2), 5);
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

    // 2. Passive Mobs (Deer) - Update all so deathTimer and mesh advance
    for (auto& deer : m_passiveMobs) {
        deer->Update(deltaTime, player.Position, &player, particles, damageNumbers);
    }

    // 3. Water Lurkers
    for (auto& wm : m_waterMonsters) {
        wm->Update(deltaTime, player.Position, &player, particles, damageNumbers);
    }

    // 4. Ancestral Dragon Boss
    m_dragon.Update(deltaTime, player.Position, particles, damageNumbers, &player);

    // 5. Enemy Mobs AI & Combat - Update all so deathTimer and mesh advance
    for (auto& enemy : m_enemyMobs) {
        enemy->Update(deltaTime, player.Position, &player, particles, damageNumbers, projectiles);
    }

    // 6. Shadow Monsters AI & Tracking
    glm::vec2 windDir = windSystem.GetDirection();
    m_highestDangerLevel = 0.0f;

    static float s_shadowAttackTimer = 0.0f;
    if (s_shadowAttackTimer > 0.0f) s_shadowAttackTimer -= deltaTime;

    for (auto& m : m_monsters) {
        if (m->IsDead()) continue;
        m->Update(deltaTime, player.Position, player.Front, windDir,
                  chunkManager, scentSystem, particles,
                  player.Velocity, 0, false,
                  player.IsClimbing, player.ClimbingTreePos,
                  (player.HasTorchActive && !player.AreBothHandsOccupied()));

        float dist = glm::distance(player.Position, m->GetPosition());
        if (dist < 24.0f) {
            float d = 1.0f - (dist / 24.0f);
            if (d > m_highestDangerLevel) m_highestDangerLevel = d;
        }

        // Ataque cuerpo a cuerpo del monstruo de las sombras al estar en rango (incluso en pendientes)
        float playerFeetY = player.Position.y - 1.6f;
        float mobFeetY = m->GetPosition().y;
        float groundDiff = playerFeetY - mobFeetY;
        float d2D = glm::distance(glm::vec2(player.Position.x, player.Position.z), glm::vec2(m->GetPosition().x, m->GetPosition().z));
        bool isClimbingTree = player.IsClimbing;

        if (d2D <= 2.6f && !isClimbingTree && groundDiff <= 2.5f && groundDiff >= -2.8f && s_shadowAttackTimer <= 0.0f && !player.IsDead()) {
            s_shadowAttackTimer = 1.15f;
            int dmg = 22 + (nightCount * 4);
            player.TakeDamage(dmg, damageNumbers);
            for (int i = 0; i < 20; ++i) {
                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.0f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*3.0f);
                particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.85f, 0.05f, 0.05f, 1.0f), 0.14f, 0.75f, -9.8f);
            }
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

    // 7. Cleanup targeting if target died
    if (targeting && targeting->HasTarget() && !targeting->IsTargetAlive()) {
        targeting->ClearTarget();
    }

    // 8. Cleanup removable / decayed / distant entities
    m_passiveMobs.erase(std::remove_if(m_passiveMobs.begin(), m_passiveMobs.end(),
        [&](const std::unique_ptr<PassiveMob>& d) {
            if (!d->IsAlive()) {
                bool rem = d->IsSkinned() && d->IsRemovable();
                if (rem && targeting && targeting->GetPassiveTarget() == d.get()) targeting->ClearTarget();
                return rem;
            }
            return glm::distance(player.Position, d->GetPosition()) > 180.0f;
        }), m_passiveMobs.end());

    m_enemyMobs.erase(std::remove_if(m_enemyMobs.begin(), m_enemyMobs.end(),
        [&](const std::unique_ptr<EnemyMob>& e) {
            if (!e->IsAlive()) {
                bool rem = e->IsRemovable() && e->HasDroppedLoot();
                if (rem && targeting && targeting->GetEnemyTarget() == e.get()) targeting->ClearTarget();
                return rem;
            }
            return glm::distance(player.Position, e->GetPosition()) > 180.0f;
        }), m_enemyMobs.end());

    m_baseMobs.erase(std::remove_if(m_baseMobs.begin(), m_baseMobs.end(),
        [&](const std::unique_ptr<BaseMob>& mob) {
            bool rem = mob->IsRemovable();
            if (rem && targeting && targeting->GetBaseMobTarget() == mob.get()) targeting->ClearTarget();
            return rem;
        }), m_baseMobs.end());

    m_waterMonsters.erase(std::remove_if(m_waterMonsters.begin(), m_waterMonsters.end(),
        [&](const std::unique_ptr<WaterMonster>& wm) {
            bool rem = !wm->IsAlive() && wm->IsRemovable();
            if (rem && targeting && targeting->GetWaterTarget() == wm.get()) targeting->ClearTarget();
            return rem;
        }), m_waterMonsters.end());

    m_monsters.erase(std::remove_if(m_monsters.begin(), m_monsters.end(),
        [&](const std::unique_ptr<Monster>& m) {
            bool rem = m->IsDead() && m->HasDroppedLoot();
            if (rem && targeting && targeting->GetMonsterTarget() == m.get()) targeting->ClearTarget();
            return rem;
        }), m_monsters.end());

    // 9. Dynamic Population Spawning & Living World Maintenance
    m_populationTimer += deltaTime;
    if (m_populationTimer >= 2.0f) {
        m_populationTimer = 0.0f;
        MaintainWorldPopulation(player.Position, nightCount, isBloodMoon, isNight);
    }
}

void MobManager::MaintainWorldPopulation(glm::vec3 playerPos, int nightCount, bool isBloodMoon, bool isNight) {
    if (!Config::Gameplay::SpawnMobs) return;

    // 1. Maintain Passive Deer Population (~12-16 living deer within 100m)
    int aliveDeer = 0;
    for (auto& d : m_passiveMobs) {
        if (d->IsAlive() && glm::distance(playerPos, d->GetPosition()) < 100.0f) {
            aliveDeer++;
        }
    }
    if (aliveDeer < 13) {
        int toSpawn = std::min(3, 14 - aliveDeer);
        for (int i = 0; i < toSpawn; ++i) {
            float angle = (float)(rand() % 360) * 0.017453f;
            float dist = 28.0f + (float)(rand() % 35);
            float dx = playerPos.x + cos(angle) * dist;
            float dz = playerPos.z + sin(angle) * dist;
            float dy = WorldGenerator::GetHeight(dx, dz);
            if (dy > Config::Water::Level + 0.6f) {
                int roll = rand() % 10;
                DeerSize size = (roll == 0) ? DeerSize::DEMONIC : ((roll < 3) ? DeerSize::ALPHA : ((roll < 6) ? DeerSize::ADULT : DeerSize::FAWN));
                m_passiveMobs.push_back(std::make_unique<PassiveMob>(glm::vec3(dx, dy, dz), size));
            }
        }
    }

    // 2. Maintain Enemy Mob Population (~12-16 living enemies within 100m)
    if (!isNight) {
        // Los cazadores/asesinos de las sombras NO salen de día: desaparecen con la luz solar
        for (auto& e : m_enemyMobs) {
            if (e->IsAlive() && e->GetType() == EnemyType::SHADOW_ASSASSIN) {
                e->SetDead();
            }
        }
    }

    int aliveEnemies = 0;
    for (auto& e : m_enemyMobs) {
        if (e->IsAlive() && glm::distance(playerPos, e->GetPosition()) < 100.0f) {
            aliveEnemies++;
        }
    }
    if (aliveEnemies < 13) {
        std::vector<EnemyType> availableTypes = {
            EnemyType::BERSERKER_WARRIOR,
            EnemyType::DEATH_KNIGHT,
            EnemyType::SKELETON_ARCHER,
            EnemyType::CORRUPTED_WARRIOR,
            EnemyType::DARK_MAGE,
            EnemyType::VAMPIRE,
            EnemyType::TREANT,
            EnemyType::NEUTRAL_GIANT
        };
        // Los cazadores blancos (Shadow Assassins) SOLO salen de noche
        if (isNight) {
            availableTypes.push_back(EnemyType::SHADOW_ASSASSIN);
            availableTypes.push_back(EnemyType::SHADOW_ASSASSIN); // Mayor probabilidad nocturna
        }

        int toSpawn = std::min(3, 14 - aliveEnemies);
        for (int i = 0; i < toSpawn; ++i) {
            float angle = (float)(rand() % 360) * 0.017453f;
            float dist = 32.0f + (float)(rand() % 35);
            float ex = playerPos.x + cos(angle) * dist;
            float ez = playerPos.z + sin(angle) * dist;
            float ey = WorldGenerator::GetHeight(ex, ez);
            if (ey > Config::Water::Level + 0.6f) {
                EnemyType type = availableTypes[rand() % availableTypes.size()];
                int lvl = std::max(1, nightCount);
                m_enemyMobs.push_back(std::make_unique<EnemyMob>(glm::vec3(ex, ey, ez), type, lvl));
            }
        }
    }

    // 3. Maintain Water Monsters (~2-3 near lakes)
    int aliveWater = 0;
    for (auto& wm : m_waterMonsters) {
        if (wm->IsAlive() && glm::distance(playerPos, wm->GetPosition()) < 100.0f) {
            aliveWater++;
        }
    }
    if (aliveWater < 3) {
        for (int attempts = 0; attempts < 25; ++attempts) {
            float angle = (float)(rand() % 360) * 0.017453f;
            float dist = 25.0f + (float)(rand() % 65);
            float wx = playerPos.x + cos(angle) * dist;
            float wz = playerPos.z + sin(angle) * dist;
            float wy = WorldGenerator::GetHeight(wx, wz);
            if (wy < Config::Water::Level - 0.2f) {
                m_waterMonsters.push_back(std::make_unique<WaterMonster>(glm::vec3(wx, Config::Water::Level - 0.6f, wz)));
                break;
            }
        }
    }

    // 4. Night Shadow Lurkers (maintain while night active, 2-5 monsters)
    if (isNight) {
        int targetShadowCount = std::min(2 + (nightCount / 2), 5);
        if (isBloodMoon) targetShadowCount += 2;
        int aliveShadow = 0;
        for (auto& m : m_monsters) {
            if (!m->IsDead()) aliveShadow++;
        }
        if (aliveShadow < targetShadowCount) {
            SpawnNightMonsters(playerPos, targetShadowCount - aliveShadow);
        }
    }
}

void MobManager::Render(GLuint shaderProgram, glm::vec3 activeCamPos, GLuint textureID, float globalTime) {
    // 1. Birds & Critters
    glBindTexture(GL_TEXTURE_2D, textureID);
    m_birds.Render(shaderProgram);
    m_critters.Render(shaderProgram);

    // 2. Base Mobs Polimorficos (DummyMob, etc.)
    for (auto& mob : m_baseMobs) {
        if (mob->IsAlive()) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            mob->Render(shaderProgram);
            mob->RenderHealthBar(shaderProgram, activeCamPos);
        }
    }

    if (!Config::Gameplay::SpawnMobs) return;

    // 2. Passive Forest Animals (Render alive OR dead unskinned deer)
    glBindTexture(GL_TEXTURE_2D, textureID);
    for (auto& deer : m_passiveMobs) {
        if (!deer->IsAlive() && deer->IsSkinned()) continue; // Keep dead deer visible until skinned
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
        if (!wm->IsAlive()) continue;
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
