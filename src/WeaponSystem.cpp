#include "WeaponSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

WeaponSystem::WeaponSystem() : currentAmmo(2), maxAmmo(2), recoilTimer(0.0f), cooldownTimer(0.0f) {
    BuildShotgunMesh();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, gunVertices.size() * sizeof(float), gunVertices.data(), GL_STATIC_DRAW);

    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(3);
}

WeaponSystem::~WeaponSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void WeaponSystem::BuildShotgunMesh() {
    // Simple Double Barrel: 2 Cylinders (Hexagons) + 1 Box Stock
    auto addQuad = [&](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 color) {
         glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
         // Tri 1
         gunVertices.push_back(p1.x); gunVertices.push_back(p1.y); gunVertices.push_back(p1.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);

         gunVertices.push_back(p2.x); gunVertices.push_back(p2.y); gunVertices.push_back(p2.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);

         gunVertices.push_back(p3.x); gunVertices.push_back(p3.y); gunVertices.push_back(p3.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);

         // Tri 2
         gunVertices.push_back(p1.x); gunVertices.push_back(p1.y); gunVertices.push_back(p1.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);

         gunVertices.push_back(p3.x); gunVertices.push_back(p3.y); gunVertices.push_back(p3.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);

         gunVertices.push_back(p4.x); gunVertices.push_back(p4.y); gunVertices.push_back(p4.z); 
         gunVertices.push_back(color.r); gunVertices.push_back(color.g); gunVertices.push_back(color.b);
         gunVertices.push_back(normal.x); gunVertices.push_back(normal.y); gunVertices.push_back(normal.z);
    };

    glm::vec3 metal(0.15f, 0.15f, 0.18f); // Darker steel
    glm::vec3 wood(0.35f, 0.18f, 0.08f);  // Warm walnut wood

    // BARRELS
    float L = 1.2f;
    float W = 0.05f;
    
    // Left Barrel
    float offX = -0.055f;
    addQuad({offX-W, W, 0}, {offX+W, W, 0}, {offX+W, W, -L}, {offX-W, W, -L}, metal); // Top
    addQuad({offX+W, W, 0}, {offX+W, -W, 0}, {offX+W, -W, -L}, {offX+W, W, -L}, metal); // Right
    addQuad({offX-W, -W, 0}, {offX-W, W, 0}, {offX-W, W, -L}, {offX-W, -W, -L}, metal); // Left
    addQuad({offX-W, -W, 0}, {offX+W, -W, 0}, {offX+W, -W, -L}, {offX-W, -W, -L}, metal); // Bottom

    // Right Barrel
    offX = 0.055f;
    addQuad({offX-W, W, 0}, {offX+W, W, 0}, {offX+W, W, -L}, {offX-W, W, -L}, metal);
    addQuad({offX+W, W, 0}, {offX+W, -W, 0}, {offX+W, -W, -L}, {offX+W, W, -L}, metal);
    addQuad({offX-W, -W, 0}, {offX-W, W, 0}, {offX-W, W, -L}, {offX-W, -W, -L}, metal);
    addQuad({offX-W, -W, 0}, {offX+W, -W, 0}, {offX+W, -W, -L}, {offX-W, -W, -L}, metal);

    // CONNECTOR (Upper part of barrels)
    addQuad({-0.12f, 0.04f, -0.2f}, {0.12f, 0.04f, -0.2f}, {0.12f, 0.04f, -0.6f}, {-0.12f, 0.04f, -0.6f}, metal);

    // STOCK (Culata) - Tapered wooden piece
    // Starting after the barrels
    float sL = 0.6f; // Length of stock
    float sW1 = 0.12f; // Width at front (connecting to barrels)
    float sW2 = 0.20f; // Width at back (buttplate)
    float sH1 = 0.10f; // Height at front
    float sH2 = 0.25f; // Height at back
    float sZ = 0.0f;   // Starts at origin (relative to cam)
    float sZB = 0.6f;  // Goes backwards

    // Top
    addQuad({-sW1, sH1, sZ}, {sW1, sH1, sZ}, {sW2, sH2, sZB}, {-sW2, sH2, sZB}, wood);
    // Bottom
    addQuad({-sW1, -sH1, sZ}, {sW2, -sH2, sZB}, {sW2, -sH2, sZB}, {-sW1, -sH1, sZ}, wood); // Simplified
    addQuad({-sW1, -sH1, sZ}, {sW1, -sH1, sZ}, {sW2, -sH2, sZB}, {-sW2, -sH2, sZB}, wood);
    // Left
    addQuad({-sW1, sH1, sZ}, {-sW2, sH2, sZB}, {-sW2, -sH2, sZB}, {-sW1, -sH1, sZ}, wood);
    // Right
    addQuad({sW1, sH1, sZ}, {sW1, -sH1, sZ}, {sW2, -sH2, sZB}, {sW2, sH2, sZB}, wood);
    // Back (Buttplate)
    addQuad({-sW2, sH2, sZB}, {sW2, sH2, sZB}, {sW2, -sH2, sZB}, {-sW2, -sH2, sZB}, wood);
}

