#include "WorldGenerator.h"
#include "TerrainDeformation.h"
#include <cstdlib>
#include <cmath>
#include "Config.h"
#include <iostream> 
#include "ModelLoader.h"
#include <map>

unsigned int WorldGenerator::GlobalSeed = 0;
float WorldGenerator::OffsetX = 0.0f;
float WorldGenerator::OffsetZ = 0.0f;

// Pseudo-random hash (0-1)
float Hash(float x, float z) {
    float p = x * 12.9898f + z * 78.233f;
    return glm::fract(sin(p) * 43758.5453f);
}

// Smooth Value Noise
float SmoothNoise(float x, float z) {
    float i_x = floor(x); float i_z = floor(z);
    float f_x = glm::fract(x); float f_z = glm::fract(z);
    
    // Four corners
    float a = Hash(i_x, i_z);
    float b = Hash(i_x + 1.0f, i_z);
    float c = Hash(i_x, i_z + 1.0f);
    float d = Hash(i_x + 1.0f, i_z + 1.0f);
    
    // Smooth interpolation (Smoothstep)
    float u = f_x * f_x * (3.0f - 2.0f * f_x);
    float v = f_z * f_z * (3.0f - 2.0f * f_z);
    
    return glm::mix(glm::mix(a, b, u), glm::mix(c, d, u), v);
}

float WorldGenerator::GetMountainFactor(float x, float z) {
    // Large tectonic noise for distant, grand mountain ranges
    float mNoise = SmoothNoise(x * 0.0020f + 500.0f, z * 0.0020f + 500.0f);
    // Smooth transition: only higher values create mountain ridges (15-20% of map)
    return glm::smoothstep(0.60f, 0.82f, mNoise);
}

float WorldGenerator::GetBaseHeight(float x, float z) {
    // Apply Random Offset for Per-Run Variety
    float noiseX = x + OffsetX;
    float noiseZ = z + OffsetZ;

    float y = 0.0f;
    
    // Octave 1: Base Hills (Valleys & Foothills)
    y += SmoothNoise(noiseX * Config::Terrain::BaseFreqX, noiseZ * Config::Terrain::BaseFreqZ) * Config::Terrain::BaseAmplitude;
    
    // Octave 2: Detail (Roughness)
    y += SmoothNoise(noiseX * Config::Terrain::DetailFreqX, noiseZ * Config::Terrain::DetailFreqZ) * Config::Terrain::DetailAmplitude;
    
    // --- PROCEDURAL MOUNTAIN RANGES (Ridged Multifractal) ---
    float mFactor = GetMountainFactor(noiseX, noiseZ);
    if (mFactor > 0.001f) {
        // Octave 1: Main sharp ridges & knife-edge peaks
        float r1 = SmoothNoise(noiseX * 0.012f + 50.0f, noiseZ * 0.012f + 50.0f);
        float ridge1 = 1.0f - std::abs(2.0f * r1 - 1.0f);
        ridge1 = ridge1 * ridge1; // Sharpen peaks

        // Octave 2: Secondary jagged crags
        float r2 = SmoothNoise(noiseX * 0.035f, noiseZ * 0.035f);
        float ridge2 = 1.0f - std::abs(2.0f * r2 - 1.0f);

        // Octave 3: High frequency rock ruggedness
        float r3 = SmoothNoise(noiseX * 0.12f, noiseZ * 0.12f);

        float mountainElevation = (ridge1 * 44.0f) + (ridge2 * 14.0f) + (r3 * 3.5f);
        y += mountainElevation * mFactor;
    }

    // --- LAGOON CARVING (Only in Lowland Valleys) ---
    if (mFactor < 0.35f) {
        float moisture = GetMoisture(noiseX, noiseZ);
        if (moisture > (1.0f - Config::Water::Chance)) {
            float factor = (moisture - (1.0f - Config::Water::Chance)) / Config::Water::Chance;
            factor = factor * factor * (3.0f - 2.0f * factor);
            factor *= (1.0f - mFactor / 0.35f);
            y -= factor * Config::Water::Depth;
        }
    }

    return y;
}

float WorldGenerator::GetHeight(float x, float z) {
    return GetBaseHeight(x, z) + TerrainDeformation::GetDeformation(x, z);
}

float WorldGenerator::GetVisualHeight(float x, float z) {
    float y = GetHeight(x, z);
    // Quantize height to discrete "steps" (0.5 units per step) for visual style
    y = std::floor(y * 2.0f) / 2.0f;
    return y;
}

