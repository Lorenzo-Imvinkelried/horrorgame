#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct FloatingNumber {
    glm::vec3 Pos;
    glm::vec3 Velocity;
    glm::vec4 Color;
    float Scale;
    float Lifetime;
    float MaxLifetime;
    int Value;
    bool IsCrit;
    bool IsExp;
    bool IsLevelUp;
};

class DamageNumberSystem {
public:
    DamageNumberSystem();
    ~DamageNumberSystem();

    void SpawnDamage(glm::vec3 pos, int damage, bool isCrit);
    void SpawnExp(glm::vec3 pos, int exp);
    void SpawnLevelUp(glm::vec3 pos);

    void Update(float deltaTime);
    void Render(GLuint shaderProgram, glm::vec3 cameraPos);

    const std::vector<FloatingNumber>& GetNumbers() const { return m_numbers; }

private:
    void initMesh();
    void renderDigitQuad(GLuint shaderProgram, glm::vec3 pos, glm::vec3 toCam, float scale, glm::vec4 color, int digit);

    std::vector<FloatingNumber> m_numbers;
    GLuint m_VAO;
    GLuint m_VBO;
};
