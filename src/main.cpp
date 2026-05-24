#include <glad/glad.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/glm.hpp>
#include <SFML/Window/Mouse.hpp>
#include "WeaponSystem.h"
#include "ParticleSystem.h"
#include "ScentSystem.h"
#include "Monster.h"
#include "BirdSystem.h" // NEW
#include <ctime>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime> // For time()
#include <random> // NEW: For better RNG

#include "Monster.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "WindSystem.h"
#include "FootprintSystem.h"
#include "ChunkManager.h"
#include "ScentSystem.h"
#include "Config.h"

// Shader loader helper
GLuint LoadShader(const char* vertPath, const char* fragPath) {
    auto loadFile = [](const char* path) {
        FILE* f = fopen(path, "rb");
        if(!f) return std::string("");
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = (char*)malloc(len + 1);
        fread(buf, 1, len, f);
        buf[len] = 0;
        std::string s(buf);
        free(buf);
        fclose(f);
        return s;
    };

    std::string vStr = loadFile(vertPath);
    std::string fStr = loadFile(fragPath);

    if (vStr.empty()) std::cerr << "[Shader] Failed to read vertex shader: " << vertPath << std::endl;
    if (fStr.empty()) std::cerr << "[Shader] Failed to read fragment shader: " << fragPath << std::endl;

    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLint success;
    GLchar infoLog[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cerr << "[Shader] Vertex compile error (" << vertPath << "):\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cerr << "[Shader] Fragment compile error (" << fragPath << "):\n" << infoLog << std::endl;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        std::cerr << "[Shader] Link error (" << vertPath << " & " << fragPath << "):\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Helper to push a quad (2 triangles) to a buffer
void PushQuad(std::vector<float>& data, float x, float y, float w, float h) {
    // Triangle 1
    data.push_back(x);     data.push_back(y);     data.push_back(0);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0);
    data.push_back(x+w);   data.push_back(y);     data.push_back(0);
    // Triangle 2
    data.push_back(x+w);   data.push_back(y);     data.push_back(0);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0);
    data.push_back(x+w);   data.push_back(y+h);   data.push_back(0);
}

// Helper to draw a single digit using THICK QUADS (0-9)
void DrawDigitSolid(int d, float x, float y, float size, GLuint vao, GLuint vbo) {
    // Segments are now rectangles with width/thickness
    struct Seg { float x, y, w, h; };
    float t = 0.2f; // Thickness relative to size (0..1)
    
    // 7-segment layout (Horizontal: w=1, h=t. Vertical: w=t, h=0.5)
    //   0
    // 1   2
    //   3
    // 4   5
    //   6
    static const Seg segments[] = {
        {0,1-t, 1,t},    // 0 (Top)
        {0,0.5f, t,0.5f},// 1 (Top-Left)
        {1-t,0.5f, t,0.5f},// 2 (Top-Right)
        {0,0.5f-t/2, 1,t},// 3 (Middle)
        {0,0, t,0.5f},   // 4 (Bot-Left)
        {1-t,0, t,0.5f}, // 5 (Bot-Right)
        {0,0, 1,t}       // 6 (Bottom)
    };
    
    static const int digits[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,0}  // 9
    };

    std::vector<float> data;
    for(int i=0; i<7; i++) {
        if(digits[d][i]) {
            PushQuad(data, x + segments[i].x * size, y + segments[i].y * size, segments[i].w * size, segments[i].h * size);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/3));
}

// Helper to draw a rotating arrow
// Helper to draw a rotating arrow
void DrawArrow(float x, float y, float size, float angle, GLuint vao, GLuint vbo) {
    // Arrow shape pointing Right (0 radians)
    // Triangle Head: (0.5, 0), (-0.2, 0.3), (-0.2, -0.3)
    // Shaft: (-0.5, 0.1), (-0.2, 0.1), (-0.2, -0.1), (-0.5, -0.1)
    
    std::vector<float> data;
    auto pushRotated = [&](float lx, float ly) {
        // Rotate
        float rx = lx * cos(angle) - ly * sin(angle);
        float ry = lx * sin(angle) + ly * cos(angle);
        // Translate & Scale
        data.push_back(x + rx * size);
        data.push_back(y + ry * size); 
        data.push_back(0.0f);
    };

    auto addTri = [&](float x1, float y1, float x2, float y2, float x3, float y3) {
        pushRotated(x1, y1); pushRotated(x2, y2); pushRotated(x3, y3);
    };

    // Head
    addTri(0.5f, 0.0f, -0.2f, 0.3f, -0.2f, -0.3f);
    // Shaft (2 tris)
    addTri(-0.5f, -0.1f, -0.2f, -0.1f, -0.2f, 0.1f);
    addTri(-0.5f, -0.1f, -0.2f, 0.1f, -0.5f, 0.1f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/3));
}