void WeaponSystem::Update(float deltaTime) {
    if (recoilTimer > 0.0f) recoilTimer -= deltaTime * 5.0f; // Recovery speed
    if (recoilTimer < 0.0f) recoilTimer = 0.0f;
    
    if (cooldownTimer > 0.0f) cooldownTimer -= deltaTime;
}

void WeaponSystem::TryFire(glm::vec3 camPos, glm::vec3 camDir, ParticleSystem& particles, FootprintSystem& craters, ChunkManager& chunkManager) {
    if (cooldownTimer > 0.0f) return;
    
    cooldownTimer = 0.4f; // Slightly faster fire rate
    recoilTimer = 1.0f;   // Kick back

    // 1. Muzzle Flash (Small, bright)
    glm::vec3 muzzlePos = camPos + camDir * 1.5f + glm::vec3(0.08f, -0.15f, 0.0f);
    particles.SpawnParticle(muzzlePos, glm::vec3(0,0,0), glm::vec4(1.0f, 0.9f, 0.3f, 1.0f), 0.12f, 0.04f, 0.0f);
    
    // Spawn Smoke (Whiter and very transparent: alpha 0.05)
    for(int i=0; i<3; i++) {
        glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, 1.2f, (rand()%100)/100.0f - 0.5f) * 0.4f;
        particles.SpawnParticle(muzzlePos, rndVel, glm::vec4(0.98f, 0.98f, 0.98f, 0.05f), 0.4f, 1.0f, 0.3f);
    }

    // 2. Raycast Impact (Terrain + Trees)
    glm::vec3 dir = glm::normalize(camDir);
    glm::vec3 hitPoint;
    glm::vec4 hitColor(1.0f);
    bool hit = false;
    bool hitTree = false;

    // We check for trees first as they are "raised" objects
    std::vector<glm::vec4> nearbyTrees;
    chunkManager.GetTreesInRange(camPos, 50.0f, nearbyTrees);

    for(int i=0; i<100; i++) { 
        glm::vec3 ray = camPos + dir * (i * 0.5f);
        
        // A. Check Trees (AABB Collision for Square Trunks)
        for(const auto& treeData : nearbyTrees) {
            glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
            float treeScale = treeData.w;
            float halfW = 0.6f * treeScale; // Half-width matches visual mesh
            float trunkTop = treePos.y + 10.0f * treeScale;

            // Check Height first
            if (ray.y < treePos.y || ray.y > trunkTop) continue;

            // Check AABB (Axis Aligned Box)
            float minX = treePos.x - halfW;
            float maxX = treePos.x + halfW;
            float minZ = treePos.z - halfW;
            float maxZ = treePos.z + halfW;

            if (ray.x >= minX && ray.x <= maxX && ray.z >= minZ && ray.z <= maxZ) {
                hit = true;
                hitTree = true;
                
                // Snap to Closest Face (with bias)
                // Determine distances to each face
                float dLeft   = abs(ray.x - minX);
                float dRight  = abs(ray.x - maxX);
                float dBack   = abs(ray.z - minZ);
                float dFront  = abs(ray.z - maxZ);
                
                // Find minimum distance
                float minD = std::min({dLeft, dRight, dBack, dFront});
                float bias = 0.08f; 

                hitPoint = ray;
                if (minD == dLeft)       hitPoint.x = minX - bias;
                else if (minD == dRight) hitPoint.x = maxX + bias;
                else if (minD == dBack)  hitPoint.z = minZ - bias;
                else                     hitPoint.z = maxZ + bias; // Front

                hitColor = glm::vec4(0.25f, 0.15f, 0.05f, 1.0f); // Dark Wood Color
                break;
            }
        }
        if(hit) break;

        // B. Check Terrain
        float groundY = WorldGenerator::GetHeight(ray.x, ray.z);
        if(ray.y < groundY) {
            hitPoint = ray;
            hitPoint.y = groundY; 
            hit = true;
            glm::vec3 gColor = WorldGenerator::GetTerrainColor(ray.x, ray.z, groundY);
            hitColor = glm::vec4(gColor, 1.0f);
            break;
        }
    }
    
    if (hit) {
        // Spawn Crater (Only on ground)
        if(!hitTree) craters.AddFootprint(hitPoint, (float)(rand() % 360));
        
        // Spawn Debris (Smaller: size 0.08, matched color)
        int debrisCount = hitTree ? 15 : 8; // More shards from wood
        for(int i=0; i<debrisCount; i++) {
             glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, 3.0f + (rand()%100)/50.0f, (rand()%100)/100.0f - 0.5f);
             if(hitTree) rndVel *= 1.4f; // Faster explosion from tree
             particles.SpawnParticle(hitPoint, rndVel, hitColor, 0.08f, 0.6f, -9.8f);
        }
    }
}

