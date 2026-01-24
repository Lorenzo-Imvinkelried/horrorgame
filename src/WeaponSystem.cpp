#include "WeaponSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include "Config.h" // NEW

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
    float L = 1.5f;
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

// Helper for Ray-Box Intersection (Returns Entry and Exit times)
bool GetRayAABBIntersections(glm::vec3 origin, glm::vec3 dir, glm::vec3 minB, glm::vec3 maxB, float& tNear, float& tFar) {
     float t1 = (minB.x - origin.x)/dir.x;
     float t2 = (maxB.x - origin.x)/dir.x;
     float t3 = (minB.y - origin.y)/dir.y;
     float t4 = (maxB.y - origin.y)/dir.y;
     float t5 = (minB.z - origin.z)/dir.z;
     float t6 = (maxB.z - origin.z)/dir.z;

     tNear = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
     tFar = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

     if (tFar < 0) return false;
     if (tNear > tFar) return false;
     return true;
}

// Wrapper for old simpler API (Trunks)
bool RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 minB, glm::vec3 maxB, float maxDist, float& tScale) {
     float tNear, tFar;
     if (GetRayAABBIntersections(origin, dir, minB, maxB, tNear, tFar)) {
         if (tNear > maxDist) return false;
         tScale = tNear;
         return true;
     }
     return false;
}