// 3D Arrow (XZ Plane) for Wind Debug
void DrawArrow3D(glm::vec3 pos, float size, float angle, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    auto push = [&](float lx, float lz) {
        float rx = lx * cos(angle) - lz * sin(angle);
        float rz = lx * sin(angle) + lz * cos(angle);
        
        // Pos
        data.push_back(pos.x + rx * size);
        data.push_back(pos.y);
        data.push_back(pos.z + rz * size);
        
        // Color
        data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
        
        // UV, Norm
        data.push_back(0); data.push_back(0);
        data.push_back(0); data.push_back(1); data.push_back(0);
    };

    // Arrow pointing Right (0 rad) -> (1,0)
    // Tip
    push(1.0f, 0.0f); push(0.0f, 0.5f); push(0.0f, -0.5f);
    
    // Tail
    push(0.0f, 0.2f); push(-1.0f, 0.2f); push(-1.0f, -0.2f);
    push(0.0f, 0.2f); push(-1.0f, -0.2f); push(0.0f, -0.2f);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    
    // Setup Layout (Matches standard 3D layout)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0); // Pos
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // Col
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3); // Norm
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/11));
}

// Helper to draw a DONUT (Ring)
void DrawDonut(float cx, float cy, float cz, float rMin, float rMax, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    int segments = 64;
    float step = 6.28318f / segments;

    for (int i = 0; i < segments; i++) {
        float theta1 = i * step;
        float theta2 = (i + 1) * step;

        float c1 = cos(theta1); float s1 = sin(theta1);
        float c2 = cos(theta2); float s2 = sin(theta2);
        
        // Vertex helper for adding simple Position+Color+UV+Normal struct
        auto addVert = [&](float r, float c, float s) {
            data.push_back(cx + c * r); data.push_back(cy); data.push_back(cz + s * r); // Pos
            data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);  // Color
            data.push_back(0); data.push_back(0); // UV
            data.push_back(0); data.push_back(1); data.push_back(0); // Normal
        };
        
        // Tri 1
        addVert(rMin, c1, s1);
        addVert(rMax, c1, s1);
        addVert(rMin, c2, s2);
        
        // Tri 2
        addVert(rMax, c1, s1);
        addVert(rMax, c2, s2);
        addVert(rMin, c2, s2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    // Ensure Attributes are set up for "Vertex" layout!
    // Stride = 11 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0); // Pos
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // Color
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3); // Norm
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/11));
}

// Helper to draw a single 3D Line
void DrawLine(glm::vec3 start, glm::vec3 end, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    // Vertex format matching DrawDonut for simplicity (Pos + Color + UV + Normal)
    // P1
    data.push_back(start.x); data.push_back(start.y); data.push_back(start.z);
    data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
    data.push_back(0); data.push_back(0); // UV
    data.push_back(0); data.push_back(1); data.push_back(0); // Normal
    
    // P2
    data.push_back(end.x); data.push_back(end.y); data.push_back(end.z);
    data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
    data.push_back(0); data.push_back(0);
    data.push_back(0); data.push_back(1); data.push_back(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Use GL_LINES instead of triangles
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0); // Pos
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1); // Color
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3); // Norm
    
    glDrawArrays(GL_LINES, 0, 2);
}
#include <memory>
struct GameConfig {
    bool isNight = false;
    float darkness = 0.08f;
    int monsterCount = 1;
};

#include <fstream>
#include <sstream>
#include <algorithm>

GameConfig LoadConfig(const std::string& filename) {
    GameConfig config;
    std::vector<std::string> paths = {
        filename,                  // config.json (CWD)
        "../" + filename,          // ../config.json
        "../../" + filename,       // ../../config.json
        "bin/" + filename          // bin/config.json
    };
    
    std::string foundPath = "";
    for (const auto& p : paths) {
        std::ifstream file(p);
        if (file.is_open()) {
            foundPath = p;
            file.close();
            break;
        }
    }
    
    if (foundPath.empty()) {
        foundPath = filename;
        std::ofstream outFile(foundPath);
        if (outFile.is_open()) {
            outFile << "{\n";
            outFile << "  \"isNight\": false,\n";
            outFile << "  \"darkness\": 0.08,\n";
            outFile << "  \"monsterCount\": 1\n";
            outFile << "}\n";
            outFile.close();
        }
    }
    
    std::ifstream file(foundPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("isNight") != std::string::npos) {
                if (line.find("true") != std::string::npos) {
                    config.isNight = true;
                } else if (line.find("false") != std::string::npos) {
                    config.isNight = false;
                }
            }
            if (line.find("darkness") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> config.darkness;
                }
            }
            if (line.find("monsterCount") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> config.monsterCount;
                }
            }
        }
        file.close();
        std::cout << "[Config] Loaded configuration from: " << foundPath 
                  << " (isNight: " << (config.isNight ? "true" : "false") 
                  << ", darkness: " << config.darkness 
                  << ", monsterCount: " << config.monsterCount << ")" << std::endl;
    }
    return config;
}

