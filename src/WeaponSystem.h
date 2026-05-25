#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ParticleSystem.h"
#include "FootprintSystem.h" // For crater decals
#include "ChunkManager.h"    // For raycast

class WeaponSystem {
public:
    WeaponSystem();
    ~WeaponSystem();

    struct Projectile {
        glm::vec3 Position;
        glm::vec3 Velocity;
        float LifeTime;
        bool Active;
    };

    void Update(float deltaTime, glm::vec2 windDir, float windStrength, class ChunkManager& chunkManager, class FootprintSystem& craters, class ParticleSystem& particles, const std::vector<std::unique_ptr<class Monster>>& monsters);
    void Render(GLuint shaderProgram, bool isGameOver = false, float gameOverTimer = 0.0f); // Draw Gun Model
    void RenderProjectiles(GLuint shaderProgram, GLuint vao, GLuint vbo); // Draw Tracers (Needs World View)
    void TryFire(glm::vec3 camPos, glm::vec3 camDir, class ParticleSystem& particles, const std::vector<std::unique_ptr<class Monster>>& monsters);
    
    int GetAmmo() const { return currentAmmo; }
    bool IsReloading() const { return reloadTimer > 0.0f; }
    void Reload();

private:
    GLuint VAO, VBO;
    int currentAmmo;
    int maxAmmo;
    float recoilTimer;
    float cooldownTimer;
    float reloadTimer;
    
    std::vector<Projectile> projectiles;
    
    // Procedural Model Data
    std::vector<float> gunVertices;
    void BuildShotgunMesh();
};
