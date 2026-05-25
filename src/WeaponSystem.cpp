#include "WeaponSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include "Monster.h" // Needed for collision
#include "Config.h" // NEW

// Möller–Trumbore Ray-Triangle Intersection
bool IntersectTriangle(glm::vec3 orig, glm::vec3 dir, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, float& t) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1, edge2, h, s, q;
    float a, f, u, v;
    edge1 = v1 - v0;
    edge2 = v2 - v0;
    h = glm::cross(dir, edge2);
    a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return false; // Parallel
    f = 1.0f / a;
    s = orig - v0;
    u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    q = glm::cross(s, edge1);
    v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    t = f * glm::dot(edge2, q);
    if (t > EPSILON) return true;
    return false;
}

// Check Ray against 6 triangles of the pyramid shell
bool GetRayPyramidMeshIntersection(glm::vec3 origin, glm::vec3 dir, glm::vec3 baseCenter, float w, float h, float& tNear, float& tFar) {
    glm::vec3 apex(baseCenter.x, baseCenter.y + h, baseCenter.z);
    
    // Base Corners (Clockwise or CCW doesn't matter much for double sided check, but consistent winding is good)
    glm::vec3 c1 = baseCenter + glm::vec3(-w, 0, -w);
    glm::vec3 c2 = baseCenter + glm::vec3(w, 0, -w);
    glm::vec3 c3 = baseCenter + glm::vec3(w, 0, w);
    glm::vec3 c4 = baseCenter + glm::vec3(-w, 0, w);
    
    float tMin = 999999.0f;
    float tMax = -999999.0f;
    bool hitAny = false;
    
    auto checkTri = [&](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        float t;
        if (IntersectTriangle(origin, dir, p1, p2, p3, t)) {
            hitAny = true;
            if (t < tMin) tMin = t;
            if (t > tMax) tMax = t;
        }
    };
    
    // 4 SIDES
    checkTri(c1, c2, apex); // Front
    checkTri(c2, c3, apex); // Right
    checkTri(c3, c4, apex); // Back
    checkTri(c4, c1, apex); // Left
    
    // BASE (2 aabb triangles)
    checkTri(c1, c3, c2);
    checkTri(c1, c4, c3);
    
    if (hitAny) {
        tNear = tMin;
        tFar = tMax;
        // If inside (tMin < 0), logic might need adjustment but usually standard raycast is outside-in.
        if (tMin < 0 && tMax > 0) tNear = 0; // Started inside
        return true;
    }
    return false;
}

WeaponSystem::WeaponSystem() : currentAmmo(2), maxAmmo(2), recoilTimer(0.0f), cooldownTimer(0.0f), reloadTimer(0.0f) {
    BuildShotgunMesh();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, gunVertices.size() * sizeof(float), gunVertices.data(), GL_STATIC_DRAW);

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

WeaponSystem::~WeaponSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

#include "ModelLoader.h"

void WeaponSystem::BuildShotgunMesh() {
    gunVertices.clear();
    
    // Load local file from assets
    // Path needs to be relative to CWD (usually bin/ or project root depending on run)
    // Assets are copied to bin/assets. So if CWD is bin, path is assets/models/shotgun.txt.
    // If running from project root, path is assets/models/shotgun.txt.
    std::string path = "assets/models/shotgun.txt";
    
    std::vector<BoxDef> boxes = ModelLoader::Load(path);
    if (boxes.empty()) {
        std::cerr << "Failed to load shotgun model from " << path << ". Using fallback." << std::endl;
        // Fallback: A simple box so it's not invisible
        BoxDef fallback;
        fallback.Pos = glm::vec3(0,0,0); fallback.Scale = glm::vec3(0.1, 0.1, 1.0); fallback.Color = glm::vec3(1,0,1);
        boxes.push_back(fallback);
    }

    ModelLoader::GenerateMesh(boxes, gunVertices);
}

// Helper for Ray-Box Intersection
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

