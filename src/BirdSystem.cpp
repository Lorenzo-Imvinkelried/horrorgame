#include "BirdSystem.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BirdSystem::BirdSystem() {
    // std::cout << "[BirdSystem] Initializing..." << std::endl;
    // Load Model
    std::string path = "assets/models/gorrion.txt";
    m_basePose = ModelLoader::Load(path);
    if (m_basePose.empty()) {
        std::cerr << "[BirdSystem] Failed to load " << path << std::endl;
    } else {
        // std::cout << "[BirdSystem] Loaded model with " << m_basePose.size() << " boxes." << std::endl;
    }
    
    BuildMesh();
}

void BirdSystem::BuildMesh() {
    m_meshVertices.clear();
    ModelLoader::GenerateMesh(m_basePose, m_meshVertices);
    // std::cout << "[BirdSystem] Mesh built. Vertices: " << m_meshVertices.size() << std::endl;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_meshVertices.size() * sizeof(float), m_meshVertices.data(), GL_STATIC_DRAW);

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
}

void BirdSystem::TrySpawnBirds(const std::vector<glm::vec4>& treeData) {
    if (treeData.empty()) return;
    // std::cout << "[BirdSystem] Checking " << treeData.size() << " trees..." << std::endl;
    int added = 0;
    for (const auto& t : treeData) {
        glm::vec3 treePos(t.x, t.y, t.z);
        float scale = t.w;
        
        Perch p;
        p.pos = treePos + glm::vec3(0, 5.0f * scale, 0); // Top of tree
        
        // Roll for birds (5% chance)
        // FORCE HIGHER CHANCE momentarily for debug if needed, but 0.05 is fine if we have many trees
        float r = (float)(rand() % 100) / 100.0f;
        p.hasBirds = (r < Config::Bird::SpawnChance);
        
        if (p.hasBirds) added++;
        
        m_perches.push_back(p);
    }
    // if(added > 0) std::cout << "[BirdSystem] Spawned " << added << " flocks." << std::endl;
}

void BirdSystem::SpawnFlock(glm::vec3 startPos) {
    int count = 2 + (rand() % 4); // 2 to 5 birds
    
    // Pick a distant target
    glm::vec3 targetPos = startPos + glm::vec3(0, 40, 0); // Default: Fly straight UP
    
    // Try to find a real distant perch
    if (!m_perches.empty()) {
        int attempts = 0;
        while(attempts < 10) {
            int idx = rand() % m_perches.size();
            float d = glm::distance(startPos, m_perches[idx].pos);
            if (d > 30.0f) { // Fly far away
                targetPos = m_perches[idx].pos; 
                // CRITICAL FIX: Add significant height offset to the destination so they arc UP
                // irrespective of whether the next tree is higher or lower.
                // We want them to fly INTO THE SKY, not just to the next tree immediately.
                targetPos.y = std::max(targetPos.y, startPos.y + 20.0f);
                break;
            }
            attempts++;
        }
    }
    
    // Spawn birds
    for (int i=0; i<count; i++) {
        FlyingBird b;
        b.pos = startPos + glm::vec3((rand()%100)/100.0f, (rand()%100)/100.0f, (rand()%100)/100.0f); // Scatter start
        b.startPos = b.pos;
        b.targetPos = targetPos + glm::vec3((rand()%200)/50.0f, (rand()%200)/50.0f, (rand()%200)/50.0f); // Scatter target
        b.flapTimer = (float)(rand() % 100);
        
        // Calculate Flight Params
        float dist = glm::distance(b.startPos, b.targetPos);
        b.speed = Config::Bird::FlySpeed * (0.8f + (rand()%50)/100.0f); 
        b.flightDuration = dist / b.speed;
        b.elapsedTime = 0.0f;
        
        // Fixed: Ensure ALL birds have a huge arc
        // Was 15-35, now 30-50. This ensures visible "up and down" parabola.
        b.arcHeight = 30.0f + (rand() % 20); 
        
        b.yaw = 0.0f;
        
        m_activeBirds.push_back(b);
    }
}

void BirdSystem::Update(float deltaTime, glm::vec3 playerPos, glm::vec3 monsterPos) {
    // 1. Check Triggers on Perches
    for (auto& p : m_perches) {
        if (p.hasBirds) {
            float dPlayer = glm::distance(p.pos, playerPos);
            float dMonster = glm::distance(p.pos, monsterPos);
            
            if (dPlayer < Config::Bird::TriggerDistance || dMonster < Config::Bird::TriggerDistance) {
                p.hasBirds = false; // Empty the tree
                SpawnFlock(p.pos);
            }
        }
    }
    
    // 2. Update Flying Birds
    for (auto it = m_activeBirds.begin(); it != m_activeBirds.end();) {
        it->elapsedTime += deltaTime;
        float t = it->elapsedTime / it->flightDuration;
        
        if (t >= 1.0f) {
            // Arrived
            it = m_activeBirds.erase(it);
        } else {
            // Parabolic Flight
            
            // 1. Linear Base
            glm::vec3 currentPos = glm::mix(it->startPos, it->targetPos, t);
            
            // 2. Add Arc (Sin Start -> End)
            // sin(0) = 0, sin(PI/2) = 1, sin(PI) = 0
            float heightOffset = it->arcHeight * sin(t * 3.14159f);
            currentPos.y += heightOffset;
            
            // Calculate Yaw (Direction)
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

void BirdSystem::Render(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glBindVertexArray(VAO);
    
    // Draw Flying Birds
    for (const auto& b : m_activeBirds) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, b.pos);
        model = glm::rotate(model, glm::radians(b.yaw), glm::vec3(0,1,0));
        
        // Flap
        float flap = sin(b.flapTimer);
        model = glm::scale(model, glm::vec3(1.0f, 1.0f + flap*0.2f, 1.0f)); 
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/11));
    }
    
    // Draw Debug Rings if enabled
    if (m_showDebug) {
        if (m_perches.empty()) {
             // Only print once per second to avoid spam, or just once
             static bool warned = false;
             // if(!warned) { std::cout << "[BirdSystem] Debug ON but NO perches found!" << std::endl; warned = true; }
        }
        for (const auto& p : m_perches) {
            if (p.hasBirds) {
                // Ring around tree
                RenderDebugRing(p.pos, glm::vec3(1, 1, 0), shaderProgram); // Yellow ring
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
