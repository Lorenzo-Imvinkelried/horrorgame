#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

enum class TargetType {
    NONE,
    PASSIVE_MOB,
    MONSTER,
    ENEMY_MOB,
    WATER_MONSTER
};

class PassiveMob;
class Monster;
class EnemyMob;
class WaterMonster;

class TargetingSystem {
public:
    TargetingSystem() 
        : m_targetType(TargetType::NONE)
        , m_passiveTarget(nullptr)
        , m_monsterTarget(nullptr)
        , m_enemyTarget(nullptr)
        , m_waterTarget(nullptr)
        , m_autoApproaching(false)
        , m_ringAngle(0.0f)
        , m_ringVAO(0)
        , m_ringVBO(0)
    {
        initRingMesh();
    }

    ~TargetingSystem() {
        if (m_ringVAO) glDeleteVertexArrays(1, &m_ringVAO);
        if (m_ringVBO) glDeleteBuffers(1, &m_ringVBO);
    }

    void SelectPassive(PassiveMob* mob) {
        m_passiveTarget = mob;
        m_monsterTarget = nullptr;
        m_enemyTarget = nullptr;
        m_waterTarget = nullptr;
        m_targetType = TargetType::PASSIVE_MOB;
    }

    void SelectMonster(Monster* monster) {
        m_monsterTarget = monster;
        m_passiveTarget = nullptr;
        m_enemyTarget = nullptr;
        m_waterTarget = nullptr;
        m_targetType = TargetType::MONSTER;
    }

    void SelectEnemy(EnemyMob* enemy) {
        m_enemyTarget = enemy;
        m_passiveTarget = nullptr;
        m_monsterTarget = nullptr;
        m_waterTarget = nullptr;
        m_targetType = TargetType::ENEMY_MOB;
    }

    void SelectWaterMonster(WaterMonster* waterMonster) {
        m_waterTarget = waterMonster;
        m_passiveTarget = nullptr;
        m_monsterTarget = nullptr;
        m_enemyTarget = nullptr;
        m_targetType = TargetType::WATER_MONSTER;
    }

    void ClearTarget() {
        m_targetType = TargetType::NONE;
        m_passiveTarget = nullptr;
        m_monsterTarget = nullptr;
        m_enemyTarget = nullptr;
        m_waterTarget = nullptr;
        m_autoApproaching = false;
    }

    bool HasTarget() const {
        return m_targetType != TargetType::NONE;
    }

    TargetType GetTargetType() const { return m_targetType; }
    PassiveMob* GetPassiveTarget() const { return m_passiveTarget; }
    Monster* GetMonsterTarget() const { return m_monsterTarget; }
    EnemyMob* GetEnemyTarget() const { return m_enemyTarget; }
    WaterMonster* GetWaterTarget() const { return m_waterTarget; }

    void SetAutoApproaching(bool autoApproach) {
        m_autoApproaching = autoApproach;
    }

    bool IsAutoApproaching() const {
        return m_autoApproaching;
    }

    glm::vec3 GetTargetPosition() const;
    std::string GetTargetName() const;
    int GetTargetLevel() const;
    int GetTargetCurrentHP() const;
    int GetTargetMaxHP() const;
    bool IsTargetAlive() const;

    void Update(float deltaTime, glm::vec3 playerPos, bool manualInputActive);
    void RenderTargetRing(GLuint shaderProgram);

private:
    void initRingMesh();

    TargetType m_targetType;
    PassiveMob* m_passiveTarget;
    Monster* m_monsterTarget;
    EnemyMob* m_enemyTarget;
    WaterMonster* m_waterTarget;
    bool m_autoApproaching;
    float m_ringAngle;

    GLuint m_ringVAO;
    GLuint m_ringVBO;
    int m_ringVertexCount;
};
