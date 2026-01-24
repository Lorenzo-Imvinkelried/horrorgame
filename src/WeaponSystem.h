#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ParticleSystem.h"
#include "FootprintSystem.h" // For crater decals
#include "ChunkManager.h"    // For raycast

class WeaponSystem {
public:
    WeaponSystem();
    ~WeaponSystem();

    void Update(float deltaTime);
    void Render(GLuint shaderProgram); // Draw Gun Model
    void TryFire(glm::vec3 camPos, glm::vec3 camDir, ParticleSystem& particles, FootprintSystem& craters, class ChunkManager& chunkManager);

    int GetAmmo() const { return currentAmmo; }

private:
    GLuint VAO, VBO;
    int currentAmmo;
    int maxAmmo;
    float recoilTimer;
    float cooldownTimer;

    // Procedural Model Data
    std::vector<float> gunVertices;
    void BuildShotgunMesh();
};
