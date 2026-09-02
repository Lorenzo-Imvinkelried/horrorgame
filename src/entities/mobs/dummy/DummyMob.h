#pragma once

#include "../BaseMob.h"
#include "ModelLoader.h"
#include "ParticleSystem.h"
#include "combat/DamageNumberSystem.h"
#include "ProjectileSystem.h"

/**
 * @brief DummyMob: Maniquí de pruebas de combate.
 * - Carga su modelo desde assets/models/mobs/dummy.txt
 * - Tiene 1.000.000 HP, 0 defensa y 0 evasión.
 * - Totalmente estático (sin movimiento, sin IA agresiva).
 * - Se regenera instantáneamente a 1.000.000 HP si su vida llega a 0 para permitir pruebas continuas.
 */
class DummyMob : public BaseMob {
public:
    DummyMob(glm::vec3 spawnPos);
    ~DummyMob() override = default;

    void Update(float deltaTime, glm::vec3 playerPos, Player* player,
                ParticleSystem& particles, DamageNumberSystem& damageNumbers,
                ProjectileSystem& projectiles) override;

    void Render(GLuint shaderProgram) override;

    bool TakeDamage(int damage, glm::vec3 hitOrigin,
                    ParticleSystem& particles, Player* player,
                    DamageNumberSystem& damageNumbers) override;

private:
    void initMesh();
};