void WeaponSystem::Render(GLuint shaderProgram) {
    // Draw Gun Model in Screen Space (or overlay)
    // We attach it to the camera.
    // In main loop, View Matrix is already set. We just need to Model Matrix it relative to camera.
    // Wait, typically HUD weapons are drawn with Identity View Matrix (Local Space).
    // Let's do that.
    
    // We assume View/Projection are set to "UI Mode" or we manually set them?
    // main.cpp sets View/Projection for world.
    
    // Better: Draw with a specialized "Weapon View" matrix which is static (Identity at 0,0,0) so it doesn't move with world,
    // but we simulate bobbing.
    // Actually, simple Hack: Draw it at the end, clear depth, use Identity View.
    // The "Bob" is handled by the Player's view matrix usually, but if we clear View, we lose bob.
    // Let's just draw it relative to Camera Position/Rotation in World Space?
    // No, that clips into walls.
    
    // Standard Way: Clear Depth. Set View = Identity. Set Model = Translation/Recoil.
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "u_View");
    
    // Save current view
    // glm::mat4 oldView; // Can't easily get back from GPU without uniform sync.
    // We will handle this in main.cpp by modifying the Render order.
    // Here we just draw.
    
    float recoilOffset = sin(recoilTimer * 3.14f) * 0.2f; // Back and up
    
    glm::mat4 model = glm::mat4(1.0f);
    // Position on screen (Right hand side, slightly down)
    model = glm::translate(model, glm::vec3(0.2f, -0.2f, -0.5f + recoilOffset)); 
    // Aim slightly up
    model = glm::rotate(model, glm::radians(5.0f - recoilTimer * 10.0f), glm::vec3(1,0,0));
    model = glm::scale(model, glm::vec3(0.5f)); 

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    // For View, caller should have set Identity.
    
    glBindVertexArray(VAO);
    // Override color uniform if needed, or rely on vertex attributes
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(gunVertices.size()/9));
}
