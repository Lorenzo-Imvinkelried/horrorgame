#include "ChunkManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include "Config.h" 
#include "BirdSystem.h" 

void Chunk::Cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    VAO = VBO = 0;
}

ChunkManager::ChunkManager(int renderDistance) : m_renderDistance(renderDistance) {
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
    int batchesBuilt = 0;
    for (auto& pair : m_batches) {
        if (pair.second.dirty) {
            RebuildBatch(pair.second);
            batchesBuilt++;
        }
    }
    
    // CACHE BATCH LIST
    m_batchList.clear();
    m_batchList.reserve(m_batches.size());
    for(auto& pair : m_batches) {
        m_batchList.push_back(&pair.second);
    }

    std::cout << "[ChunkManager] World Loaded. Built " << batchesBuilt << " batches." << std::endl;
}

void ChunkManager::Update(glm::vec3 playerPos) {
    // Static world update logic if needed
}

void ChunkManager::LoadChunk(int x, int z) {
    auto key = std::make_pair(x, z);
    if (m_chunks.find(key) != m_chunks.end()) return;

    Chunk newChunk;
    newChunk.x = x;
    newChunk.z = z;
    newChunk.chunkSize = (float)m_chunkSize;
    newChunk.scale = m_scale;
    newChunk.visible = true; 
    
    newChunk.treePositions = WorldGenerator::GenerateChunkTrees(x, z, m_chunkSize, m_scale);
    
    if (m_birdSystem) {
        m_birdSystem->TrySpawnBirds(newChunk.treePositions);
    }
    
    newChunk.VAO = 0;
    newChunk.VBO = 0;
    newChunk.vertexCount = 0;

    m_chunks[key] = newChunk;
    MarkBatchDirty(x, z);
}

void ChunkManager::UpdateVisibility(const glm::mat4& viewProj) {
    Frustum frustum;
    frustum.Update(viewProj);

    // Pre-calculate batch size ONCE
    static float batchWorldSize = Config::World::RenderBatchSize * m_chunkSize * m_scale;
    static float margin = 15.0f; // Margin to prevent popping

    m_visibleBatches.clear();
    
    // Extract Camera Position from View Matrix (Inverse)?
    // Actually, UpdateVisibility only takes ViewProj. 
    // We can approximate distance using the ViewProj, but typically we need logic pos.
    // Let's assume the calling code passes ViewProj built from PlayerPos.
    // To sort front-to-back, we need distance.
    // We can obtain camera position if we pass it, OR we can extract it from View Matrix inversed.
    // However, simply using the CENTER of the batch and checking Z distance in View Space is enough?
    // Optimization: Just pass player pos to UpdateVisibility? 
    // Current signature: void UpdateVisibility(const glm::mat4& viewProj);
    // I can't change signature easily in .h without potentially breaking things if I missed update.
    // Wait, I just edited .h, I COULD have added arguments.
    // BUT I can also estimate distance using the Projected Depth of the center?
    // Easier: Just update signature to take glm::vec3 camPos.
    // Actually, let's keep it simple. We iterate m_batchList.
    
    // NOTE: Need to change signature in .cpp match .h? I didn't change .h signature yet.
    // Let's stick to existing signature and compute simple distance metric.
    // We can infer camera position relative to batch by checking the VIEW matrix part?
    // Or just use the fact that `ChunkManager::Update` takes `playerPos`. 
    // We can store `m_lastPlayerPos`.
    // Let's just use `m_lastPlayerPos` if available? 
    // ChunkManager::Update(playerPos) is called in Main loop BEFORE UpdateVisibility.
    // So I can store playerPos in member variable?
    // I didn't add member variable.
    // Alternative: Use the "Projected W" of the center point? 
    // Lower W = Closer.
    
    for (RenderBatch* bPtr : m_batchList) {
        RenderBatch& b = *bPtr;
        
        // Cache Key
        auto key = std::make_pair(b.bx, b.bz);
        
        // Check Cache
        if (m_batchBoundsCache.find(key) == m_batchBoundsCache.end()) {
             // Calculate and Cache
             float minX = (float)b.bx * batchWorldSize;
             float minZ = (float)b.bz * batchWorldSize;
             float maxX = minX + batchWorldSize;
             float maxZ = minZ + batchWorldSize;
             
             BatchBounds bounds;
             bounds.min = glm::vec3(minX - margin, -50.0f, minZ - margin); 
             bounds.max = glm::vec3(maxX + margin, 150.0f, maxZ + margin);
             
             m_batchBoundsCache[key] = bounds;
        }
        
        const auto& bounds = m_batchBoundsCache[key];
        b.visible = frustum.IsBoxVisible(bounds.min, bounds.max);
        
        if (b.visible) {
            // Estimate Distance for Sorting (Center of batch)
            glm::vec3 center = (bounds.min + bounds.max) * 0.5f;
            // Project Center to Clip Space
            glm::vec4 clip = viewProj * glm::vec4(center, 1.0f);
            // W component is roughly linear distance (for perspective)
            b.distFromCam = clip.w;
            
            // STRICT FOG CULLING
            // If the batch is completely beyond the fog, don't draw it.
            // Add margin for large batches
            if (b.distFromCam > (Config::World::FogDistEnd + 50.0f)) {
                 b.visible = false;
                 continue;
            }
            
            m_visibleBatches.push_back(bPtr);
        }
    }
    
    // Sort Front-to-Back (Ascending Distance)
    std::sort(m_visibleBatches.begin(), m_visibleBatches.end(), [](RenderBatch* a, RenderBatch* b) {
        return a->distFromCam < b->distFromCam;
    });
}

