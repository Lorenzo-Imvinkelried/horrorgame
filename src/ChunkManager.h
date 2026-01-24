#pragma once

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"
#include "Frustum.h"

struct Chunk {
    int x, z;
    GLuint VAO, VBO;
    int vertexCount;
    std::vector<glm::vec4> treePositions;
    bool visible;
    float chunkSize; // Store for calculating world bounds of chunk
    float scale;

    Chunk() : VAO(0), VBO(0), vertexCount(0), visible(false) {}
    void Cleanup();
};

class ChunkManager {
public:
    ChunkManager(int renderDistance = 8);
    ~ChunkManager();

    void Update(glm::vec3 playerPos);
    void UpdateVisibility(const glm::mat4& viewProj); // New Method
    void RenderTerrain(GLuint shaderProgram);
    
    // Instancing support
    void CollectAllTreePositions(std::vector<glm::vec4>& outPositions);

    // Collision support
    void GetTreesInRange(glm::vec3 pos, float range, std::vector<glm::vec4>& outTrees);

private:
    int m_renderDistance;
    int m_chunkSize = 16;
    float m_scale = 2.0f;
    std::map<std::pair<int, int>, Chunk> m_chunks;

    void LoadChunk(int x, int z);
    void UnloadFarChunks(int playerCX, int playerCZ);
};