void WeaponSystem::Update(float deltaTime, glm::vec2 windDir, float windStrength, ChunkManager& chunkManager, FootprintSystem& craters, ParticleSystem& particles, const std::vector<std::unique_ptr<Monster>>& monsters) {
    if (recoilTimer > 0.0f) recoilTimer -= deltaTime * 5.0f; // Recovery speed
    if (recoilTimer < 0.0f) recoilTimer = 0.0f;
    
    if (cooldownTimer > 0.0f) cooldownTimer -= deltaTime;
    
    if (reloadTimer > 0.0f) {
        reloadTimer -= deltaTime;
        if (reloadTimer <= 0.0f) {
            reloadTimer = 0.0f;
            currentAmmo = maxAmmo;
            std::cout << "[Weapon] Reload complete! Ammo refilled to " << currentAmmo << std::endl;
        }
    }

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

        // 0. MONSTER COLLISION (Highest Priority)
        for (const auto& monsterPtr : monsters) {
            Monster& monster = *monsterPtr;
            if (!monster.IsDead()) {
                 float mDist = 0.0f;
                 bool isHeadshot = false;
                 // Check against ray segment [oldPos, p.Position]
                 // IntersectRay returns distance from oldPos
                 if (monster.IntersectRay(oldPos, dir, mDist, isHeadshot)) {
                      if (mDist <= dist) {
                          hit = true;
                          hitPoint = oldPos + dir * mDist;
                          
                          // Apply Damage
                          float dmg = isHeadshot ? 2.0f : 1.0f;
                          monster.TakeDamage(dmg, isHeadshot);
                          
                          // Visuals: BLOOD
                          // Red Burst
                          int bloodCount = isHeadshot ? 25 : 12; // More blood for headshot
                          for(int i=0; i<bloodCount; i++) {
                              glm::vec3 rndVel = glm::vec3((rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f);
                              rndVel = glm::normalize(rndVel) * (2.0f + (rand()%100)/50.0f); // High speed burst
                              particles.SpawnParticle(hitPoint, rndVel, glm::vec4(0.7f, 0.0f, 0.0f, 1.0f), 0.1f, 0.4f, -9.8f);
                          }
                          
                          p.Active = false;
                          break; // Stop checking other monsters
                      }
                 }
            }
        }
        if (!p.Active) continue; // Skip other checks

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

            // 2. LEAVES COLLISION (Pyramid MESH Check - Robust)
            float leavesW = 3.0f * treeScale; 
            float leavesBase = treePos.y + 6.0f * treeScale;
            float leavesH = 19.0f * treeScale; // 25.0 (Top) - 6.0 (Base)
            
            glm::vec3 pyrCenter(treePos.x, leavesBase, treePos.z);
            
            float tNear, tFar;
            if (GetRayPyramidMeshIntersection(oldPos, dir, pyrCenter, leavesW, leavesH, tNear, tFar)) {
                auto SpawnLeafParticles = [&](glm::vec3 pos) {
                    for(int i=0; i<6; i++) {
                        glm::vec3 rVel = glm::vec3((rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f, (rand()%100)/100.0f - 0.5f);
                        // Very Bright Neon Green for leaves
                        particles.SpawnParticle(pos, rVel * 2.0f, glm::vec4(0.4f, 1.0f, 0.4f, 1.0f), 0.15f, 0.5f, -5.0f);
                    }
                };

                // Check Entry
                if (tNear >= 0.0f && tNear <= dist) {
                    SpawnLeafParticles(oldPos + dir * (tNear - 0.2f)); 
                }
                // Check Exit
                if (tFar >= 0.0f && tFar <= dist) {
                     SpawnLeafParticles(oldPos + dir * (tFar + 0.5f));
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

void WeaponSystem::TryFire(glm::vec3 camPos, glm::vec3 camDir, ParticleSystem& particles, const std::vector<std::unique_ptr<Monster>>& monsters) {
    if (currentAmmo <= 0) {
        return;
    }
    if (cooldownTimer > 0.0f || reloadTimer > 0.0f) return;
    
    currentAmmo--;
    cooldownTimer = 1.7f; 
    recoilTimer = 1.0f;

    // Calculate Camera Basis Vectors
    glm::vec3 worldUp(0, 1, 0);
    
    // SAFETY: Handle looking straight up/down
    glm::vec3 right;
    if (abs(glm::dot(camDir, worldUp)) > 0.99f) {
        // Looking vertical, use Z as temporary up
        right = glm::normalize(glm::cross(camDir, glm::vec3(0, 0, 1)));
    } else {
        right = glm::normalize(glm::cross(camDir, worldUp));
    }
    
    glm::vec3 up = glm::cross(right, camDir);

    // Calculate Muzzle Position (Barrel Tip)
    // Matches Visual Render: Right 0.2, Down 0.2, Forward ~1.5 (Mesh Length) + 0.5 (Offset) = 2.0
    // Adjust slightly to align perfectly with visual model tip
    glm::vec3 currMuzzleOffset = (right * 0.25f) + (up * -0.2f) + (camDir * 1.8f); 
    glm::vec3 muzzlePos = camPos + currMuzzleOffset;

    // 1. Muzzle Flash (Small, bright)
    particles.SpawnParticle(muzzlePos, glm::vec3(0,0,0), glm::vec4(1.0f, 0.9f, 0.3f, 1.0f), 0.12f, 0.04f, 0.0f);
    
    // Spawn Smoke (Whiter and very transparent: alpha 0.05)
    for(int i=0; i<3; i++) {
        // Smoke velocity generally forwards + random
        glm::vec3 rndVel = (camDir * 0.5f) + glm::vec3((rand()%100)/100.0f - 0.5f, 0.5f, (rand()%100)/100.0f - 0.5f) * 0.2f;
        particles.SpawnParticle(muzzlePos, rndVel, glm::vec4(0.98f, 0.98f, 0.98f, 0.05f), 0.4f, 1.0f, 0.3f);
    }

    // Spawn Projectile
    Projectile p;
    p.Active = true;
    p.LifeTime = 10.0f; // Long lifetime to see arc
    p.Position = muzzlePos; 
    
    // Calculate Fire Direction (Convergence)
    // Bullet should hit what the crosshair is looking at (Infinite or 50m away)
    // If we just use camDir, bullet flies parallel to sight (Offset right).
    glm::vec3 targetPoint = camPos + camDir * 50.0f; // Converge at 50 meters
    glm::vec3 fireDir = glm::normalize(targetPoint - muzzlePos);
    
    p.Velocity = fireDir * Config::Gameplay::ProjectileSpeed; 
    projectiles.push_back(p);

    // Notify Monsters of Sound (Infinite range)
    for (const auto& monsterPtr : monsters) {
        monsterPtr->HearSound(muzzlePos, Config::Gameplay::GunshotSoundRange);
    }
}

void WeaponSystem::Reload() {
    if (currentAmmo < maxAmmo && reloadTimer <= 0.0f && cooldownTimer <= 0.0f) {
        reloadTimer = 2.8f;
        cooldownTimer = 2.8f; // Prevent firing during reload
        std::cout << "[Weapon] Reloading shotgun... (2.8s)" << std::endl;
    }
}

void WeaponSystem::Render(GLuint shaderProgram) {
    // Draw Gun Model in Screen Space (or overlay)
    // We attach it to the camera.
    // In main loop, View Matrix is already set. We just need to Model Matrix it relative to camera.
    
    // Standard Way: Clear Depth. Set View = Identity. Set Model = Translation/Recoil.
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "u_View");
    
    float recoilOffset = sin(recoilTimer * 3.14f) * 0.2f; // Back and up
    
    // Reload tilt animation
    float reloadOffset = 0.0f;
    float reloadRotate = 0.0f;
    if (reloadTimer > 0.0f) {
        float progress = reloadTimer / 2.8f; // 1 to 0
        reloadOffset = -0.3f * sin(progress * 3.14159f);
        reloadRotate = -35.0f * sin(progress * 3.14159f);
    }
    
    glm::mat4 model = glm::mat4(1.0f);
    // Position on screen (Right hand side, slightly down)
    model = glm::translate(model, glm::vec3(0.2f, -0.2f + reloadOffset, -0.9f + recoilOffset)); 
    // Aim slightly up
    model = glm::rotate(model, glm::radians(5.0f - recoilTimer * 10.0f + reloadRotate), glm::vec3(1,0,0));
    model = glm::scale(model, glm::vec3(0.5f)); 

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    glBindVertexArray(VAO);
    // Override color uniform if needed, or rely on vertex attributes
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(gunVertices.size()/11));
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
