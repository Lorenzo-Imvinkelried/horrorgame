#include "ChunkManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

void Chunk::Cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    VAO = VBO = 0;
}

ChunkManager::ChunkManager(int renderDistance) : m_renderDistance(renderDistance) {}

ChunkManager::~ChunkManager() {
    for (auto& pair : m_chunks) {
        pair.second.Cleanup();
    }
}

void ChunkManager::Update(glm::vec3 playerPos) {
    int pCX = (int)floor(playerPos.x / (m_chunkSize * m_scale));
    int pCZ = (int)floor(playerPos.z / (m_chunkSize * m_scale));

    // INCREMENTAL LOADING: Load a limited number of chunks per frame to avoid lag spikes
    int loadedThisFrame = 0;
    const int MaxwellLoadPerFrame = 1; 

    // Sort potential chunks by distance to player for better streaming priority
    std::vector<std::pair<int, int>> potentialChunks;
    for (int r = 0; r <= m_renderDistance; r++) {
        for (int z = -r; z <= r; z++) {
            for (int x = -r; x <= r; x++) {
                if (abs(x) != r && abs(z) != r) continue; 
                potentialChunks.push_back({pCX + x, pCZ + z});
            }
        }
    }

    // Load first N missing chunks
    for (auto& pos : potentialChunks) {
         if (m_chunks.find(pos) == m_chunks.end()) {
             LoadChunk(pos.first, pos.second);
             loadedThisFrame++;
             if (loadedThisFrame >= MaxwellLoadPerFrame) goto end_loops;
         }
    }

end_loops:
    // Only unload occasionally or when far away to keep it smooth
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        UnloadFarChunks(pCX, pCZ);
    }
}

void ChunkManager::LoadChunk(int x, int z) {
    auto key = std::make_pair(x, z);
    // Double check
    if (m_chunks.find(key) != m_chunks.end()) return;

    Chunk newChunk;
    newChunk.x = x;
    newChunk.z = z;
    newChunk.chunkSize = (float)m_chunkSize;
    newChunk.scale = m_scale;
    newChunk.visible = true;

    auto vertices = WorldGenerator::GenerateChunkTerrain(x, z, m_chunkSize, m_scale);
    newChunk.treePositions = WorldGenerator::GenerateChunkTrees(x, z, m_chunkSize, m_scale);
    newChunk.vertexCount = (int)vertices.size();

    glGenVertexArrays(1, &newChunk.VAO);
    glGenBuffers(1, &newChunk.VBO);

    glBindVertexArray(newChunk.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, newChunk.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(3);

    m_chunks[key] = newChunk;
}

void ChunkManager::UnloadFarChunks(int pCX, int pCZ) {
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
        int dx = abs(it->first.first - pCX);
        int dz = abs(it->first.second - pCZ);
        // Larger buffer for unloading to avoid thrashing
        if (dx > m_renderDistance + 4 || dz > m_renderDistance + 4) {
            it->second.Cleanup();
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void ChunkManager::UpdateVisibility(const glm::mat4& viewProj) {
    Frustum frustum;
    frustum.Update(viewProj);

    for (auto& pair : m_chunks) {
        Chunk& c = pair.second;
        
        // Calculate AABB for chunk
        float minX = (float)c.x * m_chunkSize * m_scale;
        float minZ = (float)c.z * m_chunkSize * m_scale;
        float maxX = minX + m_chunkSize * m_scale;
        float maxZ = minZ + m_chunkSize * m_scale;
        
        // Use conservative height bounds (-10 to 30) to cover valleys and tall trees
        glm::vec3 min(minX, -10.0f, minZ);
        glm::vec3 max(maxX, 30.0f, maxZ);

        c.visible = frustum.IsBoxVisible(min, max);
    }
}

void ChunkManager::RenderTerrain(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));

    for (auto& pair : m_chunks) {
        if (!pair.second.visible) continue; // CULLING
        glBindVertexArray(pair.second.VAO);
        glDrawArrays(GL_TRIANGLES, 0, pair.second.vertexCount);
    }
}

void ChunkManager::CollectAllTreePositions(std::vector<glm::vec4>& outPositions) {
    // Optimization: reserve space
    outPositions.reserve(m_chunks.size() * 15);
    for (auto& pair : m_chunks) {
        if (!pair.second.visible) continue; // CULLING
        outPositions.insert(outPositions.end(), pair.second.treePositions.begin(), pair.second.treePositions.end());
    }
}

void ChunkManager::GetTreesInRange(glm::vec3 pos, float range, std::vector<glm::vec4>& outTrees) {
    // Spatial Optimization: Only check chunks that could possibly contain trees in range
    // Calculate the min/max chunk coordinates that overlap with the query box (pos +/- range)
    
    float worldChunkSize = m_chunkSize * m_scale;
    
    int minCX = (int)floor((pos.x - range - 5.0f) / worldChunkSize);
    int maxCX = (int)floor((pos.x + range + 5.0f) / worldChunkSize);
    int minCZ = (int)floor((pos.z - range - 5.0f) / worldChunkSize);
    int maxCZ = (int)floor((pos.z + range + 5.0f) / worldChunkSize);

    for (int z = minCZ; z <= maxCZ; z++) {
        for (int x = minCX; x <= maxCX; x++) {
            auto it = m_chunks.find({x, z});
            if (it == m_chunks.end()) continue; // Chunk not loaded

            const Chunk& chunk = it->second;
            
            // Check trees in this chunk (Standard logic)
            for (const auto& treeData : chunk.treePositions) {
                glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
                float treeScale = treeData.w;
                float distSq = glm::dot(pos - treePos, pos - treePos);
                
                float effectiveRadius = 0.5f * treeScale;
                if (distSq <= (range + effectiveRadius) * (range + effectiveRadius)) {
                    outTrees.push_back(treeData);
                }
            }
        }
    }
}
