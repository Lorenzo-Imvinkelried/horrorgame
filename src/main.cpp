#include <glad/glad.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLFW/glfw3.h>
#include <functional>
#else
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window/Mouse.hpp>
#endif
#include <glm/glm.hpp>
#include "core/PlatformInput.h"
#include "WeaponSystem.h"
#include "ParticleSystem.h"
#include "ScentSystem.h"
#include "Monster.h"
#include "BirdSystem.h" // NEW
#include "CritterSystem.h" // Butterflies, Fireflies, Frogs
#include "PassiveMob.h" // Forest Deer / Passive Mob
#include "combat/TargetingSystem.h"
#include "combat/DamageNumberSystem.h"
#include "ui/UIRenderer.h"
#include "ui/FontRenderer.h"
#include "world/ItemDropSystem.h"
#include "inventory/LootManager.h"
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
#include "HorrorPropsSystem.h"
#include "EnemyMob.h"
#include "WaterMonster.h"
#include "ProjectileSystem.h"
#include "Config.h"
#include "inventory/InventorySystem.h"
#include "combat/SpellSystem.h"
#include "world/SkinningSystem.h"
#include "world/WeatherSystem.h"
#include "world/StructureSystem.h"
#include "world/BuildingSystem.h"
#include "entities/Dragon.h"

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

#ifdef __EMSCRIPTEN__
    auto adaptForWebGL2 = [](std::string& src, bool isFragment) {
        size_t pos = src.find("#version 330 core");
        if (pos != std::string::npos) {
            std::string header = "#version 300 es\n";
            if (isFragment) {
                header += "precision highp float;\nprecision highp int;\nprecision mediump sampler2D;\n";
            } else {
                header += "precision highp float;\nprecision highp int;\n";
            }
            src.replace(pos, 17, header);
        }
    };
    adaptForWebGL2(vStr, false);
    adaptForWebGL2(fStr, true);
#endif

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
    // Formato 5 floats: (x, y, z, u, v)
    // Triangle 1
    data.push_back(x);     data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x+w);   data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    // Triangle 2
    data.push_back(x+w);   data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x+w);   data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
}

// Helper to draw a rotating arrow
void DrawArrow(float x, float y, float size, float angle, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    auto pushRotated = [&](float lx, float ly) {
        // Rotate
        float rx = lx * cos(angle) - ly * sin(angle);
        float ry = lx * sin(angle) + ly * cos(angle);
        // Translate & Scale (5 floats: x, y, z, u, v)
        data.push_back(x + rx * size);
        data.push_back(y + ry * size); 
        data.push_back(0.0f);
        data.push_back(0.0f);
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
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/5));
    glBindVertexArray(0);
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
    bool flashlightEnabled = true;
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
            outFile << "  \"monsterCount\": 1,\n";
            outFile << "  \"flashlightEnabled\": true,\n";
            outFile << "  \"baseFreqX\": 0.02,\n";
            outFile << "  \"baseFreqZ\": 0.02,\n";
            outFile << "  \"baseAmplitude\": 22.0,\n";
            outFile << "  \"detailFreqX\": 0.2,\n";
            outFile << "  \"detailFreqZ\": 0.1,\n";
            outFile << "  \"detailAmplitude\": 1.5,\n";
            outFile << "  \"monsterSpawnMinRadius\": 150.0,\n";
            outFile << "  \"monsterSpawnMaxRadius\": 260.0\n";
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
            if (line.find("flashlightEnabled") != std::string::npos) {
                if (line.find("true") != std::string::npos) {
                    config.flashlightEnabled = true;
                } else if (line.find("false") != std::string::npos) {
                    config.flashlightEnabled = false;
                }
            }
            if (line.find("baseFreqX") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::BaseFreqX;
                }
            }
            if (line.find("baseFreqZ") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::BaseFreqZ;
                }
            }
            if (line.find("baseAmplitude") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::BaseAmplitude;
                }
            }
            if (line.find("detailFreqX") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::DetailFreqX;
                }
            }
            if (line.find("detailFreqZ") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::DetailFreqZ;
                }
            }
            if (line.find("detailAmplitude") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Terrain::DetailAmplitude;
                }
            }
            if (line.find("monsterSpawnMinRadius") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Gameplay::MonsterSpawnMinRadius;
                }
            }
            if (line.find("monsterSpawnMaxRadius") != std::string::npos) {
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string valStr = line.substr(colon + 1);
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), ','), valStr.end());
                    valStr.erase(std::remove(valStr.begin(), valStr.end(), '}'), valStr.end());
                    std::stringstream ss(valStr);
                    ss >> Config::Gameplay::MonsterSpawnMaxRadius;
                }
            }
        }
        file.close();
        std::cout << "[Config] Loaded configuration from: " << foundPath 
                  << " (isNight: " << (config.isNight ? "true" : "false") 
                  << ", darkness: " << config.darkness 
                  << ", monsterCount: " << config.monsterCount 
                  << ", flashlightEnabled: " << (config.flashlightEnabled ? "true" : "false")
                  << ", spawnMin: " << Config::Gameplay::MonsterSpawnMinRadius
                  << ", spawnMax: " << Config::Gameplay::MonsterSpawnMaxRadius << ")" << std::endl;
    }
    return config;
}

struct CoutRedirector {
    std::streambuf* oldBuf;
    std::ofstream file;
    CoutRedirector(const std::string& filename) : file(filename) {
        oldBuf = std::cout.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cout << std::unitbuf; // Enable automatic flushing
    }
    ~CoutRedirector() {
        std::cout.rdbuf(oldBuf);
    }
};

