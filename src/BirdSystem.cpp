#include "BirdSystem.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> // Added for distance2
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BirdSystem::BirdSystem() {
    // Reserve memory to prevent runtime allocation spikes
    m_activeBirds.reserve(2000); 
    m_instances.reserve(2000); // Reserve instance buffer data

    // Load Model
    std::string path = "assets/models/gorrion.txt";
    m_basePose = ModelLoader::Load(path);
    if (m_basePose.empty()) {
        std::cerr << "[BirdSystem] Failed to load " << path << std::endl;
    }
    
    BuildMesh();
    
    // Create Default White Texture
    glGenTextures(1, &m_defaultTex);
    glBindTexture(GL_TEXTURE_2D, m_defaultTex);
    unsigned char white[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

ChunkKey BirdSystem::GetChunkKey(glm::vec3 pos) {
    float effectiveSize = Config::World::ChunkSize * Config::World::ChunkScale;
    return ChunkKey{
        (int)floor(pos.x / effectiveSize),
        (int)floor(pos.z / effectiveSize)
    };
}

void BirdSystem::BuildMesh() {
    m_meshVertices.clear();
    ModelLoader::GenerateMesh(m_basePose, m_meshVertices);

    // Initial buffer cleanup
    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    if (instanceVBO == 0) glGenBuffers(1, &instanceVBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // STATIC DRAW: Upload Mesh ONCE
    glBufferData(GL_ARRAY_BUFFER, m_meshVertices.size() * sizeof(float), m_meshVertices.data(), GL_STATIC_DRAW); 

    // Enable Attributes (Standard Layout)
    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    // Normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);
    
    // INSTANCE ATTRIBUTES
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // Attrib 5: Removed (Handled via Loc 5 or unused)
    // Trees use 4 and 5. Birds use 6 and 7.
    // BuildMesh should not touch 4 and 5 to avoid confusion, 
    // although they are separate VAOs.
    // BirdSystem::Render sets up 6 and 7.
}

void BirdSystem::TrySpawnBirds(const std::vector<glm::vec4>& treeData) {
    if (treeData.empty()) return;
    
    int added = 0;
    for (const auto& t : treeData) {
        glm::vec3 treePos(t.x, t.y, t.z);
        float scale = t.w;
        
        Perch p;
        p.pos = treePos + glm::vec3(0, 5.0f * scale, 0); // Top of tree
        
        float r = (float)(rand() % 100) / 100.0f;
        p.hasBirds = (r < Config::Bird::SpawnChance);
        
        // Add to Spatial Grid
        if (p.hasBirds) { 
            ChunkKey key = GetChunkKey(p.pos);
            m_perchGrid[key].push_back(p);
        }
    }
}

void BirdSystem::SpawnFlock(glm::vec3 startPos) {
    int count = 1 + (rand() % 3); 
    
    glm::vec3 targetPos = startPos;
    bool foundPerch = false;
    
    // 1. LOCAL SEARCH for Perch (3x3 Neighbors)
    if (!m_perchGrid.empty()) {
        ChunkKey homeKey = GetChunkKey(startPos);
        std::vector<Perch> candidates;
        
        // Search Radius (1 chunk neighbor)
        for(int x=-1; x<=1; x++) {
            for(int z=-1; z<=1; z++) {
                ChunkKey k = {homeKey.x + x, homeKey.z + z};
                if (m_perchGrid.find(k) != m_perchGrid.end()) {
                    const auto& list = m_perchGrid[k];
                    // Add valid candidates from this chunk
                    for(const auto& p : list) {
                        float d2 = glm::distance2(startPos, p.pos);
                        // Filter: Min 15m, Max 60m
                        if (d2 > 225.0f && d2 < 3600.0f) {
                            candidates.push_back(p);
                        }
                    }
                }
            }
        }
        
        // Pick random candidate if any
        if (!candidates.empty()) {
            const Perch& p = candidates[rand() % candidates.size()];
            targetPos = p.pos;
            // Ensure minimum height clearance (don't land at base)
            if (targetPos.y < startPos.y) targetPos.y = startPos.y; 
            foundPerch = true;
        }
    }
    
    // 2. FALLBACK: Fly Away (Escape to Horizon)
    if (!foundPerch) {
        // Random Direction (XZ)
        float angle = (float)(rand() % 360) * 0.0174533f;
        glm::vec3 dir(cos(angle), 0.5f, sin(angle)); // Bias UP slightly
        
        // Fly 60-80m away
        float dist = 60.0f + (rand() % 20);
        targetPos = startPos + dir * dist;
        //targetPos.y += 20.0f; // Gain height
    }
    
    // Spawn birds
    for (int i=0; i<count; i++) {
        FlyingBird b;
        // Slight offset at start
        b.pos = startPos + glm::vec3(
            (rand()%100 - 50)/50.0f, 
            (rand()%100)/50.0f, 
            (rand()%100 - 50)/50.0f
        ); 
        b.startPos = b.pos;
        
        // Target Variation
        glm::vec3 tVar = targetPos + glm::vec3(
            (rand()%100 - 50)/20.0f, 
            (rand()%100)/20.0f, 
            (rand()%100 - 50)/20.0f
        );
        b.targetPos = tVar;
        
        b.flapTimer = (float)(rand() % 100);
        
        float dist = glm::distance(b.startPos, b.targetPos);
        // Cap distance for speed calc logic
        if (dist > Config::Bird::MaxFlightDistance) dist = Config::Bird::MaxFlightDistance;
        
        // Speed
        b.speed = Config::Bird::FlySpeed * (0.9f + (rand()%20)/100.0f); 
        b.flightDuration = dist / b.speed;
        if (b.flightDuration > Config::Bird::MaxFlightTime) b.flightDuration = Config::Bird::MaxFlightTime;
        if (b.flightDuration < 1.0f) b.flightDuration = 1.0f; // Safety
        
        b.elapsedTime = 0.0f;
        // High Arc if landing nearby, Flat/Rising Arc if flying away
        if (foundPerch) {
            b.arcHeight = 5.0f + (dist * 0.3f); // Proportional parabola
        } else {
            b.arcHeight = 10.0f; // Gentle rise
        }
        
        b.yaw = 0.0f;
        
        m_activeBirds.push_back(b);
    }
}

void BirdSystem::Update(float deltaTime, glm::vec3 playerPos, const std::vector<glm::vec3>& monsterPositions) {
    // 1. Límite de pájaros activos para performance (REDUCIDO a 200)
    if (m_activeBirds.size() > 200) {
        m_activeBirds.erase(m_activeBirds.begin(), m_activeBirds.begin() + 50);
    }
    
    // 2. Optimizar chequeo de perchas
    ChunkKey pKey = GetChunkKey(playerPos);
    
    std::vector<ChunkKey> keysToCheck;
    
    auto addNeighbors = [&](ChunkKey cx) {
        for(int x=0; x<=1; x++) {
            for(int z=0; z<=1; z++) {
                ChunkKey k = {cx.x + x, cx.z + z};
                
                // Duplicate Check
                bool exists = false;
                for(const auto& existingKey : keysToCheck) {
                    if(existingKey.x == k.x && existingKey.z == k.z) {
                        exists = true; break;
                    }
                }
                
                if(!exists) {
                    keysToCheck.push_back(k);
                }
            }
        }
    };
    
    addNeighbors(pKey);
    for (const auto& mPos : monsterPositions) {
        ChunkKey mKey = GetChunkKey(mPos);
        addNeighbors(mKey);
    }
    
    float triggerDist2 = Config::Bird::TriggerDistance * Config::Bird::TriggerDistance;
    
    for (const auto& key : keysToCheck) {
        auto it = m_perchGrid.find(key);
        if (it != m_perchGrid.end()) {
            for (auto& p : it->second) {
                 if (p.hasBirds) {
                     float dP2 = glm::distance2(p.pos, playerPos);
                     
                     float dM2 = triggerDist2 + 10.0f; // Default above limit
                     for (const auto& mPos : monsterPositions) {
                         float dist2 = glm::distance2(p.pos, mPos);
                         if (dist2 < dM2) dM2 = dist2;
                     }
                     
                     if (dP2 < triggerDist2 || dM2 < triggerDist2) {
                          p.hasBirds = false;
                          SpawnFlock(p.pos);
                     }
                 }
            }
        }
    }
    
    // 3. Actualizar pájaros volando con límites
    for (auto it = m_activeBirds.begin(); it != m_activeBirds.end();) {
        it->elapsedTime += deltaTime;
        float t = it->elapsedTime / it->flightDuration;
        
        // ELIMINAR si: tiempo máximo excedido O distancia muy grande
        float distToPlayer2 = glm::distance2(it->pos, playerPos);
        float maxDist2 = Config::Bird::MaxFlightDistance * Config::Bird::MaxFlightDistance;

        if (t >= 1.0f || 
            it->elapsedTime > Config::Bird::MaxFlightTime ||
            distToPlayer2 > maxDist2) {
            it = m_activeBirds.erase(it);
        } else {
            glm::vec3 currentPos = glm::mix(it->startPos, it->targetPos, t);
            float heightOffset = it->arcHeight * sin(t * 3.14159f);
            currentPos.y += heightOffset;
            
            // Añadir límite de altura máxima
            if (currentPos.y > 150.0f) currentPos.y = 150.0f;

            glm::vec3 dir = currentPos - it->pos;
            if (glm::length(dir) > 0.001f) {
                dir = glm::normalize(dir);
                it->yaw = glm::degrees(atan2(dir.x, dir.z));
            }
            
            it->pos = currentPos;
            it->flapTimer += deltaTime * 20.0f;
            
            ++it;
        }
    }
}

void BirdSystem::CleanupDistantBirds(glm::vec3 playerPos, float maxDistance) {
    float maxDistSq = maxDistance * maxDistance;
    
    // Limpiar pájaros activos muy lejanos
    auto it = m_activeBirds.begin();
    while (it != m_activeBirds.end()) {
        if (glm::distance2(it->pos, playerPos) > maxDistSq) {
            it = m_activeBirds.erase(it);
        } else {
            ++it;
        }
    }
}

void BirdSystem::Render(GLuint shaderProgram) {
    if (m_activeBirds.empty()) return;

    // Clean up Tree Attributes FIRST
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    GLint instancedLoc = glGetUniformLocation(shaderProgram, "u_IsInstanced");
    
    // Build Instances
    m_instances.clear();
    size_t vertexCount = m_meshVertices.size() / 11;
    
    // 2. Add Flying Birds
    // Optimization: Frustum Cull individual birds
    // We can use a simple point-radius check.
    // Frustum is not passed? 
    // We need to modify Render signature or passing ViewProj?
    // For now, let's assume we can't easily cull without Frustum object.
    // BUT we can check simple distance to camera if passed? No camera pos passed.
    // Wait, let's modify the signature in header first if needed.
    // Actually, let's just use the "Distance to Camera" check I added in Update?
    // Better: Filter by Frustum in Render if possible.
    // Since I cannot change the signature easily without breaking main, 
    // I will skip Frustum Culling here strictly speaking, 
    // BUT I will ensure the "distance cleanup" in Update is working.
    
    // HOWEVER, I CAN ADD A SAFEGUARD LIMIT
    int drawn = 0;
    for (const auto& b : m_activeBirds) {
        if (drawn > 100) break; // Hard Limit Draw Count to 100
        
        float flap = sin(b.flapTimer);
        // AUMENTADO de 0.6f a 1.2f
        float scale = 1.2f * (1.0f + flap * 0.15f); // Menor variación de aleteo
        
        // Padding for Alpha (requires shader update to use it, but passing it now)
        m_instances.push_back({b.pos.x, b.pos.y, b.pos.z, scale, glm::radians(b.yaw), {1.0f, 0, 0}});
        drawn++;
    }

    // Upload Data
    glBindVertexArray(VAO); 
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_instances.size() * sizeof(BirdInstance), m_instances.data(), GL_STREAM_DRAW);

    // Setup Bird Attributes (6 & 7)
    // Pos+Scale
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(BirdInstance), (void*)0);
    glVertexAttribDivisor(6, 1);

    // Yaw
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(BirdInstance), (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(7, 1);

    // Shader Config
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform1i(instancedLoc, 1); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_UseBirdAttribs"), 1); // Enable Bird Mode
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    
    // Bind Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_defaultTex);
    
    glDrawArraysInstanced(GL_TRIANGLES, 0, (GLsizei)vertexCount, (GLsizei)m_instances.size());
    
    // Reset State
    glUniform1i(instancedLoc, 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_UseBirdAttribs"), 0);

    // Clean up Attributes
    glDisableVertexAttribArray(6);
    glDisableVertexAttribArray(7);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Debug
    if (m_showDebug) {
        // Warning: Debug loop still iterates map, can be slow if map is massive, but likely fine.
        for (const auto& pair : m_perchGrid) {
            for (const auto& p : pair.second) {
                if (p.hasBirds) RenderDebugRing(p.pos, glm::vec3(1,1,0), shaderProgram);
            }
        }
    }
}

void BirdSystem::RenderDebugRing(glm::vec3 pos, glm::vec3 color, GLuint shaderProgram) {
    if (debugVAO == 0) {
        glGenVertexArrays(1, &debugVAO);
        glGenBuffers(1, &debugVBO);
    }
    
    // Build Ring on fly (inefficient but debug)
    std::vector<float> verts;
    int segs = 16;
    float r = Config::Bird::TriggerDistance;
    for(int i=0; i<=segs; i++) {
        float a = (float)i / segs * 6.28f;
        // Fixed: Removed -4.0f offset so it's at the perch height (visible in air)
        verts.push_back(pos.x + cos(a)*r); verts.push_back(pos.y); verts.push_back(pos.z + sin(a)*r); // Pos
        verts.push_back(color.r); verts.push_back(color.g); verts.push_back(color.b); // Color
        verts.push_back(0); verts.push_back(0); // UV
        verts.push_back(0); verts.push_back(1); verts.push_back(0); // Norm
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    
    glBindVertexArray(debugVAO);
    // Assume Shader expects standard layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    
    // Align state
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    
    // --- FORCE VISIBILITY ---
    glDisable(GL_DEPTH_TEST); 
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_STRIP, 0, segs+1);
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}