std::vector<glm::vec2> WorldGenerator::GetChunkTreeLocations(int chunkX, int chunkZ, int chunkSize, float scale) {
    std::vector<glm::vec2> treePositions;
    float startX = chunkX * chunkSize * scale;
    float startZ = chunkZ * chunkSize * scale;

    unsigned int seed = (unsigned int)(chunkX * 73856093 ^ chunkZ * 19349663 ^ GlobalSeed);
    srand(seed);

    int treeCount = 3 + rand() % 5; 
    std::vector<glm::vec2> placedPositions;

    for (int i = 0; i < treeCount; i++) {
        int attempts = 0;
        bool valid = false;
        float tx, tz;

        while (!valid && attempts < 10) {
            tx = startX + (rand() % (int)(chunkSize * scale * 10)) / 10.0f;
            tz = startZ + (rand() % (int)(chunkSize * scale * 10)) / 10.0f;
            
            valid = true;
            for (const auto& pos : placedPositions) {
                float dx = tx - pos.x;
                float dz = tz - pos.y;
                if (sqrt(dx*dx + dz*dz) < 5.0f) { 
                    valid = false; 
                    break;
                }
            }
            attempts++;
        }

        if (valid) {
            float h = GetExactHeight(tx, tz);
            float mFactor = GetMountainFactor(tx, tz);

            // Compute ground slope
            float hL = GetExactHeight(tx - 1.0f, tz);
            float hR = GetExactHeight(tx + 1.0f, tz);
            float hD = GetExactHeight(tx, tz - 1.0f);
            float hU = GetExactHeight(tx, tz + 1.0f);
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
            float slope = 1.0f - normal.y;

            // Don't spawn trees in water or on sheer rock walls (slope > 0.38) or barren rocky tops (> 55m)
            if (IsLagoon(tx, tz, h) || h > 55.0f || slope > 0.38f) {
                valid = false;
            } else {
                // If on high mountain grassy area (mFactor > 0.45), spawn with balanced alpine distribution
                if (mFactor > 0.45f && (rand() % 100 > 55)) {
                    valid = false;
                } else {
                    treePositions.push_back(glm::vec2(tx, tz));
                    placedPositions.push_back(glm::vec2(tx, tz));
                }
            }
        }
    }
    return treePositions;
}

glm::vec3 WorldGenerator::GetTerrainColor(float x, float z, float y) {
    return GetTerrainColor(x, z, y, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 WorldGenerator::GetTerrainColor(float x, float z, float y, const glm::vec3& normal) {
    // Organic Lowland Noise (Grass / Dirt)
    float n1 = SmoothNoise(x * 0.03f, z * 0.03f); 
    float n2 = SmoothNoise(x * 0.1f, z * 0.1f);
    float n3 = SmoothNoise(x * 0.5f, z * 0.5f);
    float finalNoise = n1 * 0.6f + n2 * 0.3f + n3 * 0.1f;
    
    glm::vec3 baseColor;
    if (finalNoise > 0.55f) {
         // DIRT / MUD (Browns)
         baseColor = glm::vec3(0.35f, 0.28f, 0.18f) + glm::vec3(0.04f) * (n3 - 0.5f); 
    } else {
         // GRASS (Forest Greens)
         float tone = n1; 
         baseColor = glm::mix(glm::vec3(0.10f, 0.24f, 0.10f), glm::vec3(0.14f, 0.32f, 0.13f), tone);
         baseColor += glm::vec3(0.03f, 0.05f, 0.02f) * (n3 - 0.5f);
    }

    // Dynamic excavation / terraforming tint
    float deform = TerrainDeformation::GetDeformation(x, z);
    if (std::abs(deform) > 0.15f) {
        float deformFactor = glm::clamp(std::abs(deform) / 2.5f, 0.0f, 1.0f);
        glm::vec3 excavatedSoil = (deform < 0.0f) ? glm::vec3(0.24f, 0.18f, 0.12f) : glm::vec3(0.33f, 0.28f, 0.20f);
        baseColor = glm::mix(baseColor, excavatedSoil, deformFactor * 0.85f);
    }

    // Height darkening for valley depth
    baseColor += glm::vec3(0.01f, (y + 5.0f) * 0.012f, 0.01f);

    // --- MOUNTAIN & CLIFF ROCK SHADING ---
    // 1. Slope cliff factor: steep surfaces turn to slate rock
    float slope = 1.0f - normal.y; // 0 on flat, 1 on vertical wall
    float cliffFactor = glm::smoothstep(0.18f, 0.40f, slope);

    // 2. High altitude rock factor
    float altFactor = glm::smoothstep(15.0f, 30.0f, y);

    // Rock Color palette (Dark slate / granite / basalt)
    float rockNoise = SmoothNoise(x * 0.35f, z * 0.35f);
    glm::vec3 darkSlate = glm::vec3(0.20f, 0.20f, 0.22f) + glm::vec3(0.04f) * (rockNoise - 0.5f);
    glm::vec3 lightGranite = glm::vec3(0.32f, 0.31f, 0.34f) + glm::vec3(0.05f) * (rockNoise - 0.5f);
    glm::vec3 rockColor = glm::mix(darkSlate, lightGranite, SmoothNoise(x * 0.08f, z * 0.08f));

    // High Peak Frost / Cold Stone (y > 32.0f)
    if (y > 30.0f) {
        float snowFactor = glm::smoothstep(30.0f, 46.0f, y);
        glm::vec3 snowColor = glm::vec3(0.70f, 0.75f, 0.82f) + glm::vec3(0.04f) * rockNoise;
        rockColor = glm::mix(rockColor, snowColor, snowFactor * (1.0f - cliffFactor * 0.4f));
    }

    float rockBlend = std::max(cliffFactor, altFactor);
    return glm::mix(baseColor, rockColor, rockBlend);
}

// --- LAGOON LOGIC ---
float WorldGenerator::GetMoisture(float x, float z) {
    // Moisture Noise (Large, rare patches)
    return SmoothNoise(x * 0.015f + 500.0f, z * 0.015f + 500.0f);
}

bool WorldGenerator::IsLagoon(float x, float z, float h) {
    // Rely on moisture and depth check
    // We can just check if height is below water level, 
    // BUT since we are carving, the height IS below water level because of moisture.
    // So checking (VisualHeight < WaterLevel) is correct.
    return (h < Config::Water::Level);
}

float WorldGenerator::GetExactHeight(float x, float z) {
    float scale = Config::World::ChunkScale; 
    
    int gridX = (int)floor(x / scale);
    int gridZ = (int)floor(z / scale);
    
    float localX = (x - gridX * scale) / scale;
    float localZ = (z - gridZ * scale) / scale; // 0..1 inside the quad

    // Get heights of the 4 corners at correct world coordinates
    float y00 = GetVisualHeight((float)gridX * scale, (float)gridZ * scale);
    float y10 = GetVisualHeight((float)(gridX+1) * scale, (float)gridZ * scale);
    float y01 = GetVisualHeight((float)gridX * scale, (float)(gridZ+1) * scale);
    float y11 = GetVisualHeight((float)(gridX+1) * scale, (float)(gridZ+1) * scale);

    float height = 0.0f;
    
    if (localX + localZ <= 1.0f) {
        // Triangle 1: (0,0), (0,1), (1,0)
        // Barycentric interpolation on right-angled triangle
        height = y00 + (y10 - y00) * localX + (y01 - y00) * localZ;
    } else {
         // Triangle 2: (1,1), (0,1), (1,0)
         // Plane passing through (1,1,y11), (0,1,y01), (1,0,y10)
         height = y11 + (y01 - y11) * (1.0f - localX) + (y10 - y11) * (1.0f - localZ);
    }
    
    return height;
}

WorldData WorldGenerator::GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale) {
    std::vector<glm::vec2> nearbyTrees;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            auto chunkTrees = GetChunkTreeLocations(chunkX + dx, chunkZ + dz, chunkSize, scale);
            nearbyTrees.insert(nearbyTrees.end(), chunkTrees.begin(), chunkTrees.end());
        }
    }
    return GenerateChunkTerrain(chunkX, chunkZ, chunkSize, scale, nearbyTrees);
}