int main() {
#ifndef __EMSCRIPTEN__
    CoutRedirector redirect("log.txt");
#endif
    GameConfig gameCfg = LoadConfig("config.json");
    glm::vec4 skyColor = gameCfg.isNight ? glm::vec4(0.005f, 0.005f, 0.015f, 1.0f) : glm::vec4(0.4f, 0.6f, 1.0f, 1.0f);

    int windowWidth = 1280;
    int windowHeight = 720;

#ifndef __EMSCRIPTEN__
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
    window.setMouseCursorVisible(true);
    PlatformInput::Init(&window);
    windowWidth = desktop.width;
    windowHeight = desktop.height;
#else
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "GamePS1Horror", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    PlatformInput::Init(window);
#endif

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, windowWidth, windowHeight);
    float aspect = (float)windowWidth / (float)windowHeight;

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

    // Tree Setup (4 Archetypes: 0=Oak, 1=Pine, 2=Birch, 3=Willow)
    GLuint trunkVAO[4], trunkVBO[4];
    GLuint leavesVAO[4], leavesVBO[4];
    int trunkVertexCount[4];
    int leavesVertexCount[4];

    for (int arch = 0; arch < 4; ++arch) {
        auto tMesh = WorldGenerator::GetTreeTrunkMesh(arch);
        trunkVertexCount[arch] = (int)tMesh.size();
        glGenVertexArrays(1, &trunkVAO[arch]);
        glGenBuffers(1, &trunkVBO[arch]);
        glBindVertexArray(trunkVAO[arch]);
        glBindBuffer(GL_ARRAY_BUFFER, trunkVBO[arch]);
        glBufferData(GL_ARRAY_BUFFER, tMesh.size() * sizeof(Vertex), tMesh.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);

        auto lMesh = WorldGenerator::GetTreeLeavesMesh(arch);
        leavesVertexCount[arch] = (int)lMesh.size();
        glGenVertexArrays(1, &leavesVAO[arch]);
        glGenBuffers(1, &leavesVBO[arch]);
        glBindVertexArray(leavesVAO[arch]);
        glBindBuffer(GL_ARRAY_BUFFER, leavesVBO[arch]);
        glBufferData(GL_ARRAY_BUFFER, lMesh.size() * sizeof(Vertex), lMesh.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    }

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

    // UI Setup con soporte para texturas y Symtext.ttf
    GLuint uiVAO, uiVBO;
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    // Attribute 0: Posición vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Attribute 2: Coordenadas UV vec2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // Carga de la fuente auténtica Symtext.ttf
    FontRenderer::Init("assets/fonts/Symtext.ttf");

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

    // Critter System (Butterflies, Fireflies, Jumping Frogs)
    CritterSystem critters;
    critters.Init(player.Position);

    // ARPG Core Systems (src_rpgarena_logic)
    TargetingSystem targeting;
    DamageNumberSystem damageNumbers;
    UIRenderer uiRenderer;
    HorrorPropsSystem horrorProps;
    ProjectileSystem projectiles;

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

        // Monsters will be dynamically spawned at night by the Day/Night manager
    }

    // Dynamic Day/Night Cycle (240s = 4 minutes full cycle: Day -> Sunset -> Night -> Dawn)
    float dayCycleTime = 25.0f; // Start in pleasant morning/day
    const float dayCycleLength = 240.0f;

    // Passive & Hostile Mobs (Forest Deer: Fawns, Adults, Alphas, Demonic)
    std::vector<std::unique_ptr<PassiveMob>> passiveMobs;
    for (int i = 0; i < 8; ++i) {
        float angle = (float)(rand() % 360) * 0.01745f;
        float dist = 20.0f + (rand() % 50);
        float dx = player.Position.x + cos(angle) * dist;
        float dz = player.Position.z + sin(angle) * dist;
        float dy = WorldGenerator::GetHeight(dx, dz);
        DeerSize size = (i == 0) ? DeerSize::DEMONIC : ((i == 1 || i == 5) ? DeerSize::ALPHA : ((i % 2 == 0) ? DeerSize::FAWN : DeerSize::ADULT));
        passiveMobs.push_back(std::make_unique<PassiveMob>(glm::vec3(dx, dy, dz), size));
    }

    // Humanoid & Mythic Enemy Mobs (Living Treants, Blood Vampires, Corrupted Warriors, Berserkers, Death Knights, Shadow Assassins, Skeleton Archers, Giants, Mages)
    std::vector<std::unique_ptr<EnemyMob>> enemyMobs;
    EnemyType initialTypes[] = {
        EnemyType::BERSERKER_WARRIOR,
        EnemyType::DEATH_KNIGHT,
        EnemyType::SHADOW_ASSASSIN,
        EnemyType::SKELETON_ARCHER,
        EnemyType::CORRUPTED_WARRIOR,
        EnemyType::DARK_MAGE,
        EnemyType::VAMPIRE,
        EnemyType::TREANT,
        EnemyType::NEUTRAL_GIANT
    };
    for (int i = 0; i < 9; ++i) {
        float angle = (float)(rand() % 360) * 0.01745f;
        float dist = 45.0f + (rand() % 50); // Safe initial distance
        float ex = player.Position.x + cos(angle) * dist;
        float ez = player.Position.z + sin(angle) * dist;
        float ey = WorldGenerator::GetHeight(ex, ez);
        if (ey > 1.5f) {
            enemyMobs.push_back(std::make_unique<EnemyMob>(glm::vec3(ex, ey, ez), initialTypes[i % 9], 1));
        }
    }

    // Hidden Lake Water Monsters (Submerged in lakes/lagoons)
    std::vector<std::unique_ptr<WaterMonster>> waterMonsters;
    for (int attempts = 0; attempts < 100 && waterMonsters.size() < 4; ++attempts) {
        float angle = (float)(rand() % 360) * 0.01745f;
        float dist = 20.0f + (rand() % 90);
        float wx = player.Position.x + cos(angle) * dist;
        float wz = player.Position.z + sin(angle) * dist;
        float wy = WorldGenerator::GetHeight(wx, wz);
        if (wy < Config::Water::Level) { // Under water level -> Lake!
            waterMonsters.push_back(std::make_unique<WaterMonster>(glm::vec3(wx, Config::Water::Level - 0.6f, wz)));
        }
    }

    bool isCharacterPanelOpen = false;
    FatalErrorPopup fatalError;
    LoreDocumentModal loreModal;

    InventorySystem inventory;
    ItemDropSystem itemDropSystem;
    SpellSystem spellSystem;
    SkinningSystem skinningSystem;
    WeatherSystem weatherSystem;
    StructureSystem structureSystem;
    BuildingSystem buildingSystem;
    buildingSystem.Init();

    Dragon dragon(player.Position + glm::vec3(45.0f, 44.0f, 45.0f));

    bool isBuildMode = false;
    BuildingType currentBuildType = BuildingType::WALL;
    float currentBuildYaw = 0.0f;

    bool isShovelMode = false;

    auto handleKeyAction = [&](int key) {
#ifndef __EMSCRIPTEN__
        if (key == sf::Keyboard::Escape) window.close();
        if (key == sf::Keyboard::R) {
            if (isBuildMode) {
                currentBuildYaw = fmod(currentBuildYaw + 90.0f, 360.0f);
            } else {
                spellSystem.CastArcaneBeam(player, targeting, projectiles, particles);
            }
        }
        if (key == sf::Keyboard::F) {
            if (!itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles)) {
                player.ToggleTorch();
                std::cout << "[Antorcha] Estado: " << (player.HasTorchActive ? "ENCENDIDA (Mano Izquierda)" : "GUARDADA") << std::endl;
            }
        }
        if (key == sf::Keyboard::B) {
            isBuildMode = !isBuildMode;
            if (isBuildMode) isShovelMode = false;
            std::cout << "[Construcción] Modo: " << (isBuildMode ? "ACTIVADO" : "DESACTIVADO") << std::endl;
        }
        if (key == sf::Keyboard::P) {
            isShovelMode = !isShovelMode;
            if (isShovelMode) isBuildMode = false;
            std::cout << "[Pala] Modo: " << (isShovelMode ? "ACTIVADO" : "DESACTIVADO") << std::endl;
        }
        if (isBuildMode) {
            if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1) currentBuildType = BuildingType::WALL;
            if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2) currentBuildType = BuildingType::ROOF;
            if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3) currentBuildType = BuildingType::TORCH;
        }
        if (key == sf::Keyboard::F3) debugCam = !debugCam;
        if (debugCam && key == sf::Keyboard::G) showSpawnArea = !showSpawnArea;
        if (key == sf::Keyboard::H) {
            showHitboxes = !showHitboxes;
            std::cout << "H Key Pressed! Toggle: " << (showHitboxes ? "ON" : "OFF") << std::endl;
        }
        if (key == sf::Keyboard::J) {
            birds.ToggleDebug();
            std::cout << "J Key Pressed! Toggled Bird Debug" << std::endl;
        }
        if (key == sf::Keyboard::O) {
            showMonsterMarker = !showMonsterMarker;
            std::cout << "O Key Pressed! Monster Marker: " << (showMonsterMarker ? "ON" : "OFF") << std::endl;
        }
        if (key == sf::Keyboard::V) {
            player.ToggleCameraMode();
            std::cout << "V Key Pressed! Camera: " << (player.IsThirdPerson ? "3rd Person" : "1st Person") << std::endl;
        }
        if (key == sf::Keyboard::C) {
            isCharacterPanelOpen = !isCharacterPanelOpen;
        }
        if (!isBuildMode && isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
            if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1) {
                if (player.Stats.AllocateStrength()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.30f, 0.25f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2) {
                if (player.Stats.AllocateAgility()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.25f, 0.95f, 0.40f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3) {
                if (player.Stats.AllocateVitality()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == sf::Keyboard::Num4 || key == sf::Keyboard::Numpad4) {
                if (player.Stats.AllocateIntelligence()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.35f, 0.65f, 0.95f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            }
        }
        if (key == sf::Keyboard::I || key == sf::Keyboard::Tab) {
            inventory.ToggleOpen();
        }
        if (key == sf::Keyboard::T) {
            spellSystem.CastBloodBurst(player, monsters, passiveMobs, enemyMobs, waterMonsters, particles, damageNumbers);
        }
        if (key == sf::Keyboard::G && !isBuildMode && !isShovelMode) {
            skinningSystem.TrySkin(player.Position, passiveMobs, inventory, player, damageNumbers, particles, scentSystem);
        }
        if (key == sf::Keyboard::Return || key == sf::Keyboard::Space) {
            if (fatalError.active) fatalError.active = false;
        }
        if (key == sf::Keyboard::E) {
            if (loreModal.active) {
                loreModal.active = false;
            } else if (structureSystem.TryInteract(player.Position, player, inventory, damageNumbers, particles)) {
                // Opened
            } else if (!horrorProps.TryLootNearby(player.Position, &player, damageNumbers, loreModal)) {
                spellSystem.CastShadowAegis(player, particles);
            }
        }
#else
        if (key == GLFW_KEY_R) {
            if (isBuildMode) {
                currentBuildYaw = fmod(currentBuildYaw + 90.0f, 360.0f);
            } else {
                spellSystem.CastArcaneBeam(player, targeting, projectiles, particles);
            }
        }
        if (key == GLFW_KEY_F) {
            if (!itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles)) {
                player.ToggleTorch();
                std::cout << "[Antorcha] Estado: " << (player.HasTorchActive ? "ENCENDIDA (Mano Izquierda)" : "GUARDADA") << std::endl;
            }
        }
        if (key == GLFW_KEY_B) {
            isBuildMode = !isBuildMode;
            if (isBuildMode) isShovelMode = false;
            std::cout << "[Construcción] Modo: " << (isBuildMode ? "ACTIVADO" : "DESACTIVADO") << std::endl;
        }
        if (key == GLFW_KEY_P) {
            isShovelMode = !isShovelMode;
            if (isShovelMode) isBuildMode = false;
            std::cout << "[Pala] Modo: " << (isShovelMode ? "ACTIVADO" : "DESACTIVADO") << std::endl;
        }
        if (isBuildMode) {
            if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) currentBuildType = BuildingType::WALL;
            if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) currentBuildType = BuildingType::ROOF;
            if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) currentBuildType = BuildingType::TORCH;
        }
        if (key == GLFW_KEY_F3) debugCam = !debugCam;
        if (debugCam && key == GLFW_KEY_G) showSpawnArea = !showSpawnArea;
        if (key == GLFW_KEY_H) {
            showHitboxes = !showHitboxes;
            std::cout << "H Key Pressed! Toggle: " << (showHitboxes ? "ON" : "OFF") << std::endl;
        }
        if (key == GLFW_KEY_J) {
            birds.ToggleDebug();
            std::cout << "J Key Pressed! Toggled Bird Debug" << std::endl;
        }
        if (key == GLFW_KEY_O) {
            showMonsterMarker = !showMonsterMarker;
            std::cout << "O Key Pressed! Monster Marker: " << (showMonsterMarker ? "ON" : "OFF") << std::endl;
        }
        if (key == GLFW_KEY_V) {
            player.ToggleCameraMode();
            std::cout << "V Key Pressed! Camera: " << (player.IsThirdPerson ? "3rd Person" : "1st Person") << std::endl;
        }
        if (key == GLFW_KEY_C) {
            isCharacterPanelOpen = !isCharacterPanelOpen;
        }
        if (!isBuildMode && isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
            if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) {
                if (player.Stats.AllocateStrength()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.30f, 0.25f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) {
                if (player.Stats.AllocateAgility()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.25f, 0.95f, 0.40f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) {
                if (player.Stats.AllocateVitality()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (key == GLFW_KEY_4 || key == GLFW_KEY_KP_4) {
                if (player.Stats.AllocateIntelligence()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.35f, 0.65f, 0.95f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            }
        }
        if (key == GLFW_KEY_I || key == GLFW_KEY_TAB) {
            inventory.ToggleOpen();
        }
        if (key == GLFW_KEY_T) {
            spellSystem.CastBloodBurst(player, monsters, passiveMobs, enemyMobs, waterMonsters, particles, damageNumbers);
        }
        if (key == GLFW_KEY_G && !isBuildMode) {
            skinningSystem.TrySkin(player.Position, passiveMobs, inventory, player, damageNumbers, particles, scentSystem);
        }
        if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) {
            if (fatalError.active) fatalError.active = false;
        }
        if (key == GLFW_KEY_E) {
            if (loreModal.active) {
                loreModal.active = false;
            } else if (structureSystem.TryInteract(player.Position, player, inventory, damageNumbers, particles)) {
                // Opened
            } else if (!horrorProps.TryLootNearby(player.Position, &player, damageNumbers, loreModal)) {
                spellSystem.CastShadowAegis(player, particles);
            }
        }
#endif
    };

#ifdef __EMSCRIPTEN__
    static std::function<void(int)> s_EmscriptenKeyHandler;
    s_EmscriptenKeyHandler = handleKeyAction;
    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS && s_EmscriptenKeyHandler) {
            s_EmscriptenKeyHandler(key);
        }
    });