int main() {
    GameConfig gameCfg = LoadConfig("config.json");
    glm::vec4 skyColor = gameCfg.isNight ? glm::vec4(0.005f, 0.005f, 0.015f, 1.0f) : glm::vec4(0.4f, 0.6f, 1.0f, 1.0f);

    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 0;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    // Use Borderless Fullscreen
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Window window(desktop, "GamePS1Horror", sf::Style::None, settings);
    window.setVerticalSyncEnabled(Config::Graphics::VSyncEnabled);
    window.setMouseCursorVisible(false);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, desktop.width, desktop.height);
    float aspect = (float)desktop.width / (float)desktop.height;

    GLuint shaderProgram = LoadShader("assets/shaders/ps1.vert", "assets/shaders/ps1.frag");
    GLuint uiProgram = LoadShader("assets/shaders/ui.vert", "assets/shaders/ui.frag");

    // Noise Texture
    std::vector<unsigned char> textureData = WorldGenerator::GenerateNoiseTexture(64, 64);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Soft Shadow Texture (Radial Gradient)
    int shadowSize = 64;
    std::vector<unsigned char> shadowData(shadowSize * shadowSize * 3);
    for (int y = 0; y < shadowSize; y++) {
        for (int x = 0; x < shadowSize; x++) {
            float dx = (float)x / shadowSize - 0.5f;
            float dy = (float)y / shadowSize - 0.5f;
            float dist = sqrt(dx*dx + dy*dy) * 2.0f; // 0 at center, 1 at edge
            
            // Multiply Logic (White=Transp, Grey=Darker)
            float value = glm::smoothstep(0.0f, 1.0f, dist); 
            value = 0.4f + 0.6f * value; // Center=0.4, Edge=1.0
            if (value > 1.0f) value = 1.0f;

            unsigned char val = (unsigned char)(value * 255);
            int idx = (y * shadowSize + x) * 3;
            shadowData[idx] = val;
            shadowData[idx+1] = val;
            shadowData[idx+2] = val;
        }
    }
    GLuint shadowTexID;
    glGenTextures(1, &shadowTexID);
    glBindTexture(GL_TEXTURE_2D, shadowTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowSize, shadowSize, 0, GL_RGB, GL_UNSIGNED_BYTE, shadowData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // White Texture for UI & Shadows
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Fix alignment for 1x1 buffer
    GLuint whiteTexID;
    glGenTextures(1, &whiteTexID);
    glBindTexture(GL_TEXTURE_2D, whiteTexID);
    unsigned char whitePixel[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Seed Randomness (MUST BE BEFORE LOADING WORLD)
    srand((unsigned int)time(NULL));
    WorldGenerator::SetSeed(rand()); 

    // Render Distance Increased (User Request)
    ChunkManager chunkManager(Config::World::RenderDistance); 

    // SKY COLOR (Clear Color)
    glClearColor(skyColor.r, skyColor.g, skyColor.b, skyColor.a);
    
    float globalTime = 0.0f;
    bool debugCam = false;
    bool showSpawnArea = false; // New toggle
    bool showHitboxes = false; // Toggle for tree hitboxes (H key)
    glm::vec3 freeCamPos(0, 50, 0);
    glm::vec3 freeCamFront(0, -1, 0);
    float freeCamYaw = -90.0f;
    float freeCamPitch = -45.0f;



    // SKY COLOR (Clear Color)
    glClearColor(skyColor.r, skyColor.g, skyColor.b, skyColor.a);

    // Shadow Setup
    auto shadowMesh = WorldGenerator::GetShadowMesh();
    GLuint shadowVAO, shadowVBO;
    glGenVertexArrays(1, &shadowVAO);
    glGenBuffers(1, &shadowVBO);
    glBindVertexArray(shadowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
    glBufferData(GL_ARRAY_BUFFER, shadowMesh.size() * sizeof(Vertex), shadowMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2); // Needed for gradient

    // Tree Setup (Structural Separation)
    // 1. TRUNK (Rigid)
    auto trunkMesh = WorldGenerator::GetTreeTrunkMesh();
    GLuint trunkVAO, trunkVBO;
    glGenVertexArrays(1, &trunkVAO);
    glGenBuffers(1, &trunkVBO);
    glBindVertexArray(trunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trunkVBO);
    glBufferData(GL_ARRAY_BUFFER, trunkMesh.size() * sizeof(Vertex), trunkMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    
    // 2. LEAVES (Swaying)
    auto leavesMesh = WorldGenerator::GetTreeLeavesMesh();
    GLuint leavesVAO, leavesVBO;
    glGenVertexArrays(1, &leavesVAO);
    glGenBuffers(1, &leavesVBO);
    glBindVertexArray(leavesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, leavesVBO);
    glBufferData(GL_ARRAY_BUFFER, leavesMesh.size() * sizeof(Vertex), leavesMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);

    // Instance Buffer (Shared)
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // CLEAN SLATE: Removed Initial Instance Attribute Setup for Trunks & Leaves
    // ChunkManager now manages Attrib 4 exclusively.
    // This prevents "Double Setup" conflicts.

    // Setup Instance Attribute for Shadows
    glBindVertexArray(shadowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribDivisor(4, 1); 

    // UI Setup
    GLuint uiVAO, uiVBO;
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // -- RETRO RESOLUTION SETUP --
    // Increased to 640x480 (VGA) for less extreme pixelation, while keeping fixed retro aspect
    int const INTERNAL_W = Config::Graphics::InternalWidth;
    int const INTERNAL_H = Config::Graphics::InternalHeight;
    float const INTERNAL_ASPECT = (float)INTERNAL_W / (float)INTERNAL_H;

    // FBO
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Texture Attachment
    GLuint texColorBuffer;
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, INTERNAL_W, INTERNAL_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

    // RBO (Depth/Stencil)
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, INTERNAL_W, INTERNAL_H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Screen Quad Resources
    GLuint screenShader = LoadShader("assets/shaders/screen.vert", "assets/shaders/screen.frag");
    float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    // ...
    
    GLuint debugVAO, debugVBO;
    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &debugVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    WindSystem windSystem;
    Player player(glm::vec3(0.0f, 10.0f, 0.0f));
    FootprintSystem footprints;
    WeaponSystem weapon;           // NEW
    bool showMonsterMarker = false;
    bool isGameOver = false;
    float gameOverTimer = 0.0f;
    ParticleSystem particles;
    ScentSystem scentSystem;
    std::vector<std::unique_ptr<Monster>> monsters; 
    
    // Bird System (Sparrows)
    BirdSystem birds;

    // Chunk Manager - NOW needs bird system
    // ChunkManager was already instantiated at top of main
    chunkManager.SetBirdSystem(&birds);
    chunkManager.Init(); // Load world NOW, after birds are hooked up
    
    chunkManager.Update(player.Position); // Initial load 

    // Procedural Spawning ("The Donut" + Tree Collision Check)
    {
        // CONSTANTS (Must match ChunkManager)
        const int C_SIZE = Config::World::ChunkSize;
        const float C_SCALE = Config::World::ChunkScale;
        
        // Modern RNG Setup
    srand(time(NULL)); // Legacy RNG for simpler rand() calls
    std::random_device rd;
    std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(0.0f, 360.0f); // Degrees
        std::uniform_real_distribution<float> radiusDist(Config::Gameplay::MonsterSpawnMinRadius, Config::Gameplay::MonsterSpawnMaxRadius);
        std::uniform_real_distribution<float> coordDist(-150.0f, 150.0f); // For player

        auto IsPositionCurrentSafe = [&](float x, float z) {
            int cx = (int)floor(x / (C_SIZE * C_SCALE));
            int cz = (int)floor(z / (C_SIZE * C_SCALE));
            
            // Check trees in 3x3 chunks to be safe
            for(int dx=-1; dx<=1; dx++) {
                for(int dz=-1; dz<=1; dz++) {
                     auto trees = WorldGenerator::GetChunkTreeLocations(cx+dx, cz+dz, C_SIZE, C_SCALE);
                     for(const auto& t : trees) {
                         // Replicating scale logic from WorldGenerator:
                         float scaleNoise = sin(t.x * 12.9898 + t.y * 78.233) * 43758.5453;
                         scaleNoise = scaleNoise - floor(scaleNoise);
                         float tScale = 0.8f + scaleNoise * 0.7f;
                         
                         float treeRadius = 0.6f * tScale; // The AABB half-width
                         
                         float dist = sqrt(pow(x - t.x, 2) + pow(z - t.y, 2));
                         if(dist < (treeRadius + 0.5f)) return false; // Too close
                     }
                }
            }
            return true;
        };

        // 1. Spawn Player
        int attempts = 0;
        glm::vec3 pPos;
        do {
            float px = coordDist(gen);
            float pz = coordDist(gen);
            pPos = glm::vec3(px, WorldGenerator::GetHeight(px, pz) + 1.0f, pz);
            attempts++;
        } while(!IsPositionCurrentSafe(pPos.x, pPos.z) && attempts < 100);
        player.Position = pPos;

        // 2. Spawn Monsters (Donut Distribution)
        for (int mIndex = 0; mIndex < gameCfg.monsterCount; ++mIndex) {
            attempts = 0;
            glm::vec3 mPos;
            do {
                 // Random Angle
                 float angleDeg = angleDist(gen);
                 float angleRad = glm::radians(angleDeg);
                 
                 // Random Radius (200 - 300)
                 float dist = radiusDist(gen);
                 
                 float mx = pPos.x + cos(angleRad) * dist;
                 float mz = pPos.z + sin(angleRad) * dist;
                 
                 mPos = glm::vec3(mx, WorldGenerator::GetHeight(mx, mz), mz);
                 attempts++;
            } while(!IsPositionCurrentSafe(mPos.x, mPos.z) && attempts < 100);
            
            auto monster = std::make_unique<Monster>(mPos);
            monster->LookAt(player.Position);
            monsters.push_back(std::move(monster));
        }
    }

    sf::Clock clock;
    sf::Clock fpsClock;
    int frameCount = 0;
    int currentFPS = 0;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        if (deltaTime > 0.1f) deltaTime = 0.1f; 

        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            currentFPS = frameCount;
            frameCount = 0;
            fpsClock.restart();
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();
                
                if (!debugCam) {
                    // ...
                } else {
                     // Debug Camera Movement
                }
                
                // Manual Wind Control (Arrow Keys - Rotation)
                // Use isKeyPressed for smooth holding
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    // Rotate Counter-Clockwise
                    float angle = -2.0f * deltaTime; // Speed
                    glm::vec2 current = windSystem.GetDirection();
                    float c = cos(angle); float s = sin(angle);
                    float nx = current.x * c - current.y * s;
                    float ny = current.x * s + current.y * c;
                    windSystem.SetDirection(nx, ny);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    // Rotate Clockwise
                    float angle = 2.0f * deltaTime;
                    glm::vec2 current = windSystem.GetDirection();
                    float c = cos(angle); float s = sin(angle);
                    float nx = current.x * c - current.y * s;
                    float ny = current.x * s + current.y * c;
                    windSystem.SetDirection(nx, ny);
                }
                
                // Weapon Reload (R Key)
                if (event.key.code == sf::Keyboard::R) {
                    weapon.Reload();
                }

                // Debug Camera Toggle
                if (event.key.code == sf::Keyboard::F3) debugCam = !debugCam;
                
                // Spawn Area Toggle (Only in F3)
                if (debugCam && event.key.code == sf::Keyboard::G) showSpawnArea = !showSpawnArea;

                // Hitbox Toggle (H key)
                if (event.key.code == sf::Keyboard::H) {
                    showHitboxes = !showHitboxes;
                    std::cout << "H Key Pressed! Toggle: " << (showHitboxes ? "ON" : "OFF") << std::endl;
                }
                
                // Bird Debug Toggle (J key)
                if (event.key.code == sf::Keyboard::J) {
                    birds.ToggleDebug();
                    std::cout << "J Key Pressed! Toggled Bird Debug" << std::endl;
                }
                
                // Toggle Monster Marker (O key)
                if (event.key.code == sf::Keyboard::O) {
                    showMonsterMarker = !showMonsterMarker;
                    std::cout << "O Key Pressed! Monster Marker: " << (showMonsterMarker ? "ON" : "OFF") << std::endl;
                }
            }
        }

        chunkManager.Update(player.Position);
        // Wind System Update
        windSystem.Update(deltaTime);
        glm::vec2 windDir = windSystem.GetDirection();
        float windStrength = 1.0f; 
        
        // SCENT UPDATE
        // Pass 3D wind if needed, but Wind is 2D. Convert to 3D (x,0,y) inside or pass 2D.
        // ScentSystem expects vec3 windDir + float speed
        scentSystem.Update(deltaTime, player.Position, glm::vec3(windDir.x, 0, windDir.y), windStrength);

        weapon.Update(deltaTime, windDir, windStrength, chunkManager, footprints, particles, monsters);
        particles.Update(deltaTime);

        // Monster Update (Now takes ScentSystem, playerFront, Velocity, Weapon Ammo & Reloading State)
        for (auto& mPtr : monsters) {
            mPtr->Update(deltaTime, player.Position, player.Front, windDir, 
                        chunkManager, scentSystem, particles,
                        player.Velocity, weapon.GetAmmo(), weapon.IsReloading());
        }

        // GAME OVER CHECK & TIMING
        if (isGameOver) {
            gameOverTimer -= deltaTime;
            if (gameOverTimer <= 0.0f) {
                exit(0);
            }
        } else {
            for (const auto& mPtr : monsters) {
                if (mPtr->IsDead()) continue;
                glm::vec3 mPos = mPtr->GetPosition();
                glm::vec2 diff2D = glm::vec2(player.Position.x - mPos.x, player.Position.z - mPos.z);
                float dist2D = glm::length(diff2D);
                float heightDiff = abs((player.Position.y - 1.6f) - mPos.y);
                
                if (dist2D < 1.5f && heightDiff < 3.0f) {
                    isGameOver = true;
                    gameOverTimer = 1.5f;
                    std::cout << "GAME OVER! Spawning blood explosion..." << std::endl;
                    for (int i = 0; i < 40; ++i) {
                        glm::vec3 velocity((rand()%100/50.0f - 1.0f)*5.0f, (rand()%100/50.0f - 0.3f)*6.0f, (rand()%100/50.0f - 1.0f)*5.0f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, -0.6f, 0), velocity, glm::vec4(0.8f, 0.0f, 0.0f, 1.0f), 0.15f, 1.5f, -9.8f);
                    }
                    break;
                }
            }
        }
        
        // Update Birds
        std::vector<glm::vec3> monsterPositions;
        for (const auto& mPtr : monsters) {
            monsterPositions.push_back(mPtr->GetPosition());
        }
        birds.Update(deltaTime, player.Position, monsterPositions);
        birds.CleanupDistantBirds(player.Position, 80.0f); // Optimization
        
        if (!debugCam) {
            if (!isGameOver) {
                player.ProcessKeyboard(0, deltaTime, chunkManager, footprints);
            }
            player.Update(deltaTime);
        } else {
            // Free Cam Movement
            float camSpeed = Config::Gameplay::DebugCamSpeed * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) freeCamPos += freeCamFront * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) freeCamPos -= freeCamFront * camSpeed;
            glm::vec3 camRight = glm::normalize(glm::cross(freeCamFront, glm::vec3(0,1,0)));
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) freeCamPos -= camRight * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) freeCamPos += camRight * camSpeed;
        }

        if (window.hasFocus()) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2i center(desktop.width / 2, desktop.height / 2);
            sf::Mouse::setPosition(center, window);
            float xoff = (float)(mousePos.x - center.x);
            float yoff = (float)(center.y - mousePos.y);

            if (!debugCam) {
                player.ProcessMouseMovement(xoff, yoff);
            } else {
                freeCamYaw += xoff * 0.1f;
                freeCamPitch += yoff * 0.1f;
                if (freeCamPitch > 89.0f) freeCamPitch = 89.0f;
                if (freeCamPitch < -89.0f) freeCamPitch = -89.0f;
                glm::vec3 f;
                f.x = cos(glm::radians(freeCamYaw)) * cos(glm::radians(freeCamPitch));
                f.y = sin(glm::radians(freeCamPitch));
                f.z = sin(glm::radians(freeCamYaw)) * cos(glm::radians(freeCamPitch));
                freeCamFront = glm::normalize(f);
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                weapon.TryFire(player.Position, player.Front, particles, monsters);
            }
        }

        footprints.Update(deltaTime);
        globalTime += deltaTime;

        // Draw Loop
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, INTERNAL_W, INTERNAL_H);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Time"), globalTime);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), (float)INTERNAL_W, (float)INTERNAL_H);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Snap"), 1); // ENABLED SNAPPING 
        
        // FOG CONFIGURATION
        glUniform1f(glGetUniformLocation(shaderProgram, "u_FogStart"), Config::World::FogDistStart);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_FogEnd"), Config::World::FogDistEnd);
        
        // DAY/NIGHT & FLASHLIGHT CONFIGURATION
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsNight"), gameCfg.isNight ? 1 : 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Darkness"), gameCfg.darkness);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_PlayerPos"), player.Position.x, player.Position.y, player.Position.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_PlayerFront"), player.Front.x, player.Front.y, player.Front.z);
        glm::vec3 fogCol = gameCfg.isNight ? glm::vec3(0.005f, 0.005f, 0.015f) : glm::vec3(0.4f, 0.6f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_FogColor"), fogCol.r, fogCol.g, fogCol.b);

        glm::mat4 view = debugCam ? glm::lookAt(freeCamPos, freeCamPos + freeCamFront, glm::vec3(0,1,0)) : player.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)INTERNAL_ASPECT, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        // FRUSTUM CULLING UPDATE
        chunkManager.UpdateVisibility(projection * view);

        glUniform2f(glGetUniformLocation(shaderProgram, "u_WindDirection"), windDir.x, windDir.y);

        // 1. Terrain Render
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC TERRAIN
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f); // RESET ALPHA STATE (Critical Fix)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID); // Noise texture
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
        chunkManager.RenderTerrain(shaderProgram);

        // 2. Tree & Shadow Collection
        // (Shadows remain manual? No, Shadows were instanced too. 
        // Wait, the plan was to move TREES to batches. Shadows are separate?
        // Let's check the code. "2. Tree & Shadow Collection"
        // It collected treePositions and drew Trunks, Leaves.
        // It did NOT draw shadows here?
        // Ah, looking at code: `CollectAllTreePositions` -> `treePositions`.
        // Then drawn Trunks and Leaves.
        // Where are shadows drawn?
        // Shadows were drawn earlier? No. 
        // Wait, "Setup Instance Attribute for Shadows" was in setup.
        // But actual Draw call?
        // I don't see a Draw call for Shadows in the provided snippet of main.cpp around line 770.
        // Let's look at the snippet Step 80 again.
        // Line 769: // 2. Tree & Shadow Collection
        // Line 770: chunkManager.CollectAllTreePositions(treePositions);
        // Line 783: // PASS A: TRUNKS
        // Line 790: // PASS B: LEAVES
        // It seems SHADOWS were NOT drawn in that block? 
        // Ah, maybe I missed it. OR maybe Shadows are drawn as part of the Trunks? No.
        // Let's assuming the original code only drew Trunks and Leaves there.
        // I will replace that block with RenderTrees.
        
        // However, I need to make sure I pass the correct counts.
        // trunkMesh.size() and leavesMesh.size().
        
        chunkManager.RenderTrees(shaderProgram, trunkVAO, leavesVAO, (int)trunkMesh.size(), (int)leavesMesh.size());

        // 3. Footprints
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0); // DISABLED: Using CPU Exact Height
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC FOOTPRINTS
        footprints.Render(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);

        glm::vec3 activeCamPos = debugCam ? freeCamPos : player.Position;
        if (!debugCam && player.IsGrounded) {
             if (player.HeadBobTimer > 0.001f) {
                 // Walking Head Bob
                 activeCamPos.y += sin(player.HeadBobTimer) * player.HeadBobAmount;
             } else {
                 // Idle Breathing
                 activeCamPos.y += sin(player.BreathTimer) * player.BreathAmount;
             }
        }

        particles.Render(shaderProgram, activeCamPos);
        
        // 4. Monster Render (Normal)
        // Explicitly bind noise texture to match terrain style (Render will switch to white for eyes)
        for (auto& mPtr : monsters) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            mPtr->Render(shaderProgram, whiteTexID);
        }
        
        // Render Birds (Using noise texture)
        glBindTexture(GL_TEXTURE_2D, textureID);
        birds.Render(shaderProgram);
        
        // --- SAFETY RESET (Fix for State Leakage) ---
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_UseBirdAttribs"), 0); // RESET BIRD MODE
        
        glBindVertexArray(0); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // GLOBAL SAFETY: Disable ALL attributes 0-7
        for(int i=0; i<8; i++) glDisableVertexAttribArray(i);
        
        // --- 4b. Render Water (Transparent) ---
        // Render LAST for correct blending with terrain/trees
        // Use Noise Texture for water surface details (if any) or just color
        glBindTexture(GL_TEXTURE_2D, textureID);
        chunkManager.RenderWater(shaderProgram);

        // 5. Render Projectiles (World Space) - VISIBLE IN NORMAL MODE
        // Reuse debug VBO or creating a new one?
        static GLuint projVAO=0, projVBO=0;
        if(projVAO==0) { glGenVertexArrays(1, &projVAO); glGenBuffers(1, &projVBO); }
        
        // Fix Visibility: Bind White Texture and ensure simple uniform state
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f))); // RESET MODEL MATRIX!
        glBindTexture(GL_TEXTURE_2D, whiteTexID);
        
        weapon.RenderProjectiles(shaderProgram, projVAO, projVBO);

        // Monster Marker (O Key)
        if (showMonsterMarker) {
            glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
            glBindTexture(GL_TEXTURE_2D, whiteTexID);
            
            glDisable(GL_DEPTH_TEST); // Draw on top of everything
            
            for (const auto& mPtr : monsters) {
                glm::vec3 mPos = mPtr->GetPosition();
                // Draw a high-visibility vertical red beacon line
                DrawLine(mPos, mPos + glm::vec3(0.0f, 200.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), debugVAO, debugVBO);
                // Draw a red horizontal cross on the ground to pinpoint exact position
                DrawLine(mPos - glm::vec3(2.5f, 0.0f, 0.0f), mPos + glm::vec3(2.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), debugVAO, debugVBO);
                DrawLine(mPos - glm::vec3(0.0f, 0.0f, 2.5f), mPos + glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(1.0f, 0.0f, 0.0f), debugVAO, debugVBO);
            }
            
            glEnable(GL_DEPTH_TEST); // Restore depth test
        }

        // 6. DEBUG HITBOXES
        if (showHitboxes) {
            // Using same state as projectiles (White Tex, No instance, No terrain conform)
            glm::vec3 activeCamPos = debugCam ? freeCamPos : player.Position;
            chunkManager.RenderDebug(shaderProgram, activeCamPos);
        }
        
        // Debug Highlights (Always on top)
        static bool wasDebug = false;
        
        if (debugCam) {
            wasDebug = true;
            // Render from FreeCam Perspective
            // World
            chunkManager.RenderTerrain(shaderProgram);
            
            // Visualization of Scent (Trapezoids)
            scentSystem.RenderDebug(shaderProgram, activeCamPos);
            
            // Wind Indicator (Blue Arrow above player)
            glm::vec2 w = windSystem.GetDirection();
            float wAngle = atan2(w.y, w.x); // Radians
            // Draw Arrow 3 meters above player
            DrawArrow3D(glm::vec3(player.Position.x, player.Position.y + 3.0f, player.Position.z), 
                      1.0f, wAngle, glm::vec3(0,0,1), debugVAO, debugVBO); 
            
            player.RenderDebug(shaderProgram);
            for (auto& mPtr : monsters) {
                mPtr->RenderDebug(shaderProgram);
            }

            
            if (showSpawnArea) {
                // Use a dynamic VBO for debug drawing (can reuse shadowVBO or create a dedicated debug one?)
                // Let's create a quick one-off VAO/VBO for this tool to be safe and clean.
                static GLuint dbgVAO = 0, dbgVBO = 0;
                if (dbgVAO == 0) {
                     glGenVertexArrays(1, &dbgVAO);
                     glGenBuffers(1, &dbgVBO);
                }
                
                glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
                glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
                glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
                glBindTexture(GL_TEXTURE_2D, whiteTexID);
                
                // Yellow semi-transparent
                // Note: Shader likely doesn't support transparency well without sorting or specific blend modes,
                // but we can try basic addition.
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Use the configured radii
                DrawDonut(player.Position.x, player.Position.y + 0.1f, player.Position.z, 
                          Config::Gameplay::MonsterSpawnMinRadius, 
                          Config::Gameplay::MonsterSpawnMaxRadius, 
                          glm::vec3(1.0f, 1.0f, 0.0f), dbgVAO, dbgVBO);
                          
                glDisable(GL_BLEND);
            }
            
            // Render Range Lines
            // Reuse the dynamic VBO (dbgVBO) and White Texture
            static GLuint dbgVAO2 = 0, dbgVBO2 = 0;
            if (dbgVAO2 == 0) {
                 glGenVertexArrays(1, &dbgVAO2);
                 glGenBuffers(1, &dbgVBO2);
            }
            glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
            glBindTexture(GL_TEXTURE_2D, whiteTexID);
            
            // 2. Vision Range (Blue) - 1000.0 units (Far Plane)
            // Drawn slightly higher
            glm::vec3 startPos = player.Position + glm::vec3(0, -0.5f, 0); 
            glm::vec3 visionEnd = startPos + player.Front * 1000.0f;
            DrawLine(startPos + glm::vec3(0, 0.1f, 0), visionEnd, glm::vec3(0.0f, 0.0f, 1.0f), dbgVAO2, dbgVBO2);
        } else if (wasDebug) {
             // TRANSITION: Just exited debug mode. Clear buffers ONCE.
             wasDebug = false;
             
             // CLEAR Debug VBOs to prevent "Ghost" geometry if toggle off
             glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
             glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        }

        // 5. Weapon (Overlay - Clear Depth)
        glClear(GL_DEPTH_BUFFER_BIT); 
        glm::mat4 viewIdentity = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(viewIdentity));
        
        // Use NOISE texture for weapon
        glBindTexture(GL_TEXTURE_2D, textureID);
        weapon.Render(shaderProgram);

        // 4. UI Pass (SEPARATE SHADER PASS - IN FBO FOR RETRO LOOK)
        glDisable(GL_DEPTH_TEST);
        glDepthRange(0, 0.01); // Force Near
        glUseProgram(uiProgram); // Switch to specialized UI shader
        
        // NUCLEAR STATE RESET
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(uiVAO); 
        glDisableVertexAttribArray(1); 
        glDisableVertexAttribArray(2); 
        glDisableVertexAttribArray(3); 
        glDisableVertexAttribArray(4); 
        
        // Force safe defaults
        glVertexAttrib2f(2, 0.0f, 0.0f); 
        
        glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(uiProgram, "u_Texture"), 0);
        glBindTexture(GL_TEXTURE_2D, whiteTexID);

        // Crosshair
        std::vector<float> chData;
        float chS = 0.03f; float chT = 0.004f;
        // Fix aspect for UI in 320x240
        PushQuad(chData, -chS, -chT, chS*2, chT*2); 
        PushQuad(chData, -chT, -chS*INTERNAL_ASPECT, chT*2, chS*INTERNAL_ASPECT*2); 
        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, chData.size() * sizeof(float), chData.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(chData.size()/3));

        // FPS Counter
        std::string fpsStr = std::to_string(currentFPS);
        float charX = 0.8f; 
        float charSize = 0.06f; 
        for(char c : fpsStr) {
            DrawDigitSolid(c - '0', charX, 0.85f, charSize, uiVAO, uiVBO); 
            charX += charSize * 1.3f; 
        }

        // Ammo Counter (Bottom Right)
        int ammo = weapon.GetAmmo();
        bool showAmmo = true;
        if (weapon.IsReloading()) {
            showAmmo = (((int)(globalTime * 4.0f)) % 2 == 0); // Blink at 4Hz
        }
        if (showAmmo) {
            glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 0.8f, 0.1f, 0.1f); // Red digit
            DrawDigitSolid(ammo, 0.82f, -0.85f, 0.08f, uiVAO, uiVBO);
        }
        // Restore white color
        glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);

        // Wind Arrow
        float windAngle = atan2(windDir.y, windDir.x);
        float playerYawRad = glm::radians(player.Yaw);
        // Relative Angle: PlayerYaw - WindAngle + 90deg (Offset for Arrow UP)
        float relativeAngle = playerYawRad - windAngle + 1.5708f; 
        DrawArrow(0.9f, 0.1f, 0.05f, relativeAngle, uiVAO, uiVBO); 

        glEnable(GL_DEPTH_TEST);
        glDepthRange(0, 1.0); // Restore Depth

        // =================================================================================
        // PASS 2: UPSCALE TO SCREEN
        // =================================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default
        glViewport(0, 0, desktop.width, desktop.height); 
        
        // Debug: Clear to BLACK to distinguish "Quad Failed" (Black) from "FBO Empty" (Sky Blue)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT); 

        glDisable(GL_DEPTH_TEST); // IMPORTANT: Disable depth for 2D Quad
        glUseProgram(screenShader);
        glBindVertexArray(quadVAO);
        // Bind FBO texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer); 
        glUniform1i(glGetUniformLocation(screenShader, "u_ScreenTexture"), 0);
        glUniform1i(glGetUniformLocation(screenShader, "u_IsGameOver"), isGameOver ? 1 : 0);
        glUniform1f(glGetUniformLocation(screenShader, "u_GameOverTime"), gameOverTimer);
        glUniform1f(glGetUniformLocation(screenShader, "u_Time"), globalTime);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Restore State for next frame's Pass 1
        glClearColor(skyColor.r, skyColor.g, skyColor.b, skyColor.a);
        glEnable(GL_DEPTH_TEST);

        window.display();
    }

    return 0;
}
