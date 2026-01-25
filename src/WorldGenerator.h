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
    static std::vector<Vertex> GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale);
    
    // Optimized (Uses cached trees for shadows)
    static std::vector<Vertex> GenerateChunkTerrain(int chunkX, int chunkZ, int chunkSize, float scale, const std::vector<glm::vec2>& neighborTrees);

    // Returns a list of tree positions + scale (vec4) for a specific chunk
    static std::vector<glm::vec4> GenerateChunkTrees(int chunkX, int chunkZ, int chunkSize, float scale);
    
    // Helper: Returns deterministically generated tree positions for a chunk (used for shadows)
    static std::vector<glm::vec2> GetChunkTreeLocations(int chunkX, int chunkZ, int chunkSize, float scale);

    // Returns a static mesh for ONE tree (to be used with instancing)
    static std::vector<Vertex> GetTreeTrunkMesh();
    static std::vector<Vertex> GetTreeLeavesMesh();
    // Returns a simple quad mesh for blob shadows
    static std::vector<Vertex> GetShadowMesh();

    // Calculates SMOOTH height at a given (x, z) for physics/collision
    static float GetHeight(float x, float z);

    // Calculates QUANTIZED height for visual terrain rendering
    static float GetVisualHeight(float x, float z);

    // Returns terrain color at (x, z) - useful for particles
    static glm::vec3 GetTerrainColor(float x, float z, float y);

    // Calculates EXACT height on the terrain mesh (Interpolated)
    // Matches the triangulation (0,0)-(1,0)-(0,1) and (1,0)-(1,1)-(0,1)
    static float GetExactHeight(float x, float z) {
        // Assume default scale (1.0f) or pass it if variable
        float scale = 1.0f; 
        
        int gridX = (int)floor(x / scale);
        int gridZ = (int)floor(z / scale);
        
        float localX = x - gridX * scale;
        float localZ = z - gridZ * scale; // 0..1 inside the quad

        // Get heights of the 4 corners
        float y00 = GetVisualHeight((float)gridX, (float)gridZ);
        float y10 = GetVisualHeight((float)(gridX+1), (float)gridZ);
        float y01 = GetVisualHeight((float)gridX, (float)(gridZ+1));
        float y11 = GetVisualHeight((float)(gridX+1), (float)(gridZ+1));

        // Triangulation Split: (01)-(10)
        // Tri 1: (00)-(01)-(10) -> Triangle uses y00, y01, y10
        // Tri 2: (10)-(01)-(11) -> Triangle uses y10, y01, y11
        // Check which side of diagonal X + Z = 1 we are on... wait.
        // Diagonal connecting (0,1) and (1,0).
        // Equation for diagonal: z = -x + 1 OR x + z = 1.
        
        // Let's verify standard triangulation in GenerateChunkTerrain:
        // p1(x0, z0), p2(x0, z1), p3(x1, z0)  -> (0,0), (0,1), (1,0)
        // p4(x1, z0), p5(x0, z1), p6(x1, z1)  -> (1,0), (0,1), (1,1)
        
        float height = 0.0f;
        
        if (localX + localZ <= 1.0f) {
            // Triangle 1: (0,0), (0,1), (1,0)
            // Barycentric interpolation on right-angled triangle is linear plane equation.
            // Plane passing through (0,0,y00), (0,1,y01), (1,0,y10)
            
            // Vector local:
            // H(x,z) = y00 + (y10-y00)*x + (y01-y00)*z
            height = y00 + (y10 - y00) * localX + (y01 - y00) * localZ;
        } else {
             // Triangle 2: (1,1), (0,1), (1,0)
             // Plane passing through (1,1,y11), (0,1,y01), (1,0,y10)
             
             // H(x,z) = y11 + (y01-y11)*(1-x) + (y10-y11)*(1-z)
             height = y11 + (y01 - y11) * (1.0f - localX) + (y10 - y11) * (1.0f - localZ);
        }
        
        return height;
    }

    // Generates a simple noise texture (64x64)
    static std::vector<unsigned char> GenerateNoiseTexture(int width, int height);

private:
    static void Internal_AddTreeGeometry(float x, float y, float z, float trunkW, float trunkH, float leavesW, float leavesH, std::vector<Vertex>& vertices);
};