#endif

#ifndef __EMSCRIPTEN__
    sf::Clock clock;
    sf::Clock fpsClock;
    int frameCount = 0;
    int currentFPS = 0;
#endif

    auto updateAndRenderFrame = [&]() {
#ifndef __EMSCRIPTEN__
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
                handleKeyAction(event.key.code);
            }
        }
#else
        static double lastFrameTime = emscripten_get_now() / 1000.0;
        static double lastFpsTime = lastFrameTime;
        static int frameCount = 0;
        static int currentFPS = 0;
        double currentNow = emscripten_get_now() / 1000.0;
        float deltaTime = (float)(currentNow - lastFrameTime);
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastFrameTime = currentNow;

        frameCount++;
        if (currentNow - lastFpsTime >= 1.0) {
            currentFPS = frameCount;
            frameCount = 0;
            lastFpsTime = currentNow;
        }

        glfwPollEvents();
#endif

        if (PlatformInput::IsKeyPressed(PlatformInput::Left)) {
            float angle = -2.0f * deltaTime;
            glm::vec2 current = windSystem.GetDirection();
            float c = cos(angle); float s = sin(angle);
            float nx = current.x * c - current.y * s;
            float ny = current.x * s + current.y * c;
            windSystem.SetDirection(nx, ny);
        }
        if (PlatformInput::IsKeyPressed(PlatformInput::Right)) {
            float angle = 2.0f * deltaTime;
            glm::vec2 current = windSystem.GetDirection();
            float c = cos(angle); float s = sin(angle);
            float nx = current.x * c - current.y * s;
            float ny = current.x * s + current.y * c;
            windSystem.SetDirection(nx, ny);
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

        // Monster Update (Now takes ScentSystem, playerFront, Velocity, Weapon Ammo, Reloading State & Flashlight)
        for (auto& mPtr : monsters) {
            mPtr->Update(deltaTime, player.Position, player.Front, windDir, 
                        chunkManager, scentSystem, particles,
                        player.Velocity, weapon.GetAmmo(), weapon.IsReloading(),
                        player.IsClimbing, player.ClimbingTreePos,
                        gameCfg.flashlightEnabled);
        }

        // Debug state printing to terminal (every 1.0 seconds)
        static float debugPrintTimer = 0.0f;
        debugPrintTimer += deltaTime;
        if (debugPrintTimer >= 1.0f) {
            debugPrintTimer = 0.0f;
            std::cout << "\n=== GAME STATE DEBUG ===" << std::endl;
            std::cout << "Player Position: (" << player.Position.x << ", " << player.Position.y << ", " << player.Position.z << ")" << std::endl;
            std::cout << "Player Velocity: (" << player.Velocity.x << ", " << player.Velocity.y << ", " << player.Velocity.z << ")" << std::endl;
            std::cout << "Player IsGrounded: " << (player.IsGrounded ? "Yes" : "No") << " | IsClimbing: " << (player.IsClimbing ? "Yes" : "No") << std::endl;
            for (size_t i = 0; i < monsters.size(); ++i) {
                const auto& m = monsters[i];
                if (m->IsDead()) {
                    std::cout << "Monster #" << i << ": DEAD" << std::endl;
                    continue;
                }
                std::cout << "Monster #" << i << ":" << std::endl;
                std::cout << "  Position: (" << m->GetPosition().x << ", " << m->GetPosition().y << ", " << m->GetPosition().z << ")" << std::endl;
                std::cout << "  Action: ";
                switch (m->GetAction()) {
                    case MonsterAction::WANDER: std::cout << "WANDER"; break;
                    case MonsterAction::INVESTIGATE: std::cout << "INVESTIGATE"; break;
                    case MonsterAction::STALK: std::cout << "STALK"; break;
                    case MonsterAction::RETREAT: std::cout << "RETREAT"; break;
                    case MonsterAction::CLIMB_TREE: std::cout << "CLIMB_TREE"; break;
                    case MonsterAction::CHASE: std::cout << "CHASE"; break;
                    case MonsterAction::TRACK_SCENT: std::cout << "TRACK_SCENT"; break;
                }
                std::cout << std::endl;
                std::cout << "  Is Climbing: " << (m->IsClimbing() ? "Yes" : "No");
                if (m->IsClimbing() || m->GetTreeClimbHeight() > 0.0f) {
                    std::cout << " (Height: " << m->GetTreeClimbHeight() << ")";
                }
                std::cout << std::endl;
                std::cout << "  Enraged: " << (m->IsEnraged() ? "YES" : "NO") << std::endl;
                std::cout << "  LOS to Player: " << (m->HasVisualContact() ? "Yes" : "No") << std::endl;
                std::cout << "  Confidence: " << m->GetConfidence() << " | Stress: " << m->GetStress() << std::endl;
                std::cout << "  Estimated Player Ammo: " << m->GetEstimatedAmmo() << std::endl;
                
                float dist = glm::distance(m->GetPosition(), player.Position);
                std::cout << "  Distance to Player: " << dist << "m" << std::endl;
            }
            std::cout << "============================\n" << std::endl;
        }

        // GAME OVER CHECK & TIMING (Linked to true Player RPG Health)
        if (isGameOver) {
            gameOverTimer -= deltaTime;
            if (gameOverTimer <= 0.0f) {
#ifndef __EMSCRIPTEN__
                window.close();
#endif
            }
        } else {
            if (player.Stats.CurrentHP <= 0) {
                isGameOver = true;
                gameOverTimer = 3.5f;
                std::cout << "GAME OVER! Player HP depleted..." << std::endl;
                for (int i = 0; i < 40; ++i) {
                    glm::vec3 velocity((rand()%100/50.0f - 1.0f)*5.0f, (rand()%100/50.0f - 0.3f)*6.0f, (rand()%100/50.0f - 1.0f)*5.0f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, -0.6f, 0), velocity, glm::vec4(0.8f, 0.0f, 0.0f, 1.0f), 0.15f, 1.5f, -9.8f);
                }
            }
        }
        
        // Update Birds
        birds.Update(deltaTime, player.Position, monsters);
        birds.CleanupDistantBirds(player.Position, 80.0f); // Optimization
        
        if (!debugCam) {
            if (!isGameOver) {
                player.ProcessKeyboard(0, deltaTime, chunkManager, footprints);
            }
            // Colisión sólida con paredes y techos construidos
            glm::vec3 bPush(0.0f);
            buildingSystem.CheckCollision(player.Position, player.PlayerRadius, player.PlayerHeight, bPush);

            // Permitir pararse y caminar sobre techos construidos
            float feetY = player.Position.y - player.PlayerHeight;
            float defaultGroundY = WorldGenerator::GetHeight(player.Position.x, player.Position.z);
            float floorY = buildingSystem.GetFloorHeight(player.Position.x, player.Position.z, feetY, defaultGroundY);

            player.Update(deltaTime);

            if (player.Position.y < floorY + player.PlayerHeight) {
                player.Position.y = floorY + player.PlayerHeight;
                player.Velocity.y = 0.0f;
                player.IsGrounded = true;
            }
        } else {
            // Free Cam Movement
            float camSpeed = Config::Gameplay::DebugCamSpeed * deltaTime;
            if (PlatformInput::IsKeyPressed(PlatformInput::W)) freeCamPos += freeCamFront * camSpeed;
            if (PlatformInput::IsKeyPressed(PlatformInput::S)) freeCamPos -= freeCamFront * camSpeed;
            glm::vec3 camRight = glm::normalize(glm::cross(freeCamFront, glm::vec3(0,1,0)));
            if (PlatformInput::IsKeyPressed(PlatformInput::A)) freeCamPos -= camRight * camSpeed;
            if (PlatformInput::IsKeyPressed(PlatformInput::D)) freeCamPos += camRight * camSpeed;
        }

        float mouseNdcX = -999.0f;
        float mouseNdcY = -999.0f;

        // Compute Building Target Position in World
        glm::vec3 buildFwd = player.Front;
        buildFwd.y = 0.0f;
        if (glm::length(buildFwd) > 0.001f) buildFwd = glm::normalize(buildFwd);
        float buildDist = 4.2f;
        glm::vec3 buildPos(0.0f);
        buildPos.x = std::round((player.Position.x + buildFwd.x * buildDist) / 0.5f) * 0.5f;
        buildPos.z = std::round((player.Position.z + buildFwd.z * buildDist) / 0.5f) * 0.5f;
        float bGroundY = WorldGenerator::GetHeight(buildPos.x, buildPos.z);
        buildPos.y = bGroundY;
        if (currentBuildType == BuildingType::ROOF) {
            buildPos.y = bGroundY + 2.8f;
        }

        // Compute Shovel Target Position in World (Only when in Shovel Mode [P])
        glm::vec3 terraTarget(0.0f);
        bool hasTerraTarget = false;
        if (isShovelMode) {
            glm::vec3 terraRayOrigin = player.GetCameraPosition();
            glm::vec3 terraRayDir = player.Front;
            for (float rDist = 0.5f; rDist < 20.0f; rDist += 0.35f) {
                glm::vec3 p = terraRayOrigin + terraRayDir * rDist;
                float groundY = WorldGenerator::GetHeight(p.x, p.z);
                if (p.y <= groundY + 0.15f) {
                    terraTarget = glm::vec3(p.x, groundY, p.z);
                    hasTerraTarget = true;
                    break;
                }
            }
            if (!hasTerraTarget) {
                float tx = player.Position.x + player.Front.x * 4.5f;
                float tz = player.Position.z + player.Front.z * 4.5f;
                terraTarget = glm::vec3(tx, WorldGenerator::GetHeight(tx, tz), tz);
                hasTerraTarget = true;
            }
        }

