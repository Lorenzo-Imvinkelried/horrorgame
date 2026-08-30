#pragma once

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "WorldGenerator.h"
#include "Frustum.h"
#include "Config.h" // Needed for default args
#include <iostream> // Fixed: Needed for cout/endl

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
    void RenderWater(GLuint shaderProgram); // Water Pass (Transparent)
    void RenderTrees(GLuint shaderProgram, const GLuint trunkVAO[4], const GLuint leavesVAO[4], const int trunkVertexCount[4], const int leavesVertexCount[4], glm::vec3 playerPos);
    void RenderDebug(GLuint shaderProgram, glm::vec3 playerPos);

    // Collision support
    void GetTreesInRange(glm::vec3 pos, float range, std::vector<glm::vec4>& outTrees);

    // Dynamic Terrain Destruction & Construction (Smooth Deformation)
    bool ModifyTerrain(float worldX, float worldZ, float radius, float deltaHeight, class ParticleSystem* particles = nullptr);

    // Batched Rendering Support
    struct RenderBatch {
        int bx, bz;
        GLuint VAO, VBO;
        GLuint waterVAO, waterVBO; 
        
        // Tree Instance Data (Static per batch, 4 archetypes)
        GLuint treeInstanceVBO[4]; 
        int treeCount[4];

        int vertexCount;
        int waterVertexCount;
        bool visible;
        bool dirty;
        float distFromCam; // For Front-to-Back Sorting
        
        RenderBatch() : VAO(0), VBO(0), waterVAO(0), waterVBO(0), vertexCount(0), waterVertexCount(0), visible(false), dirty(true), distFromCam(0.0f) {
            for (int i = 0; i < 4; ++i) {
                treeInstanceVBO[i] = 0;
                treeCount[i] = 0;
            }
        }
        void Cleanup() {
            if(VAO) glDeleteVertexArrays(1, &VAO);
            if(VBO) glDeleteBuffers(1, &VBO);
            if(waterVAO) glDeleteVertexArrays(1, &waterVAO);
            if(waterVBO) glDeleteBuffers(1, &waterVBO);
            for (int i = 0; i < 4; ++i) {
                if(treeInstanceVBO[i]) glDeleteBuffers(1, &treeInstanceVBO[i]);
                treeInstanceVBO[i] = 0;
                treeCount[i] = 0;
            }
            VAO=0; VBO=0; waterVAO=0; waterVBO=0; vertexCount=0;
        }
    };

private:
    int m_renderDistance;
    int m_chunkSize = Config::World::ChunkSize;
    float m_scale = Config::World::ChunkScale;
    
    // Logic Map (Collision, Trees)
    std::map<std::pair<int, int>, Chunk> m_chunks; // Still used for logic
    
    // Render Map (Batches)
    // Render Map (Batches)
    std::map<std::pair<int, int>, RenderBatch> m_batches;
    std::vector<RenderBatch*> m_batchList; // Cache for fast iteration
    std::vector<RenderBatch*> m_visibleBatches; // SORTED cache for rendering
    struct BatchBounds {
        glm::vec3 min;
        glm::vec3 max;
    };
    std::map<std::pair<int, int>, BatchBounds> m_batchBoundsCache;

    void LoadChunk(int x, int z);
    
public:
    // Optimized: Pre-load entire world
    void Init() {
        std::cout << "[ChunkManager] Pre-loading World (" << Config::World::MapRadius << " chunk radius)..." << std::endl;
        LoadWorld(); 
    }

    // New Render Method for Trees
    void RenderTrees(GLuint shaderProgram, GLuint trunkVAO, GLuint leavesVAO, int trunkVertexCount, int leavesVertexCount, glm::vec3 playerPos);

private:
    void LoadWorld();
    
    // Batching Helpers
    RenderBatch* GetBatch(int bx, int bz);
    void RebuildBatch(RenderBatch& batch);
    void MarkBatchDirty(int cx, int cz);
    
    // Bird System Integration
    class BirdSystem* m_birdSystem = nullptr;
public:
    void SetBirdSystem(class BirdSystem* birds) { 
        m_birdSystem = birds; 
        std::cout << "[ChunkManager] BirdSystem linked: " << (birds ? "YES" : "NO") << std::endl;
    }
};