void WeaponSystem::Update(float deltaTime, glm::vec2 windDir, float windStrength, ChunkManager& chunkManager, FootprintSystem& craters, ParticleSystem& particles) {
    if (recoilTimer > 0.0f) recoilTimer -= deltaTime * 5.0f; // Recovery speed
    if (recoilTimer < 0.0f) recoilTimer = 0.0f;
    
    if (cooldownTimer > 0.0f) cooldownTimer -= deltaTime;

    // PROJECTILE PHYSICS
    for (auto& p : projectiles) {
        if (!p.Active) continue;

        p.LifeTime -= deltaTime;
        if (p.LifeTime <= 0) {
            p.Active = false;
            continue;
        }

        // 1. Integration (Velocity Verlet / Euler)
        glm::vec3 oldPos = p.Position;
        
        // Forces (Using Exaggerated Config Values)
        glm::vec3 gravity(0, Config::Gameplay::ProjectileGravity, 0);
        glm::vec3 windForce = glm::vec3(windDir.x, 0, windDir.y) * windStrength * Config::Gameplay::ProjectileWindInfluence; 
        
        // Split Drag: 
        // Horizontal: Full Drag (slows down forward movement)
        // Vertical: Reduced Drag (allows falling fast like a heavy object, not a feather)
        glm::vec3 velHoriz(p.Velocity.x, 0, p.Velocity.z);
        glm::vec3 velVert(0, p.Velocity.y, 0);
        
        glm::vec3 drag = -velHoriz * Config::Gameplay::ProjectileDrag * deltaTime; 
        drag += -velVert * (Config::Gameplay::ProjectileDrag * 0.1f) * deltaTime; // 10% vertical drag

        p.Velocity += (gravity + windForce) * deltaTime + drag;
        p.Position += p.Velocity * deltaTime;

        // 2. Collision Detection (Step Raycast)
        glm::vec3 movementVec = p.Position - oldPos;
        float dist = glm::length(movementVec);
        if (dist < 0.001f) continue;
        
        glm::vec3 dir = glm::normalize(movementVec);
        glm::vec3 hitPoint;
        glm::vec4 hitColor(1.0f);
        bool hit = false;
        bool hitTree = false;

        // A. Trees
        std::vector<glm::vec4> nearbyTrees;
        chunkManager.GetTreesInRange(oldPos, dist + 5.0f, nearbyTrees); // Optimized fetch
        
        for(const auto& treeData : nearbyTrees) {
            glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
            float treeScale = treeData.w;
            
            // 1. TRUNK COLLISION (Stops Bullet)
            float halfW = 0.6f * treeScale;
            // Trunk AABB
            glm::vec3 minB = treePos - glm::vec3(halfW, 0, halfW); 
            glm::vec3 maxB = treePos + glm::vec3(halfW, 6.0f*treeScale, halfW); // Trunk Height 6.0
            minB.y = treePos.y; 
            
            float t = 0;
            // Use existing RayAABB for trunk (Boolean result + closest hit)
            if (RayAABB(oldPos, dir, minB, maxB, dist, t)) {
                hit = true; 
                hitTree = true;
                hitPoint = oldPos + dir * (t - 0.2f); // Retract slightly to prevent Z-fighting at distance
                hitColor = glm::vec4(0.8f, 0.6f, 0.3f, 1.0f); // Very Bright Wood
                // Trunk hit stops bullet instantly, so we don't check leaves for this tree (or maybe we should? No, trunk is solid)
                break; 
            }

            // 2. LEAVES COLLISION (Pass-through + Particles)
            // Leaves AABB: Expanded to cover "Pyramid" top and wider branches
            float leavesW = 4.0f * treeScale; // Wider
            float leavesBase = treePos.y + 6.0f * treeScale;
            float leavesTop = treePos.y + 25.0f * treeScale; // Much Taller to catch the top
            
            glm::vec3 lMin = treePos - glm::vec3(leavesW, 0, leavesW);
            glm::vec3 lMax = treePos + glm::vec3(leavesW, 0, leavesW);
            lMin.y = leavesBase;
            lMax.y = leavesTop;

            float tNear, tFar;
            if (GetRayAABBIntersections(oldPos, dir, lMin, lMax, tNear, tFar)) {
                auto SpawnLeafParticles = [&](glm::vec3 pos) {
                    for(int i=0; i<6; i++) {
                        glm::vec3 rVel = glm::vec3((rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f);
                        // Very Bright Neon Green for leaves
                        particles.SpawnParticle(pos, rVel * 2.0f, glm::vec4(0.4f, 1.0f, 0.4f, 1.0f), 0.15f, 0.5f, -5.0f);
                    }
                };

                // Check Entry
                if (tNear >= 0.0f && tNear <= dist) {
                    SpawnLeafParticles(oldPos + dir * (tNear - 0.2f)); // Bias Entry
                }
                // Check Exit
                if (tFar >= 0.0f && tFar <= dist) {
                     SpawnLeafParticles(oldPos + dir * (tFar + 0.5f)); // Push Exit OUTWARD
                }
            }
        }

        if (hit) {
            p.Active = false;
            
            // Spawn Crater (Only on ground)
            // Bias crater up slightly too? FootprintSystem might handle it.
            if(!hitTree) craters.AddFootprint(hitPoint, (float)(rand() % 360));
            
            // Spawn Debris
            int debrisCount = hitTree ? 15 : 8; 
            for(int i=0; i<debrisCount; i++) {
                 glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, 3.0f + (rand()%100)/50.0f, (rand()%100)/100.0f - 0.5f);
                 if(hitTree) rndVel *= 1.4f; 
                 particles.SpawnParticle(hitPoint, rndVel, hitColor, 0.08f, 0.6f, -9.8f);
            }
            continue; // Stop processing this bullet
        }

        // B. Terrain
        if (!hit) {
            float groundY = WorldGenerator::GetHeight(p.Position.x, p.Position.z);
            if (p.Position.y < groundY) {
                hit = true;
                // Approximate impact point
                hitPoint = p.Position; 
                hitPoint.y = groundY + 0.1f; // Bias up
                hitColor = glm::vec4(WorldGenerator::GetTerrainColor(hitPoint.x, hitPoint.z, groundY), 1.0f);
                // Ultra Bright Terrain Hit (Light Gray/White tint)
                hitColor = hitColor + glm::vec4(0.4f, 0.4f, 0.4f, 0.0f);
                
                p.Active = false;
                // Craters handle their own Y bias usually
                craters.AddFootprint(glm::vec3(hitPoint.x, groundY, hitPoint.z), (float)(rand() % 360)); 
                for(int i=0; i<8; i++) {
                     glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, 3.0f + (rand()%100)/50.0f, (rand()%100)/100.0f - 0.5f);
                     particles.SpawnParticle(hitPoint, rndVel, hitColor, 0.08f, 0.6f, -9.8f);
                }
            }
        }
    }
    
    // Cleanup inactive
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p){ return !p.Active; }), projectiles.end());
}