#ifndef __EMSCRIPTEN__
        bool hasFocus = window.hasFocus();
        int screenW = 1280;
        int screenH = 720;
        sf::Vector2u winSize = window.getSize();
        screenW = (int)winSize.x;
        screenH = (int)winSize.y;
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float curMouseX = (float)mousePos.x;
        float curMouseY = (float)mousePos.y;
        bool rightIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);
        bool leftIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
#else
        bool hasFocus = true;
        int screenW = 1280, screenH = 720;
        glfwGetFramebufferSize(window, &screenW, &screenH);
        if (screenW <= 0) screenW = 1280;
        if (screenH <= 0) screenH = 720;
        double mx = 0, my = 0;
        glfwGetCursorPos(window, &mx, &my);
        float curMouseX = (float)mx;
        float curMouseY = (float)my;
        bool rightIsPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool leftIsPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
#endif

        if (hasFocus) {
            static bool firstMouse = true;
            static float lastMouseX = curMouseX;
            static float lastMouseY = curMouseY;
            
            if (firstMouse) {
                lastMouseX = curMouseX;
                lastMouseY = curMouseY;
                firstMouse = false;
            }

            float xoff = curMouseX - lastMouseX;
            float yoff = lastMouseY - curMouseY;
            lastMouseX = curMouseX;
            lastMouseY = curMouseY;

#ifdef __EMSCRIPTEN__
            static float virtualMouseX = (float)screenW * 0.5f;
            static float virtualMouseY = (float)screenH * 0.5f;
            static bool s_firstVirtual = true;
            if (s_firstVirtual) {
                virtualMouseX = (float)screenW * 0.5f;
                virtualMouseY = (float)screenH * 0.5f;
                s_firstVirtual = false;
            }
            virtualMouseX += xoff;
            virtualMouseY -= yoff;
            if (virtualMouseX < 0.0f) virtualMouseX = 0.0f;
            if (virtualMouseX > (float)screenW) virtualMouseX = (float)screenW;
            if (virtualMouseY < 0.0f) virtualMouseY = 0.0f;
            if (virtualMouseY > (float)screenH) virtualMouseY = (float)screenH;

            mouseNdcX = (virtualMouseX / (float)screenW) * 2.0f - 1.0f;
            mouseNdcY = 1.0f - (virtualMouseY / (float)screenH) * 2.0f;
#else
            mouseNdcX = (curMouseX / (float)screenW) * 2.0f - 1.0f;
            mouseNdcY = 1.0f - (curMouseY / (float)screenH) * 2.0f;
#endif

            bool isUiModalActive = isCharacterPanelOpen || loreModal.active || fatalError.active || inventory.IsOpen();

            // In 3rd Person or when UI is open: ONLY rotate camera when Right Click is held down (WoW style)
            // In 1st Person: Rotate camera when UI is not active
            bool shouldRotateCam = (!isUiModalActive && !player.IsThirdPerson) || rightIsPressed;

            if (shouldRotateCam) {
                if (std::abs(xoff) < 250.0f && std::abs(yoff) < 250.0f && (std::abs(xoff) > 0.001f || std::abs(yoff) > 0.001f)) {
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
                }
            }

            // Left Click: UI Button Clicks, Target Selection & Sword Attack / Build / Dig
            static bool leftWasPressed = false;
            if (leftIsPressed && !leftWasPressed) {
                // 0. Build Mode Placement
                if (isBuildMode) {
                    buildingSystem.PlacePiece(currentBuildType, buildPos, currentBuildYaw, &particles);
                    leftWasPressed = leftIsPressed;
                    goto skipWorldTargeting;
                }

                // 0.1 Shovel Mode Cavar
                if (isShovelMode && hasTerraTarget) {
                    chunkManager.ModifyTerrain(terraTarget.x, terraTarget.z, 3.6f, -1.2f, &particles);
                    leftWasPressed = leftIsPressed;
                    goto skipWorldTargeting;
                }

                // 1. Check UI Modals First
                if (inventory.IsOpen()) {
                    bool closeReq = false;
                    if (inventory.HandleMouseClick(mouseNdcX, mouseNdcY, &player, &particles, &damageNumbers, closeReq)) {
                        if (closeReq) inventory.SetOpen(false);
                        leftWasPressed = leftIsPressed;
                        goto skipWorldTargeting;
                    }
                }

                if (fatalError.active) {
                    if (UIRenderer::HandleFatalErrorClick(mouseNdcX, mouseNdcY, fatalError)) {
                        leftWasPressed = leftIsPressed;
                        goto skipWorldTargeting;
                    }
                }

                if (loreModal.active) {
                    if (UIRenderer::HandleLoreModalClick(mouseNdcX, mouseNdcY, loreModal)) {
                        leftWasPressed = leftIsPressed;
                        goto skipWorldTargeting;
                    }
                }

                if (isCharacterPanelOpen) {
                    bool closeReq = false;
                    if (UIRenderer::HandleCharacterPanelClick(mouseNdcX, mouseNdcY, player.Stats, closeReq)) {
                        if (closeReq) {
                            isCharacterPanelOpen = false;
                        } else {
                            // Stat allocated! Spawn golden fanfare particles
                            for (int i = 0; i < 22; ++i) {
                                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                                particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                            }
                        }
                        leftWasPressed = leftIsPressed;
                        goto skipWorldTargeting;
                    }
                }

                // 2. World Targeting & Combat
                glm::vec3 activeCamPos = player.GetCameraPosition();
                glm::vec3 camForward = player.Front;

                // Find closest target in aim direction
                float bestDot = 0.80f;
                PassiveMob* bestDeer = nullptr;
                Monster* bestMonster = nullptr;
                EnemyMob* bestEnemy = nullptr;
                WaterMonster* bestWater = nullptr;
                // Darkness & Flashlight logic on targeting:
                float maxTargetingRange = 42.0f;
                bool isNightTimeNow = (dayCycleTime >= 120.0f && dayCycleTime <= 228.0f);
                if (isNightTimeNow && !gameCfg.flashlightEnabled) {
                    maxTargetingRange = 4.0f; // Blind targeting in dark mist without flashlight!
                }
                float closestDist = maxTargetingRange;

                for (auto& deer : passiveMobs) {
                    if (!deer->IsAlive()) continue;
                    glm::vec3 mPos = deer->GetPosition() + glm::vec3(0, 1.2f, 0);
                    glm::vec3 toM = glm::normalize(mPos - activeCamPos);
                    float dot = glm::dot(camForward, toM);
                    float dist = glm::distance(player.Position, deer->GetPosition());
                    if (dot > bestDot && dist < closestDist) {
                        bestDot = dot;
                        closestDist = dist;
                        bestDeer = deer.get();
                        bestMonster = nullptr;
                        bestEnemy = nullptr;
                        bestWater = nullptr;
                    }
                }

                for (auto& enemy : enemyMobs) {
                    if (!enemy->IsAlive()) continue;
                    glm::vec3 ePos = enemy->GetPosition() + glm::vec3(0, 1.4f, 0);
                    glm::vec3 toE = glm::normalize(ePos - activeCamPos);
                    float dot = glm::dot(camForward, toE);
                    float dist = glm::distance(player.Position, enemy->GetPosition());
                    if (dot > bestDot && dist < closestDist) {
                        bestDot = dot;
                        closestDist = dist;
                        bestEnemy = enemy.get();
                        bestDeer = nullptr;
                        bestMonster = nullptr;
                        bestWater = nullptr;
                    }
                }

                for (auto& wm : waterMonsters) {
                    if (!wm->IsAlive()) continue;
                    glm::vec3 wPos = wm->GetPosition() + glm::vec3(0, 1.2f, 0);
                    glm::vec3 toW = glm::normalize(wPos - activeCamPos);
                    float dot = glm::dot(camForward, toW);
                    float dist = glm::distance(player.Position, wm->GetPosition());
                    if (dot > bestDot && dist < closestDist) {
                        bestDot = dot;
                        closestDist = dist;
                        bestWater = wm.get();
                        bestDeer = nullptr;
                        bestMonster = nullptr;
                        bestEnemy = nullptr;
                    }
                }

                for (auto& mPtr : monsters) {
                    if (mPtr->IsDead()) continue;
                    glm::vec3 mPos = mPtr->GetPosition() + glm::vec3(0, 1.5f, 0);
                    glm::vec3 toM = glm::normalize(mPos - activeCamPos);
                    float dot = glm::dot(camForward, toM);
                    float dist = glm::distance(player.Position, mPtr->GetPosition());
                    if (dot > bestDot && dist < closestDist) {
                        bestDot = dot;
                        closestDist = dist;
                        bestMonster = mPtr.get();
                        bestDeer = nullptr;
                        bestEnemy = nullptr;
                        bestWater = nullptr;
                    }
                }

                if (bestWater) {
                    targeting.SelectWaterMonster(bestWater);
                } else if (bestEnemy) {
                    targeting.SelectEnemy(bestEnemy);
                } else if (bestDeer) {
                    targeting.SelectPassive(bestDeer);
                } else if (bestMonster) {
                    targeting.SelectMonster(bestMonster);
                }

                // Swing sword immediately on click (even while running/sprinting)
                if (player.TryAttack()) {
                    horrorProps.CheckSwordCut(player.Position, 3.2f, particles);
                }
            }
        skipWorldTargeting:
            leftWasPressed = leftIsPressed;
        }

        // Holding [Q] attacks constantly respecting attack speed / agility multiplier
        if (PlatformInput::IsKeyPressed(PlatformInput::Q)) {
            if (player.TryAttack()) {
                horrorProps.CheckSwordCut(player.Position, 3.2f, particles);
            }
        }

        targeting.Update(deltaTime, player.Position, false);

        // --- DYNAMIC TERRAIN DEFORMATION (PALA [P]: G para Cavar, H o Click Der para Poner Tierra) ---
        bool isDigging = isShovelMode && (PlatformInput::IsKeyPressed(PlatformInput::G) || PlatformInput::IsKeyPressed(PlatformInput::Num3));
        bool isBuilding = isShovelMode && (PlatformInput::IsKeyPressed(PlatformInput::H) || PlatformInput::IsKeyPressed(PlatformInput::Num4) || (rightIsPressed && !player.IsThirdPerson));

        if (isShovelMode && hasTerraTarget && (isDigging || isBuilding)) {
            float deformRadius = 3.6f;
            float deltaH = (isBuilding ? +1.0f : -1.0f) * 3.8f * deltaTime;
            chunkManager.ModifyTerrain(terraTarget.x, terraTarget.z, deformRadius, deltaH, &particles);
        }

        // Building System Update (Torch particles) & Solid Collision
        buildingSystem.Update(deltaTime, player.Position, particles);
        glm::vec3 bPush(0.0f);
        buildingSystem.CheckCollision(player.Position, player.PlayerRadius, player.PlayerHeight, bPush);

        // 1. Advance Day/Night Cycle Progression & Dynamic Weather / Spells
        spellSystem.Update(deltaTime, player, particles);
        weatherSystem.Update(deltaTime, dayCycleTime, player.Position, glm::vec3(windDir.x, 0, windDir.y), particles);

        dayCycleTime += deltaTime;
        float cycleNormalized = fmod(dayCycleTime, dayCycleLength) / dayCycleLength;

        // Day: 0.00 - 0.45, Sunset: 0.45 - 0.58, Night: 0.58 - 0.88, Dawn: 0.88 - 1.00
        float nightFactor = 0.0f;
        glm::vec3 fogCol(0.40f, 0.60f, 0.95f);

        if (cycleNormalized < 0.45f) {
            // Day
            nightFactor = 0.0f;
            fogCol = glm::vec3(0.40f, 0.60f, 0.95f);
        } else if (cycleNormalized < 0.58f) {
            // Sunset / Dusk
            float t = (cycleNormalized - 0.45f) / 0.13f;
            nightFactor = t;
            glm::vec3 sunsetCol(0.85f, 0.32f, 0.14f);
            glm::vec3 nightCol(0.005f, 0.005f, 0.015f);
            if (t < 0.5f) {
                fogCol = glm::mix(glm::vec3(0.40f, 0.60f, 0.95f), sunsetCol, t * 2.0f);
            } else {
                fogCol = glm::mix(sunsetCol, nightCol, (t - 0.5f) * 2.0f);
            }
        } else if (cycleNormalized < 0.88f) {
            // Night
            nightFactor = 1.0f;
            fogCol = glm::vec3(0.005f, 0.005f, 0.015f);
        } else {
            // Dawn / Sunrise
            float t = (cycleNormalized - 0.88f) / 0.12f;
            nightFactor = 1.0f - t;
            glm::vec3 sunriseCol(0.90f, 0.45f, 0.25f);
            glm::vec3 dayCol(0.40f, 0.60f, 0.95f);
            if (t < 0.5f) {
                fogCol = glm::mix(glm::vec3(0.005f, 0.005f, 0.015f), sunriseCol, t * 2.0f);
            } else {
                fogCol = glm::mix(sunriseCol, dayCol, (t - 0.5f) * 2.0f);
            }
        }

        fogCol = weatherSystem.GetAdjustedFog(fogCol);

        bool isNightTime = (nightFactor > 0.45f) || weatherSystem.IsBloodMoon();

        // Dynamic Night Monster Spawning & Stalking
        static float monsterSpawnTimer = 0.0f;
        monsterSpawnTimer += deltaTime;

        if (isNightTime) {
            // Remove dead monsters
            for (auto it = monsters.begin(); it != monsters.end();) {
                if ((*it)->IsDead()) {
                    if (targeting.GetMonsterTarget() == it->get()) targeting.ClearTarget();
                    it = monsters.erase(it);
                } else {
                    ++it;
                }
            }

            // Spawn smart shadow monsters at night if population < 3
            if (monsterSpawnTimer >= 5.0f && monsters.size() < 3) {
                monsterSpawnTimer = 0.0f;
                float mAngle = (float)(rand() % 360) * 0.01745f;
                float mDist = 38.0f + (rand() % 35);
                float mx = player.Position.x + cos(mAngle) * mDist;
                float mz = player.Position.z + sin(mAngle) * mDist;
                float my = WorldGenerator::GetHeight(mx, mz);
                if (my > 1.5f) {
                    auto m = std::make_unique<Monster>(glm::vec3(mx, my, mz));
                    m->LookAt(player.Position);
                    monsters.push_back(std::move(m));
                }
            }
        } else {
            // During the day, monsters retreat / dissolve into mist
            for (auto it = monsters.begin(); it != monsters.end();) {
                if (targeting.GetMonsterTarget() == it->get()) targeting.ClearTarget();
                it = monsters.erase(it);
            }
        }

        // Update Active Monsters AI & Combat (Night Stalkers)
        static float monsterAttackCooldown = 0.0f;
        if (monsterAttackCooldown > 0.0f) monsterAttackCooldown -= deltaTime;

        for (auto& mPtr : monsters) {
            if (mPtr->IsDead()) continue;
            mPtr->Update(deltaTime, player.Position, player.Front, windDir,
                         chunkManager, scentSystem, particles,
                         player.Velocity, 0, false,
                         player.IsClimbing, player.ClimbingTreePos,
                         isNightTime);

            glm::vec3 mPos = mPtr->GetPosition();
            glm::vec3 mPush(0.0f);
            if (buildingSystem.CheckCollision(mPos, 1.1f, 2.5f, mPush)) {
                mPtr->SetPosition(mPos);
            }

            // Monster Melee Attack against Player (Balanced 18-24 damage, NOT 1-hit kill)
            float distToPlayer = glm::distance(player.Position, mPtr->GetPosition());
            if (distToPlayer < 2.3f && monsterAttackCooldown <= 0.0f) {
                monsterAttackCooldown = 1.6f;
                int dmg = 18 + (rand() % 7);
                player.TakeDamage(dmg, damageNumbers, &fatalError);

                // Claw impact particles
                glm::vec3 hitPos = player.Position + glm::vec3(0.0f, 1.2f, 0.0f);
                for (int i = 0; i < 20; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.2f, (rand()%100/50.0f + 0.3f)*3.5f, (rand()%100/50.0f - 1.0f)*3.2f);
                    particles.SpawnParticle(hitPos, pVel, glm::vec4(0.85f, 0.05f, 0.05f, 1.0f), 0.14f, 0.85f, -9.8f);
                }
            }
        }

        // Dynamic Deer Spawning & Herd Management
        static float deerSpawnTimer = 0.0f;
        deerSpawnTimer += deltaTime;
        if (deerSpawnTimer >= 1.5f) {
            deerSpawnTimer = 0.0f;

            // 1. Despawn dead deer after decay or deer too far away (> 130m)
            for (auto it = passiveMobs.begin(); it != passiveMobs.end();) {
                float dist = glm::distance(glm::vec2(player.Position.x, player.Position.z), glm::vec2((*it)->GetPosition().x, (*it)->GetPosition().z));
                if ((*it)->IsRemovable() || (dist > 140.0f && !(*it)->IsAlive())) {
                    if (targeting.GetPassiveTarget() == it->get()) {
                        targeting.ClearTarget();
                    }
                    it = passiveMobs.erase(it);
                } else if (dist > 150.0f) {
                    it = passiveMobs.erase(it);
                } else {
                    ++it;
                }
            }

            // 2. Spawn a herd (2-4 deer together) if population < 10
            if (passiveMobs.size() < 10) {
                float herdAngle = (float)(rand() % 360) * 0.01745f;
                float herdDist = 42.0f + (rand() % 38);
                float herdCenterX = player.Position.x + cos(herdAngle) * herdDist;
                float herdCenterZ = player.Position.z + sin(herdAngle) * herdDist;

                int herdSize = 2 + rand() % 3;
                for (int i = 0; i < herdSize; ++i) {
                    float offX = herdCenterX + (rand() % 100 / 10.0f - 5.0f);
                    float offZ = herdCenterZ + (rand() % 100 / 10.0f - 5.0f);
                    float offY = WorldGenerator::GetHeight(offX, offZ);
                    if (offY > 1.5f) { // Above water level
                        int roll = rand() % 100;
                        int demonicChance = isNightTime ? 24 : 8;
                        DeerSize size;
                        if (roll < demonicChance) {
                            size = DeerSize::DEMONIC;
                        } else if (roll < demonicChance + 22) {
                            size = DeerSize::ALPHA;
                        } else if (roll < demonicChance + 55) {
                            size = DeerSize::FAWN;
                        } else {
                            size = DeerSize::ADULT;
                        }
                        passiveMobs.push_back(std::make_unique<PassiveMob>(glm::vec3(offX, offY, offZ), size));
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // PHYSICAL ITEM DROPS & LOOT ENGINE
        // ---------------------------------------------------------------------
        itemDropSystem.Update(deltaTime, player.Position, inventory, damageNumbers, particles);

        // Generar botín físico cuando muere un enemigo
        for (auto& enemy : enemyMobs) {
            if (!enemy->IsAlive() && !enemy->HasDroppedLoot()) {
                enemy->SetLootDropped(true);
                LootTable table = LootManager::GetEnemyLoot(enemy->GetType(), enemy->GetNightLevel());
                std::vector<ItemInstance> drops = table.GenerateLoot(1.0f);
                itemDropSystem.SpawnDrops(drops, enemy->GetPosition());
            }
        }

        // Generar botín físico cuando muere el monstruo acechador
        for (auto& mPtr : monsters) {
            if (mPtr->IsDead() && !mPtr->HasDroppedLoot()) {
                mPtr->SetLootDropped(true);
                LootTable table = LootManager::GetEnemyLoot(EnemyType::CORRUPTED_WARRIOR, 1);
                std::vector<ItemInstance> drops = table.GenerateLoot(1.0f);
                itemDropSystem.SpawnDrops(drops, mPtr->GetPosition());
            }
        }

        // Dynamic Enemy Mobs Spawning & Cleanup (Corrupted Warriors, Giants, Mages)
        static float enemySpawnTimer = 0.0f;
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= 2.0f) {
            enemySpawnTimer = 0.0f;

            for (auto it = enemyMobs.begin(); it != enemyMobs.end();) {
                float dist = glm::distance(glm::vec2(player.Position.x, player.Position.z), glm::vec2((*it)->GetPosition().x, (*it)->GetPosition().z));
                if ((*it)->IsRemovable() || (dist > 140.0f && !(*it)->IsAlive())) {
                    if (targeting.GetEnemyTarget() == it->get()) targeting.ClearTarget();
                    it = enemyMobs.erase(it);
                } else if (dist > 160.0f) {
                    it = enemyMobs.erase(it);
                } else {
                    ++it;
                }
            }

            if (enemyMobs.size() < 12) {
                float angle = (float)(rand() % 360) * 0.01745f;
                float dist = 42.0f + (rand() % 45);
                float ex = player.Position.x + cos(angle) * dist;
                float ez = player.Position.z + sin(angle) * dist;
                float ey = WorldGenerator::GetHeight(ex, ez);
                if (ey > 1.5f) {
                    int roll = rand() % 100;
                    int currentNightLevel = weatherSystem.GetNightCount();
                    EnemyType type;
                    if (isNightTime) {
                        if (roll < 16) type = EnemyType::DEATH_KNIGHT;
                        else if (roll < 32) type = EnemyType::BERSERKER_WARRIOR;
                        else if (roll < 46) type = EnemyType::SKELETON_ARCHER;
                        else if (roll < 60) type = EnemyType::SHADOW_ASSASSIN;
                        else if (roll < 74) type = EnemyType::VAMPIRE;
                        else if (roll < 88) type = EnemyType::DARK_MAGE;
                        else type = EnemyType::CORRUPTED_WARRIOR;
                    } else {
                        if (roll < 18) type = EnemyType::SKELETON_ARCHER;
                        else if (roll < 36) type = EnemyType::BERSERKER_WARRIOR;
                        else if (roll < 52) type = EnemyType::CORRUPTED_WARRIOR;
                        else if (roll < 66) type = EnemyType::SHADOW_ASSASSIN;
                        else if (roll < 78) type = EnemyType::TREANT;
                        else if (roll < 88) type = EnemyType::DARK_MAGE;
                        else type = EnemyType::NEUTRAL_GIANT;
                    }
                    enemyMobs.push_back(std::make_unique<EnemyMob>(glm::vec3(ex, ey, ez), type, currentNightLevel));
                }
            }
        }

        // Dynamic Water Monster Spawning in Lakes & Lagoons
        static float waterSpawnTimer = 0.0f;
        waterSpawnTimer += deltaTime;
        if (waterSpawnTimer >= 2.5f) {
            waterSpawnTimer = 0.0f;

            for (auto it = waterMonsters.begin(); it != waterMonsters.end();) {
                float dist = glm::distance(glm::vec2(player.Position.x, player.Position.z), glm::vec2((*it)->GetPosition().x, (*it)->GetPosition().z));
                if ((*it)->IsRemovable() || (dist > 150.0f && !(*it)->IsAlive())) {
                    if (targeting.GetWaterTarget() == it->get()) targeting.ClearTarget();
                    it = waterMonsters.erase(it);
                } else if (dist > 170.0f) {
                    it = waterMonsters.erase(it);
                } else {
                    ++it;
                }
            }

            if (waterMonsters.size() < 5) {
                for (int attempts = 0; attempts < 30; ++attempts) {
                    float angle = (float)(rand() % 360) * 0.01745f;
                    float dist = 25.0f + (rand() % 65);
                    float wx = player.Position.x + cos(angle) * dist;
                    float wz = player.Position.z + sin(angle) * dist;
                    float wy = WorldGenerator::GetHeight(wx, wz);
                    if (wy < Config::Water::Level) {
                        waterMonsters.push_back(std::make_unique<WaterMonster>(glm::vec3(wx, Config::Water::Level - 0.6f, wz)));
                        break;
                    }
                }
            }
        }

        // Update Lake Water Monsters
        for (auto& wm : waterMonsters) {
            wm->Update(deltaTime, player.Position, &player, particles, damageNumbers);
        }

        // Update Enemy Mobs
        for (auto& enemy : enemyMobs) {
            enemy->Update(deltaTime, player.Position, &player, particles, damageNumbers, projectiles);
            glm::vec3 ePush(0.0f);
            buildingSystem.CheckCollision(enemy->GetPositionRef(), enemy->GetRadius(), 2.0f, ePush);
        }

        // Update Flying Magic Projectiles
        projectiles.Update(deltaTime, &player, particles, damageNumbers);

        // Update Passive Mobs (Forest Deer: Fawns, Adults, Alphas, Demonic)
        for (auto& deer : passiveMobs) {
            deer->Update(deltaTime, player.Position, &player, particles, damageNumbers);
            glm::vec3 dPush(0.0f);
            buildingSystem.CheckCollision(deer->GetPositionRef(), deer->GetRadius(), 1.4f, dPush);
        }

        // Update Flying Dragon
        dragon.Update(deltaTime, player.Position, particles, damageNumbers);

        // Update Floating Combat Numbers (Damage & EXP)
        damageNumbers.Update(deltaTime);

        // Update Melee Combat (Monsters, Passive Mobs, Enemy Mobs, Water Monsters, EXP & Level Up)
        player.UpdateCombat(deltaTime, monsters, passiveMobs, enemyMobs, waterMonsters, particles, damageNumbers);

        // Update Critters (Butterflies, Fireflies, Jumping Frogs)
        critters.Update(deltaTime, player.Position);

        // Update Environmental Horror Props (Hanging Victims Physics & Blood Scent)
        std::vector<glm::vec4> nearbyTreesForProps;
        chunkManager.GetTreesInRange(player.Position, 85.0f, nearbyTreesForProps);
        horrorProps.Update(deltaTime, player.Position, nearbyTreesForProps, particles, scentSystem);

        // Calculate Danger Level for SUFFERING_MONITOR.EXE ECG Oscilloscope
        float dangerLevel = 0.0f;
        for (auto& mPtr : monsters) {
            if (!mPtr->IsDead()) {
                float d = glm::distance(player.Position, mPtr->GetPosition());
                if (d < 24.0f) dangerLevel = std::max(dangerLevel, 1.0f - (d / 24.0f));
            }
        }
        for (auto& ePtr : enemyMobs) {
            if (ePtr->IsAlive()) {
                float d = glm::distance(player.Position, ePtr->GetPosition());
                if (d < 24.0f) dangerLevel = std::max(dangerLevel, 1.0f - (d / 24.0f));
            }
        }
        for (auto& wPtr : waterMonsters) {
            if (wPtr->IsAlive()) {
                float d = glm::distance(player.Position, wPtr->GetPosition());
                if (d < 20.0f) dangerLevel = std::max(dangerLevel, 1.0f - (d / 20.0f));
            }
        }
        if (fatalError.timer > 0.0f) {
            fatalError.timer -= deltaTime;
            if (fatalError.timer <= 0.0f) fatalError.active = false;
        }

        // Handle climbing noises (camera/movement noise)
        if (player.SoundVolumeEmitted > 0.0f) {
            // Alert all monsters
            for (auto& mPtr : monsters) {
                mPtr->HearSound(player.Position, player.SoundVolumeEmitted);
            }
            // Spawn leaf particles (green) rustling/falling from the player
            int leafCount = (player.SoundVolumeEmitted > 18.0f) ? 12 : 5;
            for (int i = 0; i < leafCount; ++i) {
                // Random offset around player position in tree foliage
                float rx = (rand() % 100 / 100.0f - 0.5f) * 4.0f;
                float ry = (rand() % 100 / 100.0f - 0.5f) * 2.0f;
                float rz = (rand() % 100 / 100.0f - 0.5f) * 4.0f;
                glm::vec3 spawnPos = player.Position + glm::vec3(rx, ry - 0.5f, rz);
                
                // Slowly falling leaves
                glm::vec3 vel(
                    (rand() % 100 / 100.0f - 0.5f) * 1.5f,
                    -1.5f - (rand() % 100 / 100.0f) * 1.5f,
                    (rand() % 100 / 100.0f - 0.5f) * 1.5f
                );
                // Green leaf color variations
                float g = 0.5f + (rand() % 100 / 200.0f);
                float r = 0.1f + (rand() % 100 / 1000.0f);
                float b = 0.1f + (rand() % 100 / 1000.0f);
                glm::vec4 color(r, g, b, 0.8f);
                
                particles.SpawnParticle(spawnPos, vel, color, 0.12f, 2.0f, -1.0f);
            }
            // Reset sound volume
            player.SoundVolumeEmitted = 0.0f;
        }

        footprints.Update(deltaTime);
        globalTime += deltaTime;

        // Draw Loop
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, INTERNAL_W, INTERNAL_H);
        glClearColor(fogCol.r, fogCol.g, fogCol.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Time"), globalTime);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), (float)INTERNAL_W, (float)INTERNAL_H);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Snap"), 1); // ENABLED SNAPPING 
        
        // FOG CONFIGURATION
        glUniform1f(glGetUniformLocation(shaderProgram, "u_FogStart"), Config::World::FogDistStart);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_FogEnd"), Config::World::FogDistEnd);
        
        // DAY/NIGHT & TORCH FIRE LIGHTING
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsNight"), isNightTime ? 1 : 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Darkness"), nightFactor * 0.95f);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_TorchActive"), player.HasTorchActive ? 1 : 0);
        glm::vec3 playerTorchPos = player.GetTorchPosition();
        glUniform3f(glGetUniformLocation(shaderProgram, "u_TorchPos"), playerTorchPos.x, playerTorchPos.y, playerTorchPos.z);

        std::vector<glm::vec4> worldTorches = buildingSystem.GetClosestTorches(player.Position, 8);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_NumWorldTorches"), (int)worldTorches.size());
        if (!worldTorches.empty()) {
            glUniform4fv(glGetUniformLocation(shaderProgram, "u_WorldTorches"), (GLsizei)worldTorches.size(), glm::value_ptr(worldTorches[0]));
        }

        glUniform3f(glGetUniformLocation(shaderProgram, "u_PlayerPos"), player.Position.x, player.Position.y, player.Position.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_PlayerFront"), player.Front.x, player.Front.y, player.Front.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "u_FogColor"), fogCol.r, fogCol.g, fogCol.b);

        glm::mat4 view;
        if (debugCam) {
            view = glm::lookAt(freeCamPos, freeCamPos + freeCamFront, glm::vec3(0,1,0));
        } else if (isGameOver) {
            // Death Camera Animation!
            // Time elapsed since death started (gameOverTimer starts at 3.5f and counts down to 0.0f)
            float fallProgress = glm::clamp((3.5f - gameOverTimer) / 1.5f, 0.0f, 1.0f); // Fall over 1.5 seconds
            
            // Get original player eye position
            glm::vec3 pos = player.Position;
            if (player.IsGrounded) {
                if (player.HeadBobTimer > 0.001f) {
                    pos.y += sin(player.HeadBobTimer) * player.HeadBobAmount;
                } else {
                    pos.y += sin(player.BreathTimer) * player.BreathAmount;
                }
            }
            
            // Camera falls to the ground
            float terrainHeight = WorldGenerator::GetHeight(player.Position.x, player.Position.z);
            float targetY = terrainHeight + 0.15f; // eye level when lying on the ground
            
            // Smoothly interpolate camera Y position to targetY
            pos.y = glm::mix(pos.y, targetY, fallProgress);
            
            // Create base lookAt
            view = glm::lookAt(pos, pos + player.Front, player.Up);
            
            // Apply a camera roll (tilt sideways) and pitch dip as we fall
            float rollAngle = fallProgress * 75.0f; // Roll 75 degrees
            float pitchAngle = sin(fallProgress * 3.14159f) * -15.0f; // Dip head down then up slightly
            
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(rollAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            rot = glm::rotate(rot, glm::radians(pitchAngle), glm::vec3(1.0f, 0.0f, 0.0f));
            view = rot * view;
        } else {
            view = player.GetViewMatrix();
        }
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
        
        chunkManager.RenderTrees(shaderProgram, trunkVAO, leavesVAO, trunkVertexCount, leavesVertexCount, player.Position);

        // 3. Footprints
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0); // DISABLED: Using CPU Exact Height
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC FOOTPRINTS
        footprints.Render(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);

        glm::vec3 activeCamPos = debugCam ? freeCamPos : player.GetCameraPosition();

        particles.Render(shaderProgram, activeCamPos);
        
        // 4. Player & Monster Render (Normal)
        if (player.IsThirdPerson) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            player.Render(shaderProgram);
        }

        // Explicitly bind noise texture to match terrain style (Render will switch to white for eyes)
        for (auto& mPtr : monsters) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            mPtr->Render(shaderProgram, whiteTexID);
        }
        
        // Render Birds (Using noise texture)
        glBindTexture(GL_TEXTURE_2D, textureID);
        birds.Render(shaderProgram);

        // Render Critters (Butterflies, Fireflies, Jumping Frogs)
        critters.Render(shaderProgram);

        // Render Passive Mobs (Forest Deer)
        for (auto& deer : passiveMobs) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            deer->Render(shaderProgram);
            deer->RenderHealthBar(shaderProgram, activeCamPos);
        }

        // Render Enemy Mobs (Corrupted Warriors, Neutral Giants, Dark Mages)
        for (auto& enemy : enemyMobs) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            enemy->Render(shaderProgram);
            enemy->RenderHealthBar(shaderProgram, activeCamPos);
        }

        // Render Lake Water Monsters (Water Lurkers)
        for (auto& wm : waterMonsters) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            wm->Render(shaderProgram);
            wm->RenderHealthBar(shaderProgram, activeCamPos);
        }

        // Render Flying Dragon (Ancestral Wyvern)
        glBindTexture(GL_TEXTURE_2D, textureID);
        dragon.Render(shaderProgram);
        dragon.RenderHealthBar(shaderProgram, activeCamPos);

        // Render Magic Projectiles
        projectiles.Render(shaderProgram);

        // Render 3D Target Ring
        targeting.RenderTargetRing(shaderProgram);

        // Render Environmental Horror Props (Hanging Victims & Claw-Marked Trunks)
        nearbyTreesForProps.clear();
        chunkManager.GetTreesInRange(player.Position, 85.0f, nearbyTreesForProps);
        glBindTexture(GL_TEXTURE_2D, textureID);
        horrorProps.Render(shaderProgram, nearbyTreesForProps, globalTime, windDir);

        // Render Procedural World Structures (Ancient Ruin Pillars, Loot Chests, Sacrifice Altars)
        structureSystem.Render(shaderProgram, activeCamPos);

        // Render Physical 3D Ground Item Drops (Glowing Pouch Mesh)
        itemDropSystem.Render(shaderProgram, activeCamPos);

        // Render Placed Modular Buildings (Walls, Cave Ceilings, Placed Torches)
        glBindTexture(GL_TEXTURE_2D, textureID);
        buildingSystem.Render(shaderProgram, activeCamPos);

        // Render Building Ghost Preview (If in Build Mode)
        if (isBuildMode) {
            buildingSystem.RenderGhost(shaderProgram, currentBuildType, buildPos, currentBuildYaw, true, whiteTexID);
        }
        
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
                glm::vec3 markerColor = mPtr->HasVisualContact() ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                // Draw a high-visibility vertical beacon line
                DrawLine(mPos, mPos + glm::vec3(0.0f, 200.0f, 0.0f), markerColor, debugVAO, debugVBO);
                // Draw a horizontal cross on the ground to pinpoint exact position
                DrawLine(mPos - glm::vec3(2.5f, 0.0f, 0.0f), mPos + glm::vec3(2.5f, 0.0f, 0.0f), markerColor, debugVAO, debugVBO);
                DrawLine(mPos - glm::vec3(0.0f, 0.0f, 2.5f), mPos + glm::vec3(0.0f, 0.0f, 2.5f), markerColor, debugVAO, debugVBO);
            }
            
            glEnable(GL_DEPTH_TEST); // Restore depth test
        }

        // Terraforming Reticle & Deform Ring Indicator (ONLY in Shovel Mode [P])
        if (isShovelMode && hasTerraTarget) {
            glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
            glBindTexture(GL_TEXTURE_2D, whiteTexID);

            glm::vec3 ringColor = glm::vec3(0.35f, 0.90f, 0.40f); // Soft green reticle
            if (isDigging) {
                ringColor = glm::vec3(0.95f, 0.30f, 0.15f); // Deep orange/red excavation
            } else if (isBuilding) {
                ringColor = glm::vec3(0.25f, 0.80f, 1.0f); // Bright blue construction
            }

            DrawDonut(terraTarget.x, terraTarget.y + 0.08f, terraTarget.z, 3.3f, 3.6f, ringColor, debugVAO, debugVBO);
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

        // 5. Weapon (Overlay - Clear Depth - 1st Person Only)
        if (!player.IsThirdPerson) {
            glClear(GL_DEPTH_BUFFER_BIT); 
            glm::mat4 viewIdentity = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(viewIdentity));
            
            // Render 1st Person Sword (Right Hand)
            glBindTexture(GL_TEXTURE_2D, textureID);
            player.RenderFirstPersonSword(shaderProgram);

            // Render 1st Person Torch (Left Hand)
            player.RenderFirstPersonTorch(shaderProgram);

            // (Shotgun deactivated for ARPG mode):
            // weapon.Render(shaderProgram, isGameOver, gameOverTimer);
        }

        // =================================================================================
        // PASS 2: UPSCALE RETRO 3D SCENE TO SCREEN FRAMEBUFFER
        // =================================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default screen framebuffer
        glViewport(0, 0, screenW, screenH); 
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT); 

        glDisable(GL_DEPTH_TEST); // Disable depth for 2D Quad
        glUseProgram(screenShader);
        glBindVertexArray(quadVAO);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer); 
        glUniform1i(glGetUniformLocation(screenShader, "u_ScreenTexture"), 0);
        glUniform1i(glGetUniformLocation(screenShader, "u_IsGameOver"), isGameOver ? 1 : 0);
        glUniform1f(glGetUniformLocation(screenShader, "u_GameOverTime"), gameOverTimer);
        glUniform1f(glGetUniformLocation(screenShader, "u_Time"), globalTime);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // =================================================================================
        // PASS 3: NATIVE ULTRA-CRISP HD UI PASS (DIRECTLY ON MONITOR RESOLUTION)
        // =================================================================================
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(uiProgram);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(uiVAO); 
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);
        glDisableVertexAttribArray(1); 
        glDisableVertexAttribArray(3); 
        glDisableVertexAttribArray(4); 
        
        glUniform1i(glGetUniformLocation(uiProgram, "u_UseTexture"), 0);
        glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(uiProgram, "u_Texture"), 0);

        // Crosshair (Visible ONLY in First Person)
        if (!player.IsThirdPerson) {
            std::vector<float> chData;
            float chS = 0.03f; float chT = 0.0022f;
            PushQuad(chData, -chS, -chT, chS*2, chT*2); 
            PushQuad(chData, -chT, -chS*INTERNAL_ASPECT, chT*2, chS*INTERNAL_ASPECT*2);  
            glBindVertexArray(uiVAO);
            glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
            glBufferData(GL_ARRAY_BUFFER, chData.size() * sizeof(float), chData.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(chData.size()/5));
            glBindVertexArray(0);
        }

        // FPS Counter con Symtext.ttf Ultra Nítido
        UIRenderer::DrawString("FPS " + std::to_string(currentFPS), 0.78f, 0.88f, 0.026f, glm::vec3(0.15f, 0.95f, 0.25f), uiProgram, uiVAO, uiVBO);

        // Wind Arrow
        float windAngle = atan2(windDir.y, windDir.x);
        float playerYawRad = glm::radians(player.Yaw);
        float relativeAngle = playerYawRad - windAngle + 1.5708f; 
        DrawArrow(0.9f, 0.1f, 0.05f, relativeAngle, uiVAO, uiVBO); 

        // Windows 98 ARPG HUD Pass (SUFFERING_MONITOR.EXE, ECG Oscilloscope, Taskbar, Target Frame & Damage Numbers)
        uiRenderer.RenderHUD(uiProgram, uiVAO, uiVBO, player.Stats, targeting, damageNumbers, projection * view, dangerLevel, globalTime, weatherSystem.GetNightCount(), weatherSystem.IsBloodMoon());

        // Interaction Prompts (Prioritized: Structures > Item Drops > Skinning > Fallen Corpses)
        std::string prompt = structureSystem.GetPrompt(player.Position);
        if (prompt.empty()) prompt = itemDropSystem.GetNearbyPrompt(player.Position);
        if (prompt.empty()) prompt = skinningSystem.GetPrompt(player.Position, passiveMobs);
        if (prompt.empty()) prompt = horrorProps.GetNearbyPrompt(player.Position);
        if (!prompt.empty() && !loreModal.active && !isBuildMode) {
            uiRenderer.RenderInteractionPrompt(uiProgram, uiVAO, uiVBO, prompt);
        }

        // Building Mode Interface
        if (isBuildMode) {
            uiRenderer.RenderBuildingHUD(uiProgram, uiVAO, uiVBO, (int)currentBuildType, currentBuildYaw);
        }

        // Spell Hotbar HUD ([Q], [E], [R])
        spellSystem.RenderHUDSpells(uiProgram, uiVAO, uiVBO);

        // Posture Broken Stun Warning
        if (player.StunTimer > 0.0f) {
            uiRenderer.RenderStunWarning(uiProgram, uiVAO, uiVBO, player.StunTimer);
        }

        // Character Stats & Attribute Allocation Panel (C key)
        if (isCharacterPanelOpen) {
            uiRenderer.RenderCharacterPanel(uiProgram, uiVAO, uiVBO, player.Stats, mouseNdcX, mouseNdcY);
        }

        // Forensic Lore Notepad Window (E key)
        if (loreModal.active) {
            uiRenderer.RenderLoreModal(uiProgram, uiVAO, uiVBO, loreModal, mouseNdcX, mouseNdcY);
        }

        // Windows 98 Critical Fatal Error Dialog
        if (fatalError.active) {
            uiRenderer.RenderFatalErrorModal(uiProgram, uiVAO, uiVBO, fatalError, mouseNdcX, mouseNdcY);
        }

        // Inventory & Equipment Window (I key)
        if (inventory.IsOpen()) {
            inventory.RenderWindow(uiProgram, uiVAO, uiVBO, mouseNdcX, mouseNdcY, &player.Stats);
        }

        // Render software cursor on top of UI if any modal is active or in 3rd person
        bool isUiActive = isCharacterPanelOpen || loreModal.active || fatalError.active || inventory.IsOpen();
        if (isUiActive || player.IsThirdPerson) {
            UIRenderer::RenderCursor(uiProgram, uiVAO, uiVBO, mouseNdcX, mouseNdcY);
        }

        // Restore State for next frame's Pass 1
        glClearColor(skyColor.r, skyColor.g, skyColor.b, skyColor.a);
        glEnable(GL_DEPTH_TEST);

#ifndef __EMSCRIPTEN__
        window.display();
#else
        glfwSwapBuffers(window);
#endif
    };

#ifdef __EMSCRIPTEN__
    static std::function<void()> s_EmscriptenMainLoop;
    s_EmscriptenMainLoop = updateAndRenderFrame;
    emscripten_set_main_loop([]() {
        s_EmscriptenMainLoop();
    }, 0, 1);
#else
    while (window.isOpen()) {
        updateAndRenderFrame();
    }
#endif

    return 0;
}
