#include "ChunkManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include "Config.h" // Added for Config constants

void Chunk::Cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    VAO = VBO = 0;
}

ChunkManager::ChunkManager(int renderDistance) : m_renderDistance(renderDistance) {
    // PRE-LOAD WORLD (Static Optimization)
    std::cout << "[ChunkManager] Pre-loading World (" << Config::World::MapRadius << " chunk radius)..." << std::endl;
    LoadWorld();
}

ChunkManager::~ChunkManager() {
    for (auto& pair : m_chunks) {
        pair.second.Cleanup();
    }
    for (auto& pair : m_batches) {
        pair.second.Cleanup();
    }
}

void ChunkManager::LoadWorld() {
    int r = Config::World::MapRadius;
    
    // 1. Load All Chunks
    for (int z = -r; z <= r; z++) {
        for (int x = -r; x <= r; x++) {
            LoadChunk(x, z);
        }
    }
    
    // 2. Build All Batches IMMEDIATELY
    // This forces the "loading time" to happen here, not during gameplay
    int batchesBuilt = 0;
    for (auto& pair : m_batches) {
        if (pair.second.dirty) {
            RebuildBatch(pair.second);
            batchesBuilt++;
        }
    }
    std::cout << "[ChunkManager] World Loaded. Built " << batchesBuilt << " batches." << std::endl;
}

void ChunkManager::Update(glm::vec3 playerPos) {
    // STATIC WORLD: No dynamic loading/unloading!
    // We only update logic that needs per-frame attention if any.
    // Currently nothing.
    // Visibility is handled in UpdateVisibility.
    
    // Just in case we add logic later...
    int pCX = (int)floor(playerPos.x / (m_chunkSize * m_scale));
    int pCZ = (int)floor(playerPos.z / (m_chunkSize * m_scale));
    
    // Optional: We could trigger physics updates for nearby chunks here?
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
    newChunk.visible = true; // Visibility is now handled by batch usually, but we keep this for logic queries?
    
    // We DON'T generate individual VBOs anymore to save VRAM and Setup time.
    // But we DO need tree positions immediately for gameplay.
    // WorldGenerator::GenerateChunkTerrain is called in RebuildBatch now.
    
    // Wait, for Trees we need to call it here.
    newChunk.treePositions = WorldGenerator::GenerateChunkTrees(x, z, m_chunkSize, m_scale);
    
    // VAO/VBO/VertexCount are 0/Unused for individual chunks now.
    newChunk.VAO = 0;
    newChunk.VBO = 0;
    newChunk.vertexCount = 0;

    m_chunks[key] = newChunk;
    
    // Trigger Batch Update
    MarkBatchDirty(x, z);
}

// UnloadFarChunks removed (Static World)

void ChunkManager::UpdateVisibility(const glm::mat4& viewProj) {
    Frustum frustum;
    frustum.Update(viewProj);

    // Update BATCH Visibility
    for (auto& pair : m_batches) {
        RenderBatch& b = pair.second;
        
        // Calculate AABB for BATCH
        // Size: BatchSize * ChunkSize * Scale
        float batchWorldSize = Config::World::RenderBatchSize * m_chunkSize * m_scale;
        float minX = (float)b.bx * batchWorldSize;
        float minZ = (float)b.bz * batchWorldSize;
        float maxX = minX + batchWorldSize;
        float maxZ = minZ + batchWorldSize;
        
        // Use conservative height bounds (-10 to 30)
        glm::vec3 min(minX, -10.0f, minZ);
        glm::vec3 max(maxX, 30.0f, maxZ);

        b.visible = frustum.IsBoxVisible(min, max);
    }
}

// Helper to get Batch Coord from Chunk Coord
// Bx = floor(cx / 4)
int GetBatchCoord(int c) {
    if (c >= 0) return c / Config::World::RenderBatchSize;
    // Handle negative correctly: -1 -> -1 (if size 4, -4..-1 is -1)
    return (c - Config::World::RenderBatchSize + 1) / Config::World::RenderBatchSize;
}

void ChunkManager::MarkBatchDirty(int cx, int cz) {
    int bx = GetBatchCoord(cx);
    int bz = GetBatchCoord(cz);
    auto it = m_batches.find({bx, bz});
    if (it != m_batches.end()) {
        it->second.dirty = true;
    } else {
        // Create if not exists (Lazy creation)
        RenderBatch batch;
        batch.bx = bx;
        batch.bz = bz;
        m_batches[{bx, bz}] = batch;
    }
}

void ChunkManager::RebuildBatch(RenderBatch& batch) {
    // Combine geometry of all loaded chunks in this batch
    std::vector<Vertex> batchVertices;
    
    int B_SIZE = Config::World::RenderBatchSize;
    int startCX = batch.bx * B_SIZE;
    int startCZ = batch.bz * B_SIZE;
    
    for (int z = 0; z < B_SIZE; z++) {
        for (int x = 0; x < B_SIZE; x++) {
            int cx = startCX + x;
            int cz = startCZ + z;
            
            // Check if chunk exists (loaded)
            auto it = m_chunks.find({cx, cz});
            if (it == m_chunks.end()) continue;
            
            // OPTIMIZATION: Collect nearby trees from cached chunks instead of regenerating logic
            std::vector<glm::vec2> nearbyTrees;
            // Scan 3x3 area around this chunk
            for(int dx=-1; dx<=1; dx++) {
                for(int dz=-1; dz<=1; dz++) {
                    auto nIt = m_chunks.find({cx+dx, cz+dz});
                    if (nIt != m_chunks.end()) {
                        // Extract XZ from vec4
                        for(const auto& t : nIt->second.treePositions) {
                            nearbyTrees.push_back(glm::vec2(t.x, t.z));
                        }
                    } else {
                         // Fallback: If neighbor not loaded, likely edge of world.
                         // Generating on fly is slow, skipping might cause missing shadows at edges.
                         // For performance, let's skip shadows from unloaded chunks.
                    }
                }
            }
            
            auto vertices = WorldGenerator::GenerateChunkTerrain(cx, cz, m_chunkSize, m_scale, nearbyTrees);
            batchVertices.insert(batchVertices.end(), vertices.begin(), vertices.end());
        }
    }
    
    // Upload to Batch VBO
    if (batch.VAO == 0) {
        glGenVertexArrays(1, &batch.VAO);
        glGenBuffers(1, &batch.VBO);
    }
    
    glBindVertexArray(batch.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, batch.VBO);
    glBufferData(GL_ARRAY_BUFFER, batchVertices.size() * sizeof(Vertex), batchVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    
    batch.vertexCount = (int)batchVertices.size();
    batch.dirty = false;
}

void ChunkManager::RenderTerrain(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));

    // Render BATCHES instead of CHUNKS
    for (auto& pair : m_batches) {
        if (!pair.second.visible) continue;
        if (pair.second.vertexCount == 0) continue;
        
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
