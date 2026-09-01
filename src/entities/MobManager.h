#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include "Monster.h"
#include "EnemyMob.h"
#include "PassiveMob.h"
#include "WaterMonster.h"
#include "entities/Dragon.h"
#include "BirdSystem.h"
#include "CritterSystem.h"

class Player;
class ChunkManager;
class ScentSystem;
class WindSystem;
class ParticleSystem;
class DamageNumberSystem;
class ItemDropSystem;
class ProjectileSystem;
struct FatalErrorPopup;

class MobManager {
public:
    MobManager();
    ~MobManager();

    void Init(glm::vec3 playerPos, int monsterCount = 1);
    void Update(float deltaTime, Player& player, ChunkManager& chunkManager, ScentSystem& scentSystem,
                WindSystem& windSystem, ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                ItemDropSystem& itemDropSystem, ProjectileSystem& projectiles, float globalTime,
                float dayCycleTime, int nightCount, bool isBloodMoon, FatalErrorPopup* fatalError);

    void Render(GLuint shaderProgram, glm::vec3 activeCamPos, GLuint textureID, float globalTime);
    void RenderDebug(GLuint shaderProgram);

    void SpawnNightMonsters(glm::vec3 playerPos, int count);
    void DespawnNightMonsters();

    std::vector<std::unique_ptr<Monster>>& GetMonsters() { return m_monsters; }
    std::vector<std::unique_ptr<EnemyMob>>& GetEnemyMobs() { return m_enemyMobs; }
    std::vector<std::unique_ptr<PassiveMob>>& GetPassiveMobs() { return m_passiveMobs; }
    std::vector<std::unique_ptr<WaterMonster>>& GetWaterMonsters() { return m_waterMonsters; }
    Dragon& GetDragon() { return m_dragon; }
    BirdSystem& GetBirds() { return m_birds; }
    CritterSystem& GetCritters() { return m_critters; }

    float GetHighestDangerLevel() const { return m_highestDangerLevel; }

private:
    std::vector<std::unique_ptr<Monster>> m_monsters;
    std::vector<std::unique_ptr<EnemyMob>> m_enemyMobs;
    std::vector<std::unique_ptr<PassiveMob>> m_passiveMobs;
    std::vector<std::unique_ptr<WaterMonster>> m_waterMonsters;
    Dragon m_dragon;
    BirdSystem m_birds;
    CritterSystem m_critters;

    float m_highestDangerLevel = 0.0f;
    float m_dragonRoarTimer = 25.0f;
    bool m_wasNight = false;
};
