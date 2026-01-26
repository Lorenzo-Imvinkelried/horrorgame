#include "WorldGenerator.h"
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

float WorldGenerator::GetHeight(float x, float z) {
    // Apply Random Offset for Per-Run Variety
    x += OffsetX;
    z += OffsetZ;

    float y = 0.0f;
    
    // Octave 1: Base Hills (Large features)
    // Value Noise returns 0..1, so we center it around 0 (-0.5..0.5) for hills/valleys if desired, 
    // or just scale it. Let's keep 0..1 and scale.
    y += SmoothNoise(x * Config::Terrain::BaseFreqX, z * Config::Terrain::BaseFreqZ) * Config::Terrain::BaseAmplitude;
    
    // Octave 2: Detail (Roughness)
    y += SmoothNoise(x * Config::Terrain::DetailFreqX, z * Config::Terrain::DetailFreqZ) * Config::Terrain::DetailAmplitude;
    
    // --- LAGOON CARVING ---
    // If this area is "Wet", sink the terrain to make room for water.
    float moisture = GetMoisture(x, z);
    if (moisture > (1.0f - Config::Water::Chance)) {
        // Calculate factor 0..1 based on how deep into the wet zone we are
        float factor = (moisture - (1.0f - Config::Water::Chance)) / Config::Water::Chance;
        
        // Smooth the hole shape (Smoothstep) to avoid sharp cliffs
        factor = factor * factor * (3.0f - 2.0f * factor);
        
        // Sink the terrain!
        y -= factor * Config::Water::Depth;
    }

    return y;
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
                float dz = tz - pos.y; // stored as x,y
                if (sqrt(dx*dx + dz*dz) < 5.0f) { 
                    valid = false; 
                    break;
                }
            }
            attempts++;
        }

        if (valid) {
            float h = GetExactHeight(tx, tz);
            // Check for Lagoon
            if (IsLagoon(tx, tz, h)) {
                // Don't spawn trees in water
                valid = false;
            } else {
                treePositions.push_back(glm::vec2(tx, tz));
                placedPositions.push_back(glm::vec2(tx, tz));
            }
        }
    }
    return treePositions;
}

glm::vec3 WorldGenerator::GetTerrainColor(float x, float z, float y) {
    // Organic Noise for Terrain Blending
    // Octave 1: Large Shapes (Biome-like)
    float n1 = SmoothNoise(x * 0.03f, z * 0.03f); 
    
    // Octave 2: Medium Detail
    float n2 = SmoothNoise(x * 0.1f, z * 0.1f);
    
    // Octave 3: Fine Grit
    float n3 = SmoothNoise(x * 0.5f, z * 0.5f);
    
    // Composite Noise (Weighted)
    float finalNoise = n1 * 0.6f + n2 * 0.3f + n3 * 0.1f;
    
    glm::vec3 baseColor;
    
    // Threshold for blending
    // Use a smooth mix instead of hard if/else for better transitions? 
    // For retro PS1 style, hard patches might be better, or slight dithering.
    // Let's use a slightly soft threshold.
    
    if (finalNoise > 0.55f) {
         // DIRT / MUD (Browns)
         baseColor = glm::vec3(0.35f, 0.28f, 0.18f); 
         
         // Add grit
         baseColor += glm::vec3(0.04f) * (n3 - 0.5f); 
    } else {
         // GRASS (Greens)
         // Varied green based on large noise
         float tone = n1; 
         baseColor = glm::mix(glm::vec3(0.1f, 0.25f, 0.1f), glm::vec3(0.15f, 0.35f, 0.15f), tone);
         
         // Add grit
         baseColor += glm::vec3(0.03f, 0.05f, 0.02f) * (n3 - 0.5f);
    }

    // Height darkening (Valleys darker)
    baseColor += glm::vec3(0.01f, (y + 5.0f) * 0.015f, 0.01f);
    
    return baseColor;
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
    return GetExactHeight_Impl(x, z);
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
            
            // 2. Calculate Base Colors
            glm::vec3 c00 = GetTerrainColor(x0, z0, y00);
            glm::vec3 c10 = GetTerrainColor(x1, z0, y10);
            glm::vec3 c01 = GetTerrainColor(x0, z1, y01);
            glm::vec3 c11 = GetTerrainColor(x1, z1, y11);
            
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
                
                // Expansion to avoid gaps/square edges (User Request)
                // "atraviese un poco el terreno"
                float pad = scale * 0.5f; // overlap by 50% of a tile? 
                
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
                // Simple distance check against all nearby trees
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
            glm::vec3 p1(x0, y00, z0);
            glm::vec3 p2(x0, y01, z1);
            glm::vec3 p3(x1, y10, z0);
            glm::vec3 n1 = glm::normalize(glm::cross(p2 - p1, p3 - p1));

            glm::vec3 p4(x1, y10, z0);
            glm::vec3 p5(x0, y01, z1);
            glm::vec3 p6(x1, y11, z1);
            glm::vec3 n2 = glm::normalize(glm::cross(p5 - p4, p6 - p4));

            data.vertices.push_back(Vertex{ p1, c00, glm::vec2(0.0f, 0.0f), n1 });
            data.vertices.push_back(Vertex{ p2, c01, glm::vec2(0.0f, 1.0f), n1 });
            data.vertices.push_back(Vertex{ p3, c10, glm::vec2(1.0f, 0.0f), n1 });

            data.vertices.push_back(Vertex{ p4, c10, glm::vec2(1.0f, 0.0f), n2 });
            data.vertices.push_back(Vertex{ p5, c01, glm::vec2(0.0f, 1.0f), n2 });
            data.vertices.push_back(Vertex{ p6, c11, glm::vec2(1.0f, 1.0f), n2 });
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

std::vector<Vertex> WorldGenerator::GetTreeTrunkMesh() {
    return LoadCachedMesh("assets/models/tree_trunk.txt");
}

std::vector<Vertex> WorldGenerator::GetTreeLeavesMesh() {
    return LoadCachedMesh("assets/models/tree_leaves.txt");
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