void WeaponSystem::TryFire(glm::vec3 camPos, glm::vec3 camDir, ParticleSystem& particles) {
    if (cooldownTimer > 0.0f) return;
    
    cooldownTimer = 1.7f; 
    recoilTimer = 1.0f;

    // 1. Muzzle Flash (Small, bright)
    glm::vec3 muzzlePos = camPos + camDir * 1.5f + glm::vec3(0.08f, -0.15f, 0.0f);
    particles.SpawnParticle(muzzlePos, glm::vec3(0,0,0), glm::vec4(1.0f, 0.9f, 0.3f, 1.0f), 0.12f, 0.04f, 0.0f);
    
    // Spawn Smoke (Whiter and very transparent: alpha 0.05)
    for(int i=0; i<3; i++) {
        glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, 1.2f, (rand()%100)/100.0f - 0.5f) * 0.4f;
        particles.SpawnParticle(muzzlePos, rndVel, glm::vec4(0.98f, 0.98f, 0.98f, 0.05f), 0.4f, 1.0f, 0.3f);
    }

    // Spawn Projectile
    Projectile p;
    p.Active = true;
    p.LifeTime = 10.0f; // Long lifetime to see arc
    p.Position = camPos + camDir * 0.5f; 
    p.Velocity = camDir * Config::Gameplay::ProjectileSpeed; 
    projectiles.push_back(p);
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
    
    // Draw Projectiles
    if (projectiles.empty()) return;

    // Simple Point/Line drawing for projectiles
    // NOTE: This usually needs World Space ViewMatrix, but we are inside Weapon Render which might be Identity
    // IF we are in identity, we cannot draw world space projectiles here easily.
    // However, in main.cpp, Weapon.Render is called with Identity View.
    // Solution: Draw projectiles in main.cpp or restore View Matrix?
    // Hack: WeaponSystem::Render only draws the GUN.
    // We should implement WeaponSystem::RenderProjectiles(shaderProgram) separately using World View?
    // OR: Temporarily use Identity here.
    // Wait, the user plan was to modify main.cpp.
}

// Separate Render helper for Projectiles using World View
void WeaponSystem::RenderProjectiles(GLuint shaderProgram, GLuint vao, GLuint vbo) {
     if (projectiles.empty()) return;
     
     std::vector<float> data;
     for(const auto& p : projectiles) {
         if(!p.Active) continue;
        
         // Draw a simple RED BALL (Cross shape for billboard-ish look or just a point)
         // Let's draw a small Quad or 3 lines crossing
         float sz = 0.2f; // Size of the ball
         glm::vec3 pos = p.Position;
         
         auto addLine = [&](glm::vec3 p1, glm::vec3 p2) {
             data.push_back(p1.x); data.push_back(p1.y); data.push_back(p1.z);
             data.push_back(1.0f); data.push_back(0.0f); data.push_back(0.0f); // RED
             data.push_back(0); data.push_back(0); 
             data.push_back(0); data.push_back(1); data.push_back(0); 
             
             data.push_back(p2.x); data.push_back(p2.y); data.push_back(p2.z);
             data.push_back(1.0f); data.push_back(0.0f); data.push_back(0.0f); // RED
             data.push_back(0); data.push_back(0); 
             data.push_back(0); data.push_back(1); data.push_back(0); 
         };

         // Cross 3 axes
         addLine(pos - glm::vec3(sz,0,0), pos + glm::vec3(sz,0,0));
         addLine(pos - glm::vec3(0,sz,0), pos + glm::vec3(0,sz,0));
         addLine(pos - glm::vec3(0,0,sz), pos + glm::vec3(0,0,sz));
     }
     
     glBindBuffer(GL_ARRAY_BUFFER, vbo);
     glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
     glBindVertexArray(vao);
     
     // Attributes (Assuming standard layout)
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
     glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
     glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3);
     
     glDrawArrays(GL_LINES, 0, (GLsizei)(data.size()/11));
}