WorldData WorldGenerator::GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale, const std::vector<glm::vec2>& nearbyTrees) {
    WorldData data;
    // Pre-allocate
    data.vertices.reserve(chunkSize * chunkSize * 6);
    data.waterVertices.reserve(chunkSize * chunkSize / 2); // Roughly
    
    float startX = chunkX * chunkSize * scale;
    float startZ = chunkZ * chunkSize * scale;

    for (int z = 0; z < chunkSize; z++) {
        for (int x = 0; x < chunkSize; x++) {
            float x0 = startX + x * scale;
            float z0 = startZ + z * scale;
            float x1 = startX + (x + 1) * scale;
            float z1 = startZ + (z + 1) * scale;
            
            // 1. Calculate Heights First
            float y00 = GetVisualHeight(x0, z0);
            float y10 = GetVisualHeight(x1, z0);
            float y01 = GetVisualHeight(x0, z1);
            float y11 = GetVisualHeight(x1, z1);

            // Compute Geometric Normals
            glm::vec3 p1(x0, y00, z0);
            glm::vec3 p2(x0, y01, z1);
            glm::vec3 p3(x1, y10, z0);
            glm::vec3 n1 = glm::normalize(glm::cross(p2 - p1, p3 - p1));

            glm::vec3 p4(x1, y10, z0);
            glm::vec3 p5(x0, y01, z1);
            glm::vec3 p6(x1, y11, z1);
            glm::vec3 n2 = glm::normalize(glm::cross(p5 - p4, p6 - p4));

            glm::vec3 avgNormal = glm::normalize(n1 + n2);
            
            // 2. Calculate Base Colors with Slope & Altitude shading
            glm::vec3 c00 = GetTerrainColor(x0, z0, y00, avgNormal);
            glm::vec3 c10 = GetTerrainColor(x1, z0, y10, avgNormal);
            glm::vec3 c01 = GetTerrainColor(x0, z1, y01, avgNormal);
            glm::vec3 c11 = GetTerrainColor(x1, z1, y11, avgNormal);
            
            // 3. Check for Lagoon (Water)
            float cx = (x0 + x1) * 0.5f;
            float cz = (z0 + z1) * 0.5f;
            float cy = (y00 + y10 + y01 + y11) * 0.25f;
            
            if (IsLagoon(cx, cz, cy)) {
                // Ground becomes Mud
                glm::vec3 mud = glm::vec3(0.25f, 0.2f, 0.15f);
                c00 = c10 = c01 = c11 = mud; 
                
                // --- GENERATE WATER SURFACE (Transparent Pass) ---
                float wY = Config::Water::Level;
                // Translucent Blue
                glm::vec3 wCol = glm::vec3(0.2f, 0.5f, 0.8f); 
                
                float pad = scale * 0.5f;
                
                Vertex w1 = { glm::vec3(x0 - pad, wY, z0 - pad), wCol, glm::vec2(0,0), glm::vec3(0,1,0) };
                Vertex w2 = { glm::vec3(x0 - pad, wY, z1 + pad), wCol, glm::vec2(0,1), glm::vec3(0,1,0) }; 
                Vertex w3 = { glm::vec3(x1 + pad, wY, z0 - pad), wCol, glm::vec2(1,0), glm::vec3(0,1,0) };
                Vertex w4 = { glm::vec3(x1 + pad, wY, z1 + pad), wCol, glm::vec2(1,1), glm::vec3(0,1,0) };
                
                data.waterVertices.push_back(w1); 
                data.waterVertices.push_back(w2); 
                data.waterVertices.push_back(w3);
                
                data.waterVertices.push_back(w3); 
                data.waterVertices.push_back(w2); 
                data.waterVertices.push_back(w4);
            }

            // 4. Calculate Shadows (Optimized)
            auto GetShadowFactor = [&](float vx, float vz) {
                float shadow = 0.0f;
                const float radiusSq = 2.5f * 2.5f;
                for (const auto& t : nearbyTrees) {
                    float dx = vx - t.x;
                    float dz = vz - t.y;
                    float d2 = dx*dx + dz*dz;
                    if (d2 < radiusSq) { 
                        float dist = sqrt(d2); 
                        float val = 1.0f - (dist / 2.5f);
                        shadow = std::max(shadow, val);
                    }
                }
                return std::min(shadow * 1.5f, 0.95f);
            };

            float s00 = GetShadowFactor(x0, z0);
            float s10 = GetShadowFactor(x1, z0);
            float s01 = GetShadowFactor(x0, z1);
            float s11 = GetShadowFactor(x1, z1);

            // Apply Shadows
            auto ApplyShadow = [](glm::vec3& baseColor, float shadow) {
                baseColor *= (1.0f - shadow);
            };

            ApplyShadow(c00, s00);
            ApplyShadow(c10, s10);
            ApplyShadow(c01, s01);
            ApplyShadow(c11, s11);

            // 5. Generate Terrain Vertices
            data.vertices.push_back(Vertex{ p1, c00, glm::vec2(0.0f, 0.0f), n1 });
            data.vertices.push_back(Vertex{ p2, c01, glm::vec2(0.0f, 1.0f), n1 });
            data.vertices.push_back(Vertex{ p3, c10, glm::vec2(1.0f, 0.0f), n1 });

            data.vertices.push_back(Vertex{ p4, c10, glm::vec2(1.0f, 0.0f), n2 });
            data.vertices.push_back(Vertex{ p5, c01, glm::vec2(0.0f, 1.0f), n2 });
            data.vertices.push_back(Vertex{ p6, c11, glm::vec2(1.0f, 1.0f), n2 });

            // 6. Generate Rocks (Piedras Sueltas) & Wildflowers (Flores)
            float hVal = sin((x0 + 17.13f) * 12.9898f + (z0 + 43.19f) * 78.233f) * 43758.5453f;
            float propHash = hVal - floor(hVal);

            bool isWaterTile = IsLagoon(cx, cz, cy);

            if (!isWaterTile) {
                // --- PROCEDURAL ROCKS (Piedras Sueltas - 100% Closed Solid Boulders) ---
                if (propHash > 0.962f) {
                    float rx = cx + (propHash - 0.98f) * scale * 0.5f;
                    float rz = cz + (sin(propHash * 33.0f) * 0.5f) * scale * 0.5f;
                    float ry = GetExactHeight(rx, rz);
                    
                    float rSize = 0.32f + (propHash - 0.962f) * 12.0f; // 0.32m to 0.78m
                    float rHeight = rSize * (0.65f + (propHash - 0.962f) * 4.0f);
                    
                    glm::vec3 rockCol = glm::vec3(0.38f, 0.39f, 0.36f) * (0.85f + (propHash - 0.962f) * 3.0f);
                    if (avgNormal.y < 0.7f) rockCol *= 0.8f;
                    
                    float hs = rSize * 0.5f;
                    float topHs = hs * 0.60f;
                    
                    // Bottom vertices (sunken into ground to seal base)
                    glm::vec3 rb1(rx - hs, ry - 0.15f, rz - hs);
                    glm::vec3 rb2(rx + hs, ry - 0.15f, rz - hs);
                    glm::vec3 rb3(rx + hs, ry - 0.15f, rz + hs);
                    glm::vec3 rb4(rx - hs, ry - 0.15f, rz + hs);
                    
                    // Top vertices
                    glm::vec3 rt1(rx - topHs, ry + rHeight, rz - topHs);
                    glm::vec3 rt2(rx + topHs, ry + rHeight, rz - topHs);
                    glm::vec3 rt3(rx + topHs, ry + rHeight, rz + topHs);
                    glm::vec3 rt4(rx - topHs, ry + rHeight, rz + topHs);
                    
                    // 1. Top Face
                    glm::vec3 nTop(0.0f, 1.0f, 0.0f);
                    data.vertices.push_back(Vertex{ rt1, rockCol * 1.15f, glm::vec2(0,0), nTop });
                    data.vertices.push_back(Vertex{ rt2, rockCol * 1.15f, glm::vec2(1,0), nTop });
                    data.vertices.push_back(Vertex{ rt3, rockCol * 1.15f, glm::vec2(1,1), nTop });
                    data.vertices.push_back(Vertex{ rt1, rockCol * 1.15f, glm::vec2(0,0), nTop });
                    data.vertices.push_back(Vertex{ rt3, rockCol * 1.15f, glm::vec2(1,1), nTop });
                    data.vertices.push_back(Vertex{ rt4, rockCol * 1.15f, glm::vec2(0,1), nTop });
                    
                    // 2. Front Face (+Z)
                    glm::vec3 nFront = glm::normalize(glm::cross(rb3 - rb4, rt4 - rb4));
                    data.vertices.push_back(Vertex{ rb4, rockCol * 0.85f, glm::vec2(0,0), nFront });
                    data.vertices.push_back(Vertex{ rb3, rockCol * 0.85f, glm::vec2(1,0), nFront });
                    data.vertices.push_back(Vertex{ rt3, rockCol * 0.95f, glm::vec2(1,1), nFront });
                    data.vertices.push_back(Vertex{ rb4, rockCol * 0.85f, glm::vec2(0,0), nFront });
                    data.vertices.push_back(Vertex{ rt3, rockCol * 0.95f, glm::vec2(1,1), nFront });
                    data.vertices.push_back(Vertex{ rt4, rockCol * 0.95f, glm::vec2(0,1), nFront });

                    // 3. Back Face (-Z)
                    glm::vec3 nBack = glm::normalize(glm::cross(rb1 - rb2, rt2 - rb2));
                    data.vertices.push_back(Vertex{ rb2, rockCol * 0.80f, glm::vec2(0,0), nBack });
                    data.vertices.push_back(Vertex{ rb1, rockCol * 0.80f, glm::vec2(1,0), nBack });
                    data.vertices.push_back(Vertex{ rt1, rockCol * 0.90f, glm::vec2(1,1), nBack });
                    data.vertices.push_back(Vertex{ rb2, rockCol * 0.80f, glm::vec2(0,0), nBack });
                    data.vertices.push_back(Vertex{ rt1, rockCol * 0.90f, glm::vec2(1,1), nBack });
                    data.vertices.push_back(Vertex{ rt2, rockCol * 0.90f, glm::vec2(0,1), nBack });

                    // 4. Right Face (+X)
                    glm::vec3 nRight = glm::normalize(glm::cross(rb2 - rb3, rt3 - rb3));
                    data.vertices.push_back(Vertex{ rb3, rockCol * 0.88f, glm::vec2(0,0), nRight });
                    data.vertices.push_back(Vertex{ rb2, rockCol * 0.88f, glm::vec2(1,0), nRight });
                    data.vertices.push_back(Vertex{ rt2, rockCol * 0.98f, glm::vec2(1,1), nRight });
                    data.vertices.push_back(Vertex{ rb3, rockCol * 0.88f, glm::vec2(0,0), nRight });
                    data.vertices.push_back(Vertex{ rt2, rockCol * 0.98f, glm::vec2(1,1), nRight });
                    data.vertices.push_back(Vertex{ rt3, rockCol * 0.98f, glm::vec2(0,1), nRight });

                    // 5. Left Face (-X)
                    glm::vec3 nLeft = glm::normalize(glm::cross(rb4 - rb1, rt1 - rb1));
                    data.vertices.push_back(Vertex{ rb1, rockCol * 0.75f, glm::vec2(0,0), nLeft });
                    data.vertices.push_back(Vertex{ rb4, rockCol * 0.75f, glm::vec2(1,0), nLeft });
                    data.vertices.push_back(Vertex{ rt4, rockCol * 0.85f, glm::vec2(1,1), nLeft });
                    data.vertices.push_back(Vertex{ rb1, rockCol * 0.75f, glm::vec2(0,0), nLeft });
                    data.vertices.push_back(Vertex{ rt4, rockCol * 0.85f, glm::vec2(1,1), nLeft });
                    data.vertices.push_back(Vertex{ rt1, rockCol * 0.85f, glm::vec2(0,1), nLeft });

                    // 6. Bottom Face
                    glm::vec3 nBottom(0.0f, -1.0f, 0.0f);
                    data.vertices.push_back(Vertex{ rb1, rockCol * 0.60f, glm::vec2(0,0), nBottom });
                    data.vertices.push_back(Vertex{ rb3, rockCol * 0.60f, glm::vec2(1,1), nBottom });
                    data.vertices.push_back(Vertex{ rb2, rockCol * 0.60f, glm::vec2(1,0), nBottom });
                    data.vertices.push_back(Vertex{ rb1, rockCol * 0.60f, glm::vec2(0,0), nBottom });
                    data.vertices.push_back(Vertex{ rb4, rockCol * 0.60f, glm::vec2(0,1), nBottom });
                    data.vertices.push_back(Vertex{ rb3, rockCol * 0.60f, glm::vec2(1,1), nBottom });
                }
                // --- PROCEDURAL WILDFLOWERS (Flores Silvestres - Firmly Rooted in Grass) ---
                else if (propHash > 0.80f && propHash <= 0.855f && avgNormal.y > 0.80f && cy < 25.0f) {
                    float fx = cx + (propHash - 0.82f) * scale * 0.6f;
                    float fz = cz + (sin(propHash * 45.0f) * 0.5f) * scale * 0.6f;
                    float fy = GetExactHeight(fx, fz);
                    
                    glm::vec3 flowerColors[] = {
                        glm::vec3(0.92f, 0.18f, 0.22f), // Crimson Poppy
                        glm::vec3(0.98f, 0.80f, 0.15f), // Golden Marigold
                        glm::vec3(0.68f, 0.28f, 0.95f), // Violet Orchid
                        glm::vec3(0.22f, 0.70f, 0.98f), // Azure Bellflower
                        glm::vec3(0.95f, 0.95f, 0.90f)  // White Lily
                    };
                    int colIdx = (int)(propHash * 50.0f) % 5;
                    glm::vec3 fCol = flowerColors[colIdx];
                    glm::vec3 stemCol = glm::vec3(0.22f, 0.55f, 0.18f);
                    
                    float stemH = 0.22f + (propHash - 0.80f) * 0.25f;
                    float petalW = 0.09f;
                    
                    // Stem (starts 0.05m below ground to guarantee seamless ground contact)
                    float rootY = fy - 0.05f;
                    float topY = fy + stemH;

                    glm::vec3 s1(fx - 0.02f, rootY, fz);
                    glm::vec3 s2(fx + 0.02f, rootY, fz);
                    glm::vec3 s3(fx + 0.02f, topY, fz);
                    glm::vec3 s4(fx - 0.02f, topY, fz);
                    
                    data.vertices.push_back(Vertex{ s1, stemCol, glm::vec2(0,0), avgNormal });
                    data.vertices.push_back(Vertex{ s2, stemCol, glm::vec2(1,0), avgNormal });
                    data.vertices.push_back(Vertex{ s3, stemCol, glm::vec2(1,1), avgNormal });
                    data.vertices.push_back(Vertex{ s1, stemCol, glm::vec2(0,0), avgNormal });
                    data.vertices.push_back(Vertex{ s3, stemCol, glm::vec2(1,1), avgNormal });
                    data.vertices.push_back(Vertex{ s4, stemCol, glm::vec2(0,1), avgNormal });
                    
                    // Flower Blossom Petals (Cross Quad firmly at top of stem)
                    glm::vec3 p1(fx - petalW, topY, fz - petalW);
                    glm::vec3 p2(fx + petalW, topY, fz + petalW);
                    glm::vec3 p3(fx + petalW, topY + 0.05f, fz + petalW);
                    glm::vec3 p4(fx - petalW, topY + 0.05f, fz - petalW);
                    
                    data.vertices.push_back(Vertex{ p1, fCol, glm::vec2(0,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ p2, fCol, glm::vec2(1,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ p3, fCol * 1.15f, glm::vec2(1,1), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ p1, fCol, glm::vec2(0,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ p3, fCol * 1.15f, glm::vec2(1,1), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ p4, fCol * 1.15f, glm::vec2(0,1), glm::vec3(0,1,0) });

                    glm::vec3 q1(fx - petalW, topY, fz + petalW);
                    glm::vec3 q2(fx + petalW, topY, fz - petalW);
                    glm::vec3 q3(fx + petalW, topY + 0.05f, fz - petalW);
                    glm::vec3 q4(fx - petalW, topY + 0.05f, fz + petalW);
                    
                    data.vertices.push_back(Vertex{ q1, fCol, glm::vec2(0,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ q2, fCol, glm::vec2(1,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ q3, fCol * 1.15f, glm::vec2(1,1), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ q1, fCol, glm::vec2(0,0), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ q3, fCol * 1.15f, glm::vec2(1,1), glm::vec3(0,1,0) });
                    data.vertices.push_back(Vertex{ q4, fCol * 1.15f, glm::vec2(0,1), glm::vec3(0,1,0) });
                }
            }
        }
    }
    return data;
}
std::vector<glm::vec4> WorldGenerator::GenerateChunkTrees(int chunkX, int chunkZ, int chunkSize, float scale) {
    // Reuse the helper to get positions, then just add height/scale
    auto positions = GetChunkTreeLocations(chunkX, chunkZ, chunkSize, scale);
    if(positions.empty() && chunkX==0 && chunkZ==0) std::cout << "[WorldGenerator] Warning: No trees in chunk 0,0!" << std::endl;
    std::vector<glm::vec4> treeData;
    
    // We do need to re-seed here to maintain the same "scale" randomness if we want it perfect, 
    // or we can just derive scale from position hash.
    // However, since GetChunkTreeLocations does its own rand() for positions, 
    // simply iterating the result is not enough to recover the state of rand() for the scale generation 
    // unless we re-run the seed logic.
    // EASIER: Just generate the scale deterministically from the position (x,z) using noise/hash.
    
    for(const auto& p : positions) {
        float y = GetVisualHeight(p.x, p.y);
        // Deterministic scale based on position hash
        float scaleNoise = sin(p.x * 12.9898 + p.y * 78.233) * 43758.5453;
        scaleNoise = scaleNoise - floor(scaleNoise); // 0..1
        float tScale = Config::Trees::MinScale + scaleNoise * (Config::Trees::MaxScale - Config::Trees::MinScale); 
        
        treeData.push_back(glm::vec4(p.x, y, p.y, tScale));
    }
    return treeData;
}

// Split into two meshes for independent rendering control (Structural Fix)
// Helper to load and cache
std::vector<Vertex> LoadCachedMesh(const std::string& path) {
    static std::map<std::string, std::vector<Vertex>> cache;
    if (cache.find(path) != cache.end()) return cache[path];

    // Load Model
    std::vector<BoxDef> boxes = ModelLoader::Load(path);
    std::vector<float> rawFloats;
    ModelLoader::GenerateMesh(boxes, rawFloats);

    // Convert float array to Vertex struct
    std::vector<Vertex> vertices;
    size_t count = rawFloats.size() / 11;
    vertices.reserve(count);
    
    for(size_t i=0; i<count; i++) {
        size_t b = i * 11;
        Vertex v;
        v.position = glm::vec3(rawFloats[b+0], rawFloats[b+1], rawFloats[b+2]);
        v.color    = glm::vec3(rawFloats[b+3], rawFloats[b+4], rawFloats[b+5]);
        v.texCoord = glm::vec2(rawFloats[b+6], rawFloats[b+7]);
        v.normal   = glm::vec3(rawFloats[b+8], rawFloats[b+9], rawFloats[b+10]);
        vertices.push_back(v);
    }
    
    cache[path] = vertices;
    return vertices;
}

std::vector<Vertex> WorldGenerator::GetTreeTrunkMesh(int type) {
    switch (type) {
        case 0: return LoadCachedMesh("assets/models/tree_trunk_oak.txt");
        case 1: return LoadCachedMesh("assets/models/tree_trunk_pine.txt");
        case 2: return LoadCachedMesh("assets/models/tree_trunk_birch.txt");
        case 3: return LoadCachedMesh("assets/models/tree_trunk_willow.txt");
        default: return LoadCachedMesh("assets/models/tree_trunk_oak.txt");
    }
}

std::vector<Vertex> WorldGenerator::GetTreeLeavesMesh(int type) {
    switch (type) {
        case 0: return LoadCachedMesh("assets/models/tree_leaves_oak.txt");
        case 1: return LoadCachedMesh("assets/models/tree_leaves_pine.txt");
        case 2: return LoadCachedMesh("assets/models/tree_leaves_birch.txt");
        case 3: return LoadCachedMesh("assets/models/tree_leaves_willow.txt");
        default: return LoadCachedMesh("assets/models/tree_leaves_oak.txt");
    }
}

std::vector<Vertex> WorldGenerator::GetShadowMesh() {
    std::vector<Vertex> vertices;
    // Tessellated Grid (4x4) for terrain conformance
    // Increased size to 2.4f to cover tree base area properly
    float size = 2.4f; 
    float y = 0.08f; // Stick close to ground
    glm::vec3 color(0.5f, 0.5f, 0.5f); // Grey (will be modulated by texture)
    glm::vec3 nUp(0, 1, 0);

    int steps = 4;
    float stepSize = (size * 2.0f) / steps;

    for (int z = 0; z < steps; z++) {
        for (int x = 0; x < steps; x++) {
            float x0 = -size + x * stepSize;
            float z0 = -size + z * stepSize;
            float x1 = x0 + stepSize;
            float z1 = z0 + stepSize;

            // UVs based on range [-size, size] -> [0, 1]
            float u0 = (x0 + size) / (size * 2.0f);
            float v0 = (z0 + size) / (size * 2.0f);
            float u1 = (x1 + size) / (size * 2.0f);
            float v1 = (z1 + size) / (size * 2.0f);

            // Quad (2 triangles)
            vertices.push_back({{x0, y, z0}, color, {u0, v0}, nUp});
            vertices.push_back({{x0, y, z1}, color, {u0, v1}, nUp});
            vertices.push_back({{x1, y, z0}, color, {u1, v0}, nUp});
            
            vertices.push_back({{x1, y, z0}, color, {u1, v0}, nUp});
            vertices.push_back({{x0, y, z1}, color, {u0, v1}, nUp});
            vertices.push_back({{x1, y, z1}, color, {u1, v1}, nUp});
        }
    }
    return vertices;
}

void WorldGenerator::Internal_AddTreeGeometry(float x, float y, float z, float trunkW, float trunkH, float leavesW, float leavesH, std::vector<Vertex>& vertices) {
    glm::vec3 trunkColor(0.4f, 0.2f, 0.1f);
    glm::vec3 leavesColor(0.0f, 0.6f, 0.0f);

    float tL = x - trunkW; float tR = x + trunkW;
    float tB = z - trunkW; float tF = z + trunkW;
    // EXTEND FOUNDATION: Start trunk from (y - 5.0) to ensure it's grounded on slopes
    float tY_Base = y - 5.0f; 
    float tY_Top = y + trunkH;
    
    glm::vec3 nF(0,0,1), nB(0,0,-1), nL(-1,0,0), nR(1,0,0), nUp(0,1,0);

    // Front
    vertices.push_back({{tL,tY_Base,tF}, trunkColor, {0,0}, nF}); vertices.push_back({{tL,tY_Top,tF}, trunkColor, {0,1}, nF}); vertices.push_back({{tR,tY_Base,tF}, trunkColor, {1,0}, nF});
    vertices.push_back({{tR,tY_Base,tF}, trunkColor, {1,0}, nF}); vertices.push_back({{tL,tY_Top,tF}, trunkColor, {0,1}, nF}); vertices.push_back({{tR,tY_Top,tF}, trunkColor, {1,1}, nF});
    // Back
    vertices.push_back({{tR,tY_Base,tB}, trunkColor, {0,0}, nB}); vertices.push_back({{tR,tY_Top,tB}, trunkColor, {0,1}, nB}); vertices.push_back({{tL,tY_Base,tB}, trunkColor, {1,0}, nB});
    vertices.push_back({{tL,tY_Base,tB}, trunkColor, {1,0}, nB}); vertices.push_back({{tR,tY_Top,tB}, trunkColor, {0,1}, nB}); vertices.push_back({{tL,tY_Top,tB}, trunkColor, {1,1}, nB});
    // Left
    vertices.push_back({{tL,tY_Base,tB}, trunkColor, {0,0}, nL}); vertices.push_back({{tL,tY_Top,tB}, trunkColor, {0,1}, nL}); vertices.push_back({{tL,tY_Base,tF}, trunkColor, {1,0}, nL});
    vertices.push_back({{tL,tY_Base,tF}, trunkColor, {1,0}, nL}); vertices.push_back({{tL,tY_Top,tB}, trunkColor, {0,1}, nL}); vertices.push_back({{tL,tY_Top,tF}, trunkColor, {1,1}, nL});
    // Right
    vertices.push_back({{tR,tY_Base,tF}, trunkColor, {0,0}, nR}); vertices.push_back({{tR,tY_Top,tF}, trunkColor, {0,1}, nR}); vertices.push_back({{tR,tY_Base,tB}, trunkColor, {1,0}, nR});
    vertices.push_back({{tR,tY_Base,tB}, trunkColor, {1,0}, nR}); vertices.push_back({{tR,tY_Top,tF}, trunkColor, {0,1}, nR}); vertices.push_back({{tR,tY_Top,tB}, trunkColor, {1,1}, nR});

    // Leaves
    float lY1 = tY_Top; float lY2 = tY_Top + leavesH;
    float lL = x - leavesW; float lR = x + leavesW;
    float lB = z - leavesW; float lF = z + leavesW;
    glm::vec3 peak(x, lY2, z);

    vertices.push_back({{lL,lY1,lF}, leavesColor, {0,0}, nF}); vertices.push_back({peak, leavesColor, {0.5,1}, nUp}); vertices.push_back({{lR,lY1,lF}, leavesColor, {1,0}, nF});
    vertices.push_back({{lR,lY1,lB}, leavesColor, {0,0}, nB}); vertices.push_back({peak, leavesColor, {0.5,1}, nUp}); vertices.push_back({{lL,lY1,lB}, leavesColor, {1,0}, nB});
    vertices.push_back({{lL,lY1,lB}, leavesColor, {0,0}, nL}); vertices.push_back({peak, leavesColor, {0.5,1}, nUp}); vertices.push_back({{lL,lY1,lF}, leavesColor, {1,0}, nL});
    vertices.push_back({{lR,lY1,lF}, leavesColor, {0,0}, nR}); vertices.push_back({peak, leavesColor, {0.5,1}, nUp}); vertices.push_back({{lR,lY1,lB}, leavesColor, {1,0}, nR});
}

std::vector<unsigned char> WorldGenerator::GenerateNoiseTexture(int width, int height) {
    std::vector<unsigned char> data(width * height * 3);
    for (int i = 0; i < width * height * 3; ++i) data[i] = (unsigned char)(rand() % 256);
    return data;
}