int GetBatchCoord(int c) {
    if (c >= 0) return c / Config::World::RenderBatchSize;
    return (c - Config::World::RenderBatchSize + 1) / Config::World::RenderBatchSize;
}

void ChunkManager::MarkBatchDirty(int cx, int cz) {
    int bx = GetBatchCoord(cx);
    int bz = GetBatchCoord(cz);
    auto it = m_batches.find({bx, bz});
    if (it != m_batches.end()) {
        it->second.dirty = true;
    } else {
        RenderBatch batch;
        batch.bx = bx;
        batch.bz = bz;
        m_batches[{bx, bz}] = batch;
    }
}

void ChunkManager::RebuildBatch(RenderBatch& batch) {
    int B_SIZE = Config::World::RenderBatchSize;

    std::vector<Vertex> batchVertices;
    std::vector<Vertex> batchWaterVertices; 
    batchVertices.reserve(B_SIZE * B_SIZE * 6 * 256); 
    
    int startCX = batch.bx * B_SIZE;
    int startCZ = batch.bz * B_SIZE;
    
    for (int z = 0; z < B_SIZE; z++) {
        for (int x = 0; x < B_SIZE; x++) {
            int cx = startCX + x;
            int cz = startCZ + z;
            
            auto it = m_chunks.find({cx, cz});
            if (it == m_chunks.end()) continue;
            
            std::vector<glm::vec2> nearbyTrees;
            for(int dx=-1; dx<=1; dx++) {
                for(int dz=-1; dz<=1; dz++) {
                    auto nIt = m_chunks.find({cx+dx, cz+dz});
                    if (nIt != m_chunks.end()) {
                        for(const auto& t : nIt->second.treePositions) {
                            nearbyTrees.push_back(glm::vec2(t.x, t.z));
                        }
                    } 
                }
            }
            
            WorldData data = WorldGenerator::GenerateChunkTerrain(cx, cz, m_chunkSize, m_scale, nearbyTrees);
            batchVertices.insert(batchVertices.end(), data.vertices.begin(), data.vertices.end());
            batchWaterVertices.insert(batchWaterVertices.end(), data.waterVertices.begin(), data.waterVertices.end());
        }
    }
    
    // Upload Batch VBO (Terrain)
    if (batch.VAO == 0) { glGenVertexArrays(1, &batch.VAO); glGenBuffers(1, &batch.VBO); }
    glBindVertexArray(batch.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, batch.VBO);
    glBufferData(GL_ARRAY_BUFFER, batchVertices.size() * sizeof(Vertex), batchVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    batch.vertexCount = (int)batchVertices.size();

    // Upload Batch VBO (Water)
    if (batch.waterVAO == 0) { glGenVertexArrays(1, &batch.waterVAO); glGenBuffers(1, &batch.waterVBO); }
    glBindVertexArray(batch.waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, batch.waterVBO);
    glBufferData(GL_ARRAY_BUFFER, batchWaterVertices.size() * sizeof(Vertex), batchWaterVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    batch.waterVertexCount = (int)batchWaterVertices.size();
    
    // Upload Tree Instances by Archetype (0=Oak, 1=Pine, 2=Birch, 3=Willow)
    std::vector<glm::vec4> batchTrees[4];
    for (int z = 0; z < B_SIZE; z++) {
        for (int x = 0; x < B_SIZE; x++) {
            int cx = startCX + x;
            int cz = startCZ + z;
            auto it = m_chunks.find({cx, cz});
            if (it != m_chunks.end()) {
                for (const auto& t : it->second.treePositions) {
                    float treeHash = sin(t.x * 37.19f + t.z * 91.43f) * 43758.5453f;
                    int arch = std::abs((int)(treeHash * 100.0f)) % 4;
                    batchTrees[arch].push_back(t);
                }
            }
        }
    }
    
    for (int i = 0; i < 4; ++i) {
        if (batch.treeInstanceVBO[i] == 0) glGenBuffers(1, &batch.treeInstanceVBO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, batch.treeInstanceVBO[i]);
        if (!batchTrees[i].empty()) {
            glBufferData(GL_ARRAY_BUFFER, batchTrees[i].size() * sizeof(glm::vec4), batchTrees[i].data(), GL_STATIC_DRAW);
        } else {
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        }
        batch.treeCount[i] = (int)batchTrees[i].size();
    }

    batch.dirty = false;
    
    // SAFETY: Desvincular para no romper nada
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ChunkManager::RenderTerrain(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
    
    // --- SAFETY FIX: RESETEAR ESTADOS DEL SHADER ---
    // Si el BirdSystem o los Arboles corrieron antes, estas variables pueden tener basura.
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0); 
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);

    // Use SORTED visible batches
    for (RenderBatch* bPtr : m_visibleBatches) {
        RenderBatch& b = *bPtr;
        // Visibility already checked
        if (b.vertexCount > 0) {
            glBindVertexArray(b.VAO);
            glDrawArrays(GL_TRIANGLES, 0, b.vertexCount);
        }
    }
    glBindVertexArray(0);
}

void ChunkManager::RenderWater(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
    
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); 
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f); 

    for (RenderBatch* bPtr : m_visibleBatches) {
        RenderBatch& b = *bPtr;
        if (b.waterVertexCount > 0) {
            glBindVertexArray(b.waterVAO);
            glDrawArrays(GL_TRIANGLES, 0, b.waterVertexCount);
        }
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void ChunkManager::RenderTrees(GLuint shaderProgram, const GLuint trunkVAO[4], const GLuint leavesVAO[4], const int trunkVertexCount[4], const int leavesVertexCount[4], glm::vec3 playerPos) {
    // 1. Identify if the player is inside any tree foliage
    glm::vec4 playerTree(0.0f);
    bool hasPlayerTree = false;
    int playerTreeArch = 0;
    
    std::vector<glm::vec4> nearbyTrees;
    GetTreesInRange(playerPos, 15.0f, nearbyTrees);
    for (const auto& t : nearbyTrees) {
        float dist2D = glm::distance(glm::vec2(playerPos.x, playerPos.z), glm::vec2(t.x, t.z));
        float leavesBase = t.y + 3.0f * t.w;
        float leavesTop = t.y + 25.0f * t.w;
        float leavesRadius = 3.8f * t.w;
        if (dist2D < leavesRadius && playerPos.y >= leavesBase && playerPos.y <= leavesTop) {
            playerTree = t;
            hasPlayerTree = true;
            float treeHash = sin(t.x * 37.19f + t.z * 91.43f) * 43758.5453f;
            playerTreeArch = std::abs((int)(treeHash * 100.0f)) % 4;
            break;
        }
    }

    // Cache Uniforms
    GLint instancedLoc   = glGetUniformLocation(shaderProgram, "u_IsInstanced");
    GLint conformLoc     = glGetUniformLocation(shaderProgram, "u_ConformToTerrain");
    GLint birdAttribsLoc = glGetUniformLocation(shaderProgram, "u_UseBirdAttribs");
    GLint windStrLoc     = glGetUniformLocation(shaderProgram, "u_WindStrength");
    
    GLint isPlayerTreePassLoc = glGetUniformLocation(shaderProgram, "u_IsPlayerTreePass");
    GLint playerTreeDataLoc   = glGetUniformLocation(shaderProgram, "u_PlayerTreeData");

    glUniform1f(glGetUniformLocation(shaderProgram, "u_LodDistNear"), Config::Trees::WindLodNear);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_LodDistFar"), Config::Trees::WindLodFar);

    glUniform1i(instancedLoc, 1);
    glUniform1i(conformLoc, 0);
    glUniform1i(birdAttribsLoc, 0);
    
    if (hasPlayerTree) {
        glUniform4f(playerTreeDataLoc, playerTree.x, playerTree.y, playerTree.z, playerTree.w);
    } else {
        glUniform4f(playerTreeDataLoc, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    glUniform1i(isPlayerTreePassLoc, 0);

    GLuint lastInstanceVBO = 0;

    for (RenderBatch* bPtr : m_visibleBatches) {
        RenderBatch& b = *bPtr;
        
        for (int arch = 0; arch < 4; ++arch) {
            if (b.treeCount[arch] == 0) continue;
            
            lastInstanceVBO = b.treeInstanceVBO[arch];

            // --- PASS A: TRUNKS ---
            glUniform1f(windStrLoc, 0.0f);
            glBindVertexArray(trunkVAO[arch]);
            glBindBuffer(GL_ARRAY_BUFFER, b.treeInstanceVBO[arch]);
            
            glDisableVertexAttribArray(6);
            glDisableVertexAttribArray(7);
            
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glVertexAttribDivisor(4, 1);
            glDisableVertexAttribArray(5);
            
            glDrawArraysInstanced(GL_TRIANGLES, 0, trunkVertexCount[arch], b.treeCount[arch]);

            // --- PASS B: LEAVES ---
            glUniform1f(windStrLoc, 1.0f);
            glBindVertexArray(leavesVAO[arch]);
            glBindBuffer(GL_ARRAY_BUFFER, b.treeInstanceVBO[arch]);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glVertexAttribDivisor(4, 1);
            
            glDisableVertexAttribArray(5);
            glDisableVertexAttribArray(6);
            glDisableVertexAttribArray(7);

            glDrawArraysInstanced(GL_TRIANGLES, 0, leavesVertexCount[arch], b.treeCount[arch]);
            
            glDisableVertexAttribArray(4);
        }
    }

    // --- PASS C: PLAYER TREE LEAVES (TRANSLUCENT PASS) ---
    if (hasPlayerTree && lastInstanceVBO != 0) {
        glUniform1i(isPlayerTreePassLoc, 1);
        glUniform1f(windStrLoc, 1.0f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glBindVertexArray(leavesVAO[playerTreeArch]);
        glBindBuffer(GL_ARRAY_BUFFER, lastInstanceVBO);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glVertexAttribDivisor(4, 1);

        glDisableVertexAttribArray(5);
        glDisableVertexAttribArray(6);
        glDisableVertexAttribArray(7);

        glDrawArraysInstanced(GL_TRIANGLES, 0, leavesVertexCount[playerTreeArch], 1);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisableVertexAttribArray(4);
    }
    
    // Reset States
    glUniform1i(instancedLoc, 0);
    glUniform1f(windStrLoc, 0.0f);
    glUniform1i(isPlayerTreePassLoc, 0);
    glUniform4f(playerTreeDataLoc, 0.0f, 0.0f, 0.0f, 0.0f);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ChunkManager::GetTreesInRange(glm::vec3 pos, float range, std::vector<glm::vec4>& outTrees) {
    float worldChunkSize = m_chunkSize * m_scale;
    int minCX = (int)floor((pos.x - range - 5.0f) / worldChunkSize);
    int maxCX = (int)floor((pos.x + range + 5.0f) / worldChunkSize);
    int minCZ = (int)floor((pos.z - range - 5.0f) / worldChunkSize);
    int maxCZ = (int)floor((pos.z + range + 5.0f) / worldChunkSize);

    for (int z = minCZ; z <= maxCZ; z++) {
        for (int x = minCX; x <= maxCX; x++) {
            auto it = m_chunks.find({x, z});
            if (it == m_chunks.end()) continue;

            const Chunk& chunk = it->second;
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

void ChunkManager::RenderDebug(GLuint shaderProgram, glm::vec3 playerPos) {
    std::cout << "RenderDebug() Called" << std::endl;
    std::vector<float> lines;
    
    // Helper to add a box
    auto addBox = [&](glm::vec3 minB, glm::vec3 maxB, glm::vec3 color) {
        // Bottom Loop
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        // Top Loop
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        // Verticals
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);

        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(minB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(maxB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(maxB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        
        lines.push_back(minB.x); lines.push_back(minB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
        lines.push_back(minB.x); lines.push_back(maxB.y); lines.push_back(maxB.z); lines.push_back(color.r); lines.push_back(color.g); lines.push_back(color.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
    };

    // Iterate all chunks (Optimization: only visible batches?)
    // Debug draw can be heavy, let's just draw all visible chunks.
    
    float radiusSq = 40.0f * 40.0f; // 40 units radius
    
    // Use Cache Iteration
    for (RenderBatch* bPtr : m_batchList) {
        RenderBatch& b = *bPtr;
        if (!b.visible) continue;
        
        // This batch is visible, draw trees in its chunks
        int B_SIZE = Config::World::RenderBatchSize;
        int startCX = b.bx * B_SIZE;
        int startCZ = b.bz * B_SIZE;
        
        for (int z = 0; z < B_SIZE; z++) {
            for (int x = 0; x < B_SIZE; x++) {
                int cx = startCX + x;
                int cz = startCZ + z;
                
                auto it = m_chunks.find({cx, cz});
                if (it != m_chunks.end()) {
                    const Chunk& chunk = it->second;
                    for (const auto& treeData : chunk.treePositions) {
                        glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
                        float treeScale = treeData.w;
                        
                        // RADIUS CHECK
                        glm::vec3 diff = playerPos - treePos;
                        if (glm::dot(diff, diff) > radiusSq) continue;

                        // 1. TRUNK
                        float halfW = 0.6f * treeScale;
                        glm::vec3 minB = treePos - glm::vec3(halfW, 0, halfW); 
                        glm::vec3 maxB = treePos + glm::vec3(halfW, 6.0f*treeScale, halfW);
                        minB.y = treePos.y; 
                        addBox(minB, maxB, glm::vec3(0.8f, 0.4f, 0.1f)); // Brown/Orange
                        
                        // 2. LEAVES (Pyramid)
                        float leavesW = 3.0f * treeScale; 
                        float leavesBase = treePos.y + 6.0f * treeScale;
                        float leavesH = 19.0f * treeScale;
                        glm::vec3 apex(treePos.x, leavesBase + leavesH, treePos.z);
                        
                        // Base Corners
                        glm::vec3 c1 = treePos + glm::vec3(-leavesW, 0, -leavesW); c1.y = leavesBase;
                        glm::vec3 c2 = treePos + glm::vec3(leavesW, 0, -leavesW);  c2.y = leavesBase;
                        glm::vec3 c3 = treePos + glm::vec3(leavesW, 0, leavesW);   c3.y = leavesBase;
                        glm::vec3 c4 = treePos + glm::vec3(-leavesW, 0, leavesW);  c4.y = leavesBase;
                        
                        glm::vec3 col = glm::vec3(0.2f, 1.0f, 0.2f); // Green
                        
                        // Base Loop
                        auto addLine = [&](glm::vec3 a, glm::vec3 b) {
                             lines.push_back(a.x); lines.push_back(a.y); lines.push_back(a.z); lines.push_back(col.r); lines.push_back(col.g); lines.push_back(col.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
                             lines.push_back(b.x); lines.push_back(b.y); lines.push_back(b.z); lines.push_back(col.r); lines.push_back(col.g); lines.push_back(col.b); lines.push_back(0); lines.push_back(1); lines.push_back(0);
                        };
                        
                        addLine(c1, c2);
                        addLine(c2, c3);
                        addLine(c3, c4);
                        addLine(c4, c1);
                        
                        // To Apex
                        addLine(c1, apex);
                        addLine(c2, apex);
                        addLine(c3, apex);
                        addLine(c4, apex);
                    }
                }
            }
        }
    }
    
    std::cout << "DebugRender: " << lines.size()/9 << " lines" << std::endl; 
    
    if (lines.empty()) return;
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STREAM_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0); // Pos
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // Col
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    
    glDisable(GL_DEPTH_TEST);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 1); // Bypass lighting
    
    glDrawArrays(GL_LINES, 0, (GLsizei)(lines.size() / 9));
    
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);
    glEnable(GL_DEPTH_TEST);
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
