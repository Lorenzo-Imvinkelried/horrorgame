#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

class Player;
class ParticleSystem;
class DamageNumberSystem;
class ProjectileSystem;

/**
 * @brief Clase base abstracta para todas las entidades/mobs del juego.
 * Define la interfaz polimórfica común para ciclo de vida, actualización,
 * renderizado 3D, combate y barras de salud.
 */
class BaseMob {
public:
    BaseMob(glm::vec3 spawnPos, const std::string& name = "Mob");
    virtual ~BaseMob();

    // Actualización por frame
    virtual void Update(float deltaTime, glm::vec3 playerPos, Player* player,
                        ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                        ProjectileSystem& projectiles) = 0;

    // Renderizado 3D
    virtual void Render(GLuint shaderProgram) = 0;

    // Renderizado de Barra de Salud (Billboard genérico sobre la cabeza)
    virtual void RenderHealthBar(GLuint shaderProgram, glm::vec3 cameraPos);

    // Sistema de Combate y Daño
    virtual bool TakeDamage(int damage, glm::vec3 hitOrigin,
                            ParticleSystem& particles, Player* player,
                            DamageNumberSystem& damageNumbers) = 0;

    // Getters y Setters de Posición y Transformación
    glm::vec3 GetPosition() const { return m_pos; }
    glm::vec3& GetPositionRef() { return m_pos; }
    void SetPosition(const glm::vec3& p) { m_pos = p; }
    float GetYaw() const { return m_yaw; }
    void SetYaw(float y) { m_yaw = y; }
    float GetScale() const { return m_scale; }

    // Getters de Estadísticas RPG
    const std::string& GetName() const { return m_name; }
    int GetLevel() const { return m_level; }
    int GetCurrentHP() const { return m_currentHp; }
    int GetMaxHP() const { return m_maxHp; }
    int GetDefense() const { return m_defense; }
    int GetEvasion() const { return m_evasion; }
    int GetExpReward() const { return m_expReward; }
    float GetRadius() const { return m_radius; }

    // Estado de Vida y Limpieza
    virtual bool IsAlive() const { return m_currentHp > 0; }
    virtual bool IsRemovable() const { return !IsAlive() && m_deathTimer > 4.5f; }
    bool HasDroppedLoot() const { return m_lootDropped; }
    void SetLootDropped(bool dropped) { m_lootDropped = dropped; }

protected:
    static void initHpBarMesh();

    glm::vec3 m_pos;
    float m_yaw = 0.0f;
    float m_scale = 1.0f;

    std::string m_name;
    int m_level = 1;
    int m_maxHp = 100;
    int m_currentHp = 100;
    int m_defense = 0;
    int m_evasion = 0;
    int m_expReward = 0;
    float m_radius = 0.85f;

    float m_hitFlashTimer = 0.0f;
    float m_showHpBarTimer = 0.0f;
    float m_deathTimer = 0.0f;
    bool m_lootDropped = false;

    // Recursos de OpenGL
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    size_t m_vertexCount = 0;

    static GLuint s_hpBarVAO;
    static GLuint s_hpBarVBO;
};
