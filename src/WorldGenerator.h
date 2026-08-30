#pragma once

#include <vector>
#include <cmath>
#include <glm/glm.hpp>

// Define a simple Vertex structure
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct Tree {
    float x, z;
    float radius; // For collision
};

struct WorldData {
    std::vector<Vertex> vertices;
    std::vector<Vertex> waterVertices; // Transparent pass
    std::vector<Tree> trees;
};

class WorldGenerator {
public:
    static unsigned int GlobalSeed;
    static float OffsetX;
    static float OffsetZ;

    // Sets the global seed and generates random offsets for terrain noise
    static void SetSeed(unsigned int seed) {
        GlobalSeed = seed;
        srand(seed);
        // Generate large random offsets to shift the terrain "noise"
        OffsetX = (float)(rand() % 10000);
        OffsetZ = (float)(rand() % 10000);
    }
    // Generates a grid mesh for a specific 32x32 chunk
    // Original (Procedural)
    // Generates a grid mesh + water mesh + trees for a specific 32x32 chunk
    // Original (Procedural)
    static WorldData GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale);
    
    // Optimized (Uses cached trees for shadows)
    static WorldData GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale, const std::vector<glm::vec2>& neighborTrees);

    // Returns a list of tree positions + scale (vec4) for a specific chunk
    static std::vector<glm::vec4> GenerateChunkTrees(int chunkX, int chunkZ, int chunkSize, float scale);
    
    // Helper: Returns deterministically generated tree positions for a chunk (used for shadows)
    static std::vector<glm::vec2> GetChunkTreeLocations(int chunkX, int chunkZ, int chunkSize, float scale);

    // Returns a static mesh for ONE tree archetype (0=Oak, 1=Pine, 2=Birch, 3=Willow)
    static std::vector<Vertex> GetTreeTrunkMesh(int type = 0);
    static std::vector<Vertex> GetTreeLeavesMesh(int type = 0);
    // Returns a simple quad mesh for blob shadows
    static std::vector<Vertex> GetShadowMesh();

    // Helper: Returns mountain factor (0..1) for biome/ridge logic
    static float GetMountainFactor(float x, float z);

    // Calculates SMOOTH height at a given (x, z) including dynamic terrain deformation
    static float GetHeight(float x, float z);

    // Calculates untouched procedural base height without deformation
    static float GetBaseHeight(float x, float z);

    // Helper: Returns moisture value (0..1) for biome/lagoon logic
    static float GetMoisture(float x, float z);

    // Calculates QUANTIZED height for visual terrain rendering
    static float GetVisualHeight(float x, float z);

    // Returns terrain color at (x, z)
    static glm::vec3 GetTerrainColor(float x, float z, float y);
    static glm::vec3 GetTerrainColor(float x, float z, float y, const glm::vec3& normal);

    // Calculates EXACT height on the terrain mesh (Interpolated)
    static float GetExactHeight(float x, float z);
    
    // Check if a point is in a lagoon (Water + Low Height)
    static bool IsLagoon(float x, float z, float h);

    // Generates a simple noise texture (64x64)
    static std::vector<unsigned char> GenerateNoiseTexture(int width, int height);

private:
    static void Internal_AddTreeGeometry(float x, float y, float z, float trunkW, float trunkH, float leavesW, float leavesH, std::vector<Vertex>& vertices);
};

