#pragma once

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"
#include "Frustum.h"
#include "Config.h" // Needed for default args

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
    ChunkManager(int renderDistance = Config::World::RenderDistance);
    ~ChunkManager();

    void Update(glm::vec3 playerPos);
    void UpdateVisibility(const glm::mat4& viewProj); // New Method
    void RenderTerrain(GLuint shaderProgram);
    
    // Instancing support
    void CollectAllTreePositions(std::vector<glm::vec4>& outPositions);

    // Collision support
    void GetTreesInRange(glm::vec3 pos, float range, std::vector<glm::vec4>& outTrees);

    // Batched Rendering Support
    struct RenderBatch {
        int bx, bz;
        GLuint VAO, VBO;
        int vertexCount;
        bool visible;
        bool dirty;
        
        RenderBatch() : VAO(0), VBO(0), vertexCount(0), visible(false), dirty(true) {}
        void Cleanup() {
            if(VAO) glDeleteVertexArrays(1, &VAO);
            if(VBO) glDeleteBuffers(1, &VBO);
            VAO=0; VBO=0; vertexCount=0;
        }
    };

private:
    int m_renderDistance;
    int m_chunkSize = Config::World::ChunkSize;
    float m_scale = Config::World::ChunkScale;
    
    // Logic Map (Collision, Trees)
    std::map<std::pair<int, int>, Chunk> m_chunks; // Still used for logic
    
    // Render Map (Batches)
    std::map<std::pair<int, int>, RenderBatch> m_batches;

    void LoadChunk(int x, int z);
    
    // Optimized: Pre-load entire world
    void LoadWorld();
    
    // Batching Helpers
    RenderBatch* GetBatch(int bx, int bz);
    void RebuildBatch(RenderBatch& batch);
    void MarkBatchDirty(int cx, int cz);
};
