#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Footprint {
    glm::vec3 pos;
    float rotation;
    float life; // 0.0 to 1.0 (or max life)
};

class FootprintSystem {
public:
    FootprintSystem();
    ~FootprintSystem();

    void AddFootprint(glm::vec3 pos, float rotation);
    void Update(float deltaTime);
    void Render(GLuint shaderProgram);

private:
    std::vector<Footprint> footprints;
    GLuint VAO, VBO, TexID, instanceVBO;
    int maxFootprints = 100; // Increased
    int m_vertexCount = 6;

    struct FullVert {
        float x,y,z;
        float r,g,b;
        float u,v;
        float nx,ny,nz;
    };
};
